/*
HOST_VR.C

OpenXR for the VR mode (HALO_VR; port/linux/src/vr_host.h, port/linux/src/
vr.c). The game draws with OpenGL through SDL's context, which OpenXR takes
through XR_KHR_opengl_enable: with an EGL context (SDL_VIDEO_FORCE_EGL, as
vr.c asks) through XR_MNDX_egl_enable's binding, else through GLX's. The
swapchains' images are then GL textures the game draws into directly: the
eyes are one 2-layer texture array, which the renderer draws both of at once
with GL_OVR_multiview2 (d3d8_gl.c).

The runtime is loaded without the OpenXR loader, which few distributions
package: as the loader does, from the active runtime's manifest
($XDG_CONFIG_HOME/openxr/1/active_runtime.json, then /etc/xdg), through
xrNegotiateLoaderRuntimeInterface. A libopenxr_loader.so.1 next to the
executable or on the system is used instead where there is one, for its API
layers. Tested with SteamVR on a Steam Frame.
*/

#define XR_USE_PLATFORM_EGL
#define XR_USE_PLATFORM_XLIB
#define XR_USE_GRAPHICS_API_OPENGL
#define XR_NO_PROTOTYPES

#include "host.h"
#include "vr_host.h"

#include <EGL/egl.h>
#include <X11/Xlib.h>
#include <dlfcn.h>
#include <limits.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

/* GLX's types, without its header (which would want the desktop GL one) */
typedef struct __GLXcontextRec *GLXContext;
typedef struct __GLXFBConfigRec *GLXFBConfig;
typedef XID GLXDrawable;

#include <openxr/openxr.h>
#include <openxr/openxr_platform.h>
#include <openxr/openxr_loader_negotiation.h>

#define GL_SRGB8_ALPHA8 0x8C43
#define GL_TEXTURE_2D_ARRAY 0x8C1A

/* ---------- the runtime's functions */

#define XR_FUNCTIONS(X) \
	X(xrEnumerateInstanceExtensionProperties) \
	X(xrCreateInstance) \
	X(xrDestroyInstance) \
	X(xrGetInstanceProperties) \
	X(xrGetSystem) \
	X(xrGetSystemProperties) \
	X(xrEnumerateViewConfigurationViews) \
	X(xrCreateSession) \
	X(xrDestroySession) \
	X(xrBeginSession) \
	X(xrEndSession) \
	X(xrPollEvent) \
	X(xrResultToString) \
	X(xrCreateReferenceSpace) \
	X(xrLocateSpace) \
	X(xrLocateViews) \
	X(xrEnumerateSwapchainFormats) \
	X(xrCreateSwapchain) \
	X(xrEnumerateSwapchainImages) \
	X(xrAcquireSwapchainImage) \
	X(xrWaitSwapchainImage) \
	X(xrReleaseSwapchainImage) \
	X(xrWaitFrame) \
	X(xrBeginFrame) \
	X(xrEndFrame) \
	X(xrStringToPath) \
	X(xrCreateActionSet) \
	X(xrCreateAction) \
	X(xrSuggestInteractionProfileBindings) \
	X(xrAttachSessionActionSets) \
	X(xrCreateActionSpace) \
	X(xrSyncActions) \
	X(xrGetActionStateBoolean) \
	X(xrGetActionStateFloat) \
	X(xrGetActionStateVector2f) \
	X(xrGetActionStatePose) \
	X(xrApplyHapticFeedback)

#define XR_DECLARE(name) static PFN_##name name;
XR_FUNCTIONS(XR_DECLARE)
#undef XR_DECLARE
static PFN_xrGetInstanceProcAddr get_instance_proc_address;
static PFN_xrGetOpenGLGraphicsRequirementsKHR xrGetOpenGLGraphicsRequirementsKHR;

/* the actions (action_names, bindings) */
enum
{
	_action_a, _action_b, _action_x, _action_y, _action_menu, _action_view,
	_action_dpad_up, _action_dpad_down, _action_dpad_left, _action_dpad_right,
	_action_left_stick_click, _action_right_stick_click, _action_left_bumper, _action_right_bumper,
	NUMBER_OF_BUTTON_ACTIONS,
	_action_left_stick = NUMBER_OF_BUTTON_ACTIONS, _action_right_stick,
	_action_left_trigger, _action_right_trigger, _action_left_grip, _action_right_grip,
	_action_aim_pose, _action_grip_pose, _action_haptic,
	NUMBER_OF_ACTIONS
};

static struct
{
	void *library;
	XrInstance instance;
	XrSystemId system;
	XrSession session;
	XrSessionState state;
	int running;
	int frame_begun;
	XrFrameState frame;
	XrTime last_display_time;
	XrSpace play_space;
	XrSpace view_space;
	XrSwapchain swapchains[VR_SWAPCHAIN_COUNT];
	int swapchain_width[VR_SWAPCHAIN_COUNT];
	int swapchain_height[VR_SWAPCHAIN_COUNT];
	int acquired[VR_SWAPCHAIN_COUNT];
	XrActionSet action_set;
	XrAction actions[NUMBER_OF_ACTIONS];
	XrPath hands[2];
	XrSpace aim_spaces[2];
	XrSpace grip_spaces[2];
	int has_egl_binding;
	int has_frame_controller;
	int has_refresh_rate;
	int want_depth, has_depth;
} vr;

static const char *result_name(XrResult result)
{
	static char text[XR_MAX_RESULT_STRING_SIZE];

	if (vr.instance && xrResultToString && xrResultToString(vr.instance, result, text) == XR_SUCCESS)
		return text;
	snprintf(text, sizeof(text), "XrResult %d", (int)result);
	return text;
}

static int check(XrResult result, const char *what)
{
	if (XR_SUCCEEDED(result))
		return 1;
	host_logf(HOST_LOG_ERROR, "vr: %s: %s", what, result_name(result));
	return 0;
}

/* ---------- loading the runtime */

/* the manifest's library_path, made absolute from its folder */
static int manifest_library(const char *manifest, char *library, size_t size)
{
	char real[PATH_MAX], text[8192], *key, *value, *end, *slash;
	FILE *file;
	size_t length;

	if (!realpath(manifest, real) || !(file = fopen(real, "rb")))
		return 0;
	length = fread(text, 1, sizeof(text) - 1, file);
	fclose(file);
	text[length] = 0;
	if (!(key = strstr(text, "\"library_path\"")) || !(value = strchr(key + 14, ':')) ||
		!(value = strchr(value, '"')) || !(end = strchr(++value, '"')))
	{
		return 0;
	}
	*end = 0;
	if (value[0] == '/')
	{
		snprintf(library, size, "%s", value);
	}
	else
	{
		slash = strrchr(real, '/');
		if (slash)
			*slash = 0;
		snprintf(library, size, "%s/%s", real, value);
	}
	return 1;
}

static int runtime_load(void)
{
	char manifest[PATH_MAX], library[PATH_MAX];
	const char *config = getenv("XDG_CONFIG_HOME"), *home = getenv("HOME"), *override = getenv("XR_RUNTIME_JSON");
	PFN_xrNegotiateLoaderRuntimeInterface negotiate;
	XrNegotiateLoaderInfo loader = { 0 };
	XrNegotiateRuntimeRequest request = { 0 };

	/* the loader, where there is one */
	vr.library = dlopen("libopenxr_loader.so.1", RTLD_NOW | RTLD_LOCAL);
	if (vr.library)
	{
		get_instance_proc_address = (PFN_xrGetInstanceProcAddr)dlsym(vr.library, "xrGetInstanceProcAddr");
		if (get_instance_proc_address)
		{
			host_logf(HOST_LOG_INFO, "vr: the OpenXR loader libopenxr_loader.so.1");
			return 1;
		}
		dlclose(vr.library);
	}
	/* else the active runtime itself */
	if (override && *override)
		snprintf(manifest, sizeof(manifest), "%s", override);
	else if (config && *config)
		snprintf(manifest, sizeof(manifest), "%s/openxr/1/active_runtime.json", config);
	else
		snprintf(manifest, sizeof(manifest), "%s/.config/openxr/1/active_runtime.json", home ? home : "");
	if (!manifest_library(manifest, library, sizeof(library)))
	{
		snprintf(manifest, sizeof(manifest), "/etc/xdg/openxr/1/active_runtime.json");
		if (!manifest_library(manifest, library, sizeof(library)))
		{
			host_logf(HOST_LOG_WARN, "vr: no active OpenXR runtime");
			return 0;
		}
	}
	vr.library = dlopen(library, RTLD_NOW | RTLD_LOCAL);
	if (!vr.library)
	{
		host_logf(HOST_LOG_WARN, "vr: cannot load the OpenXR runtime %s: %s", library, dlerror());
		return 0;
	}
	negotiate = (PFN_xrNegotiateLoaderRuntimeInterface)dlsym(vr.library, "xrNegotiateLoaderRuntimeInterface");
	if (!negotiate)
		return 0;
	loader.structType = XR_LOADER_INTERFACE_STRUCT_LOADER_INFO;
	loader.structVersion = XR_LOADER_INFO_STRUCT_VERSION;
	loader.structSize = sizeof(loader);
	loader.minInterfaceVersion = 1;
	loader.maxInterfaceVersion = XR_CURRENT_LOADER_RUNTIME_VERSION;
	loader.minApiVersion = XR_MAKE_VERSION(1, 0, 0);
	loader.maxApiVersion = XR_MAKE_VERSION(1, 1, 0xffffffff);
	request.structType = XR_LOADER_INTERFACE_STRUCT_RUNTIME_REQUEST;
	request.structVersion = XR_RUNTIME_INFO_STRUCT_VERSION;
	request.structSize = sizeof(request);
	if (negotiate(&loader, &request) != XR_SUCCESS || !request.getInstanceProcAddr)
	{
		host_logf(HOST_LOG_WARN, "vr: the OpenXR runtime %s refused the negotiation", library);
		return 0;
	}
	get_instance_proc_address = request.getInstanceProcAddr;
	host_logf(HOST_LOG_INFO, "vr: the OpenXR runtime %s (from %s)", library, manifest);
	return 1;
}

static int functions_load(XrInstance instance)
{
	int success = 1;

#define XR_LOAD(name) \
	if (get_instance_proc_address(instance, #name, (PFN_xrVoidFunction *)&name) != XR_SUCCESS || !name) \
	{ \
		if (instance) \
		{ \
			host_logf(HOST_LOG_ERROR, "vr: no %s", #name); \
			success = 0; \
		} \
	}
	XR_FUNCTIONS(XR_LOAD)
#undef XR_LOAD
	return success;
}

/* ---------- the instance and session */

static int extension_listed(const XrExtensionProperties *extensions, uint32_t count, const char *name)
{
	uint32_t index;

	for (index = 0; index < count; index++)
	{
		if (!strcmp(extensions[index].extensionName, name))
			return 1;
	}
	return 0;
}

static int instance_create(void)
{
	XrExtensionProperties *extensions;
	uint32_t count = 0, index;
	const char *enabled[8];
	uint32_t enabled_count = 0;
	XrInstanceCreateInfo create = { XR_TYPE_INSTANCE_CREATE_INFO };
	XrInstanceProperties properties = { XR_TYPE_INSTANCE_PROPERTIES };

	functions_load(XR_NULL_HANDLE);
	if (!xrEnumerateInstanceExtensionProperties || !xrCreateInstance ||
		xrEnumerateInstanceExtensionProperties(NULL, 0, &count, NULL) != XR_SUCCESS)
	{
		return 0;
	}
	extensions = calloc(count, sizeof(*extensions));
	for (index = 0; index < count; index++)
		extensions[index].type = XR_TYPE_EXTENSION_PROPERTIES;
	xrEnumerateInstanceExtensionProperties(NULL, count, &count, extensions);
	if (!extension_listed(extensions, count, XR_KHR_OPENGL_ENABLE_EXTENSION_NAME))
	{
		host_logf(HOST_LOG_ERROR, "vr: the OpenXR runtime has no OpenGL binding (XR_KHR_opengl_enable)");
		free(extensions);
		return 0;
	}
	enabled[enabled_count++] = XR_KHR_OPENGL_ENABLE_EXTENSION_NAME;
	if (extension_listed(extensions, count, XR_MNDX_EGL_ENABLE_EXTENSION_NAME))
	{
		enabled[enabled_count++] = XR_MNDX_EGL_ENABLE_EXTENSION_NAME;
		vr.has_egl_binding = 1;
	}
	if (extension_listed(extensions, count, "XR_VALVE_frame_controller_interaction"))
	{
		enabled[enabled_count++] = "XR_VALVE_frame_controller_interaction";
		vr.has_frame_controller = 1;
	}
	if (extension_listed(extensions, count, XR_FB_DISPLAY_REFRESH_RATE_EXTENSION_NAME))
	{
		enabled[enabled_count++] = XR_FB_DISPLAY_REFRESH_RATE_EXTENSION_NAME;
		vr.has_refresh_rate = 1;
	}
	if (vr.want_depth && extension_listed(extensions, count, XR_KHR_COMPOSITION_LAYER_DEPTH_EXTENSION_NAME))
	{
		enabled[enabled_count++] = XR_KHR_COMPOSITION_LAYER_DEPTH_EXTENSION_NAME;
		vr.has_depth = 1;
	}
	free(extensions);

	strcpy(create.applicationInfo.applicationName, "Halo: Combat Evolved");
	create.applicationInfo.applicationVersion = 1;
	strcpy(create.applicationInfo.engineName, "halo-ce-universal");
	create.applicationInfo.apiVersion = XR_MAKE_VERSION(1, 0, 0);
	create.enabledExtensionCount = enabled_count;
	create.enabledExtensionNames = enabled;
	if (!check(xrCreateInstance(&create, &vr.instance), "xrCreateInstance"))
		return 0;
	if (!functions_load(vr.instance))
		return 0;
	get_instance_proc_address(vr.instance, "xrGetOpenGLGraphicsRequirementsKHR",
		(PFN_xrVoidFunction *)&xrGetOpenGLGraphicsRequirementsKHR);
	if (xrGetInstanceProperties(vr.instance, &properties) == XR_SUCCESS)
	{
		host_logf(HOST_LOG_INFO, "vr: %s %u.%u.%u", properties.runtimeName,
			(unsigned)XR_VERSION_MAJOR(properties.runtimeVersion), (unsigned)XR_VERSION_MINOR(properties.runtimeVersion),
			(unsigned)XR_VERSION_PATCH(properties.runtimeVersion));
	}
	return 1;
}

/* the binding of the current GL context */
static int session_create(void)
{
	XrSystemGetInfo system = { XR_TYPE_SYSTEM_GET_INFO };
	XrGraphicsRequirementsOpenGLKHR requirements = { XR_TYPE_GRAPHICS_REQUIREMENTS_OPENGL_KHR };
	XrSessionCreateInfo create = { XR_TYPE_SESSION_CREATE_INFO };
	XrGraphicsBindingEGLMNDX egl = { XR_TYPE_GRAPHICS_BINDING_EGL_MNDX };
	XrGraphicsBindingOpenGLXlibKHR xlib = { XR_TYPE_GRAPHICS_BINDING_OPENGL_XLIB_KHR };

	system.formFactor = XR_FORM_FACTOR_HEAD_MOUNTED_DISPLAY;
	if (!check(xrGetSystem(vr.instance, &system, &vr.system), "xrGetSystem (is the headset on?)"))
		return 0;
	if (xrGetOpenGLGraphicsRequirementsKHR)
		xrGetOpenGLGraphicsRequirementsKHR(vr.instance, vr.system, &requirements);

	if (vr.has_egl_binding && eglGetCurrentContext() != EGL_NO_CONTEXT)
	{
		EGLint config_id = 0, count = 0;
		EGLint attributes[] = { EGL_CONFIG_ID, 0, EGL_NONE };
		EGLConfig config = NULL;

		egl.getProcAddress = (PFN_xrEglGetProcAddressMNDX)eglGetProcAddress;
		egl.display = eglGetCurrentDisplay();
		egl.context = eglGetCurrentContext();
		eglQueryContext(egl.display, egl.context, EGL_CONFIG_ID, &config_id);
		attributes[1] = config_id;
		eglChooseConfig(egl.display, attributes, &config, 1, &count);
		egl.config = config;
		create.next = &egl;
		host_logf(HOST_LOG_INFO, "vr: EGL binding (config %d)", (int)config_id);
	}
	else
	{
		void *gl = dlopen("libGL.so.1", RTLD_NOW | RTLD_GLOBAL);
		Display *(*current_display)(void) = gl ? (Display *(*)(void))dlsym(gl, "glXGetCurrentDisplay") : NULL;
		GLXContext (*current_context)(void) = gl ? (GLXContext (*)(void))dlsym(gl, "glXGetCurrentContext") : NULL;
		GLXDrawable (*current_drawable)(void) = gl ? (GLXDrawable (*)(void))dlsym(gl, "glXGetCurrentDrawable") : NULL;
		int (*query_context)(Display *, GLXContext, int, int *) =
			gl ? (int (*)(Display *, GLXContext, int, int *))dlsym(gl, "glXQueryContext") : NULL;
		GLXFBConfig *(*choose)(Display *, int, const int *, int *) =
			gl ? (GLXFBConfig *(*)(Display *, int, const int *, int *))dlsym(gl, "glXChooseFBConfig") : NULL;
		int (*config_attribute)(Display *, GLXFBConfig, int, int *) =
			gl ? (int (*)(Display *, GLXFBConfig, int, int *))dlsym(gl, "glXGetFBConfigAttrib") : NULL;
		int id = 0, screen = 0, count = 0, visual = 0;
		int attributes[] = { 0x8013 /* GLX_FBCONFIG_ID */, 0, 0 };
		GLXFBConfig *configs;

		if (!current_context || !current_context())
		{
			host_logf(HOST_LOG_ERROR, "vr: no current EGL or GLX context");
			return 0;
		}
		xlib.xDisplay = current_display();
		xlib.glxContext = current_context();
		xlib.glxDrawable = current_drawable();
		query_context(xlib.xDisplay, xlib.glxContext, 0x8013, &id);
		query_context(xlib.xDisplay, xlib.glxContext, 0x800C /* GLX_SCREEN */, &screen);
		attributes[1] = id;
		configs = choose(xlib.xDisplay, screen, attributes, &count);
		if (configs && count)
		{
			xlib.glxFBConfig = configs[0];
			config_attribute(xlib.xDisplay, configs[0], 0x800B /* GLX_VISUAL_ID */, &visual);
			xlib.visualid = (uint32_t)visual;
		}
		create.next = &xlib;
		host_logf(HOST_LOG_INFO, "vr: GLX binding");
	}
	create.systemId = vr.system;
	return check(xrCreateSession(vr.instance, &create, &vr.session), "xrCreateSession");
}

static int swapchain_create(int which, int width, int height, int layers, struct vr_host_info *info)
{
	int64_t formats[64];
	uint32_t format_count = 0, format_index;
	XrSwapchainCreateInfo create = { XR_TYPE_SWAPCHAIN_CREATE_INFO };
	XrSwapchainImageOpenGLKHR images[VR_SWAPCHAIN_IMAGES];
	uint32_t count = 0, index;

	/* sRGB, the only colour formats SteamVR offers: the renderer writes and
	reads its values as they are (no GL_FRAMEBUFFER_SRGB, no decoding), and
	the compositor shows them as the sRGB values they already are */
	create.usageFlags = XR_SWAPCHAIN_USAGE_COLOR_ATTACHMENT_BIT | XR_SWAPCHAIN_USAGE_SAMPLED_BIT;
	create.format = GL_SRGB8_ALPHA8;
	if (which == VR_SWAPCHAIN_DEPTH)
	{
		/* 32-bit float depth where offered, else 24 or 16-bit. (SteamVR 2.18
		on the Steam Frame lists these but cannot make any of them for a GL
		session: vr.depth is off by default) */
		static const int64_t preferences[] = { 0x8CAC, 0x81A6, 0x81A5 };
		int preference;

		create.usageFlags = XR_SWAPCHAIN_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
		create.format = 0;
		xrEnumerateSwapchainFormats(vr.session, 64, &format_count, formats);
		for (preference = 2; preference >= 0; preference--)
		{
			for (format_index = 0; format_index < format_count; format_index++)
			{
				if (formats[format_index] == preferences[preference])
					create.format = formats[format_index];
			}
		}
		host_logf(HOST_LOG_INFO, "vr: depth format 0x%llx", (long long)create.format);
		if (!create.format)
			return 0;
	}
	create.sampleCount = 1;
	create.width = (uint32_t)width;
	create.height = (uint32_t)height;
	create.faceCount = 1;
	create.arraySize = (uint32_t)layers;
	create.mipCount = 1;
	if (!check(xrCreateSwapchain(vr.session, &create, &vr.swapchains[which]), "xrCreateSwapchain"))
		return 0;
	for (index = 0; index < VR_SWAPCHAIN_IMAGES; index++)
	{
		images[index].type = XR_TYPE_SWAPCHAIN_IMAGE_OPENGL_KHR;
		images[index].next = NULL;
	}
	if (!check(xrEnumerateSwapchainImages(vr.swapchains[which], VR_SWAPCHAIN_IMAGES, &count,
		(XrSwapchainImageBaseHeader *)images), "xrEnumerateSwapchainImages"))
	{
		return 0;
	}
	info->image_count[which] = (int)count;
	for (index = 0; index < count; index++)
		info->images[which][index] = images[index].image;
	vr.swapchain_width[which] = width;
	vr.swapchain_height[which] = height;
	vr.acquired[which] = -1;
	return 1;
}

/* ---------- the controllers */

static const char *const action_names[NUMBER_OF_ACTIONS] =
{
	"a", "b", "x", "y", "menu", "view", "dpad_up", "dpad_down", "dpad_left", "dpad_right",
	"left_stick_click", "right_stick_click", "left_bumper", "right_bumper",
	"left_stick", "right_stick", "left_trigger", "right_trigger", "left_grip", "right_grip",
	"aim_pose", "grip_pose", "haptic",
};

struct binding
{
	int action;
	const char *path;
};

/* the Steam Frame's controllers: a gamepad in two halves (the d-pad and
View on the left, A, B, X, Y and Menu on the right) */
static const struct binding frame_controller_bindings[] =
{
	{ _action_a, "/user/hand/right/input/a/click" },
	{ _action_b, "/user/hand/right/input/b/click" },
	{ _action_x, "/user/hand/right/input/x/click" },
	{ _action_y, "/user/hand/right/input/y/click" },
	{ _action_menu, "/user/hand/right/input/menu/click" },
	{ _action_view, "/user/hand/left/input/view/click" },
	{ _action_dpad_up, "/user/hand/left/input/dpad_up/click" },
	{ _action_dpad_down, "/user/hand/left/input/dpad_down/click" },
	{ _action_dpad_left, "/user/hand/left/input/dpad_left/click" },
	{ _action_dpad_right, "/user/hand/left/input/dpad_right/click" },
	{ _action_left_stick_click, "/user/hand/left/input/thumbstick/click" },
	{ _action_right_stick_click, "/user/hand/right/input/thumbstick/click" },
	{ _action_left_bumper, "/user/hand/left/input/bumper/click" },
	{ _action_right_bumper, "/user/hand/right/input/bumper/click" },
	{ _action_left_stick, "/user/hand/left/input/thumbstick" },
	{ _action_right_stick, "/user/hand/right/input/thumbstick" },
	{ _action_left_trigger, "/user/hand/left/input/trigger/value" },
	{ _action_right_trigger, "/user/hand/right/input/trigger/value" },
	{ _action_left_grip, "/user/hand/left/input/squeeze/value" },
	{ _action_right_grip, "/user/hand/right/input/squeeze/value" },
	{ _action_aim_pose, "/user/hand/left/input/aim/pose" },
	{ _action_aim_pose, "/user/hand/right/input/aim/pose" },
	{ _action_grip_pose, "/user/hand/left/input/grip/pose" },
	{ _action_grip_pose, "/user/hand/right/input/grip/pose" },
	{ _action_haptic, "/user/hand/left/output/haptic" },
	{ _action_haptic, "/user/hand/right/output/haptic" },
};

/* Touch-style controllers (what SteamVR maps other controllers onto): X
and Y on the left, the grips as the bumpers, no d-pad or View */
static const struct binding touch_bindings[] =
{
	{ _action_a, "/user/hand/right/input/a/click" },
	{ _action_b, "/user/hand/right/input/b/click" },
	{ _action_x, "/user/hand/left/input/x/click" },
	{ _action_y, "/user/hand/left/input/y/click" },
	{ _action_menu, "/user/hand/left/input/menu/click" },
	{ _action_left_stick_click, "/user/hand/left/input/thumbstick/click" },
	{ _action_right_stick_click, "/user/hand/right/input/thumbstick/click" },
	{ _action_left_bumper, "/user/hand/left/input/squeeze/value" },
	{ _action_right_bumper, "/user/hand/right/input/squeeze/value" },
	{ _action_left_stick, "/user/hand/left/input/thumbstick" },
	{ _action_right_stick, "/user/hand/right/input/thumbstick" },
	{ _action_left_trigger, "/user/hand/left/input/trigger/value" },
	{ _action_right_trigger, "/user/hand/right/input/trigger/value" },
	{ _action_aim_pose, "/user/hand/left/input/aim/pose" },
	{ _action_aim_pose, "/user/hand/right/input/aim/pose" },
	{ _action_grip_pose, "/user/hand/left/input/grip/pose" },
	{ _action_grip_pose, "/user/hand/right/input/grip/pose" },
	{ _action_haptic, "/user/hand/left/output/haptic" },
	{ _action_haptic, "/user/hand/right/output/haptic" },
};

static void bindings_suggest(const char *profile, const struct binding *bindings, int count)
{
	XrActionSuggestedBinding suggested[32];
	XrInteractionProfileSuggestedBinding suggestion = { XR_TYPE_INTERACTION_PROFILE_SUGGESTED_BINDING };
	XrPath profile_path;
	XrResult result;
	int index;

	if (xrStringToPath(vr.instance, profile, &profile_path) != XR_SUCCESS)
		return;
	for (index = 0; index < count && index < 32; index++)
	{
		suggested[index].action = vr.actions[bindings[index].action];
		xrStringToPath(vr.instance, bindings[index].path, &suggested[index].binding);
	}
	suggestion.interactionProfile = profile_path;
	suggestion.countSuggestedBindings = (uint32_t)index;
	suggestion.suggestedBindings = suggested;
	result = xrSuggestInteractionProfileBindings(vr.instance, &suggestion);
	if (result == XR_ERROR_PATH_UNSUPPORTED)
	{
		/* a path the runtime's profile lacks spoils all of them: find it
		(each alone) and suggest the rest */
		int kept = 0;

		count = index;
		for (index = 0; index < count; index++)
		{
			suggestion.countSuggestedBindings = 1;
			suggestion.suggestedBindings = &suggested[index];
			if (xrSuggestInteractionProfileBindings(vr.instance, &suggestion) == XR_SUCCESS)
				suggested[kept++] = suggested[index];
			else
				host_logf(HOST_LOG_WARN, "vr: %s has no %s", profile, bindings[index].path);
		}
		suggestion.countSuggestedBindings = (uint32_t)kept;
		suggestion.suggestedBindings = suggested;
		result = xrSuggestInteractionProfileBindings(vr.instance, &suggestion);
	}
	if (result != XR_SUCCESS)
		host_logf(HOST_LOG_WARN, "vr: bindings for %s: %s", profile, result_name(result));
}

static int actions_create(void)
{
	XrActionSetCreateInfo set = { XR_TYPE_ACTION_SET_CREATE_INFO };
	XrSessionActionSetsAttachInfo attach = { XR_TYPE_SESSION_ACTION_SETS_ATTACH_INFO };
	int index, hand;

	strcpy(set.actionSetName, "gameplay");
	strcpy(set.localizedActionSetName, "Gameplay");
	if (!check(xrCreateActionSet(vr.instance, &set, &vr.action_set), "xrCreateActionSet"))
		return 0;
	xrStringToPath(vr.instance, "/user/hand/left", &vr.hands[0]);
	xrStringToPath(vr.instance, "/user/hand/right", &vr.hands[1]);
	for (index = 0; index < NUMBER_OF_ACTIONS; index++)
	{
		XrActionCreateInfo action = { XR_TYPE_ACTION_CREATE_INFO };

		action.actionType = index < NUMBER_OF_BUTTON_ACTIONS ? XR_ACTION_TYPE_BOOLEAN_INPUT :
			index <= _action_right_stick ? XR_ACTION_TYPE_VECTOR2F_INPUT :
			index <= _action_right_grip ? XR_ACTION_TYPE_FLOAT_INPUT :
			index == _action_haptic ? XR_ACTION_TYPE_VIBRATION_OUTPUT : XR_ACTION_TYPE_POSE_INPUT;
		if (index >= _action_aim_pose)
		{
			action.countSubactionPaths = 2;
			action.subactionPaths = vr.hands;
		}
		strcpy(action.actionName, action_names[index]);
		strcpy(action.localizedActionName, action_names[index]);
		if (!check(xrCreateAction(vr.action_set, &action, &vr.actions[index]), "xrCreateAction"))
			return 0;
	}
	/* (booleans bound to the Touch squeeze's value, a float, are pressed
	past the runtime's threshold) */
	if (vr.has_frame_controller)
		bindings_suggest("/interaction_profiles/valve/frame_controller", frame_controller_bindings,
			(int)(sizeof(frame_controller_bindings) / sizeof(frame_controller_bindings[0])));
	bindings_suggest("/interaction_profiles/oculus/touch_controller", touch_bindings,
		(int)(sizeof(touch_bindings) / sizeof(touch_bindings[0])));
	attach.countActionSets = 1;
	attach.actionSets = &vr.action_set;
	if (!check(xrAttachSessionActionSets(vr.session, &attach), "xrAttachSessionActionSets"))
		return 0;
	for (hand = 0; hand < 2; hand++)
	{
		XrActionSpaceCreateInfo space = { XR_TYPE_ACTION_SPACE_CREATE_INFO };

		space.subactionPath = vr.hands[hand];
		space.poseInActionSpace.orientation.w = 1.0f;
		space.action = vr.actions[_action_aim_pose];
		xrCreateActionSpace(vr.session, &space, &vr.aim_spaces[hand]);
		space.action = vr.actions[_action_grip_pose];
		xrCreateActionSpace(vr.session, &space, &vr.grip_spaces[hand]);
	}
	return 1;
}

/* ---------- timing (HALO_VR_TIMING: what the runtime's calls cost) */

static struct
{
	int enabled;
	double acquire, release, end, frames;
} timing = { -1 };

static double seconds_now(void)
{
	struct timespec now;

	clock_gettime(CLOCK_MONOTONIC, &now);
	return (double)now.tv_sec + (double)now.tv_nsec * 1e-9;
}

static void timing_frame(void)
{
	if (++timing.frames >= 300)
	{
		host_logf(HOST_LOG_INFO, "vr: runtime calls a frame: acquire %.2f ms, release %.2f ms, end %.2f ms",
			timing.acquire * 1000.0 / timing.frames, timing.release * 1000.0 / timing.frames,
			timing.end * 1000.0 / timing.frames);
		timing.acquire = timing.release = timing.end = timing.frames = 0.0;
	}
}

/* ---------- the public functions */

int host_vr_initialize(struct vr_host_info *info)
{
	XrViewConfigurationView views[2] = { { XR_TYPE_VIEW_CONFIGURATION_VIEW }, { XR_TYPE_VIEW_CONFIGURATION_VIEW } };
	XrSystemProperties properties = { XR_TYPE_SYSTEM_PROPERTIES };
	XrReferenceSpaceCreateInfo space = { XR_TYPE_REFERENCE_SPACE_CREATE_INFO };
	uint32_t count = 0;
	int width, height;

	if (vr.session)
		return 1;
	vr.want_depth = info->depth;
	if (!runtime_load() || !instance_create() || !session_create())
		return 0;
	if (xrGetSystemProperties(vr.instance, vr.system, &properties) == XR_SUCCESS)
		snprintf(info->system_name, sizeof(info->system_name), "%s", properties.systemName);
	if (!check(xrEnumerateViewConfigurationViews(vr.instance, vr.system, XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO,
		2, &count, views), "xrEnumerateViewConfigurationViews"))
	{
		return 0;
	}
	info->recommended_width = (int)views[0].recommendedImageRectWidth;
	info->recommended_height = (int)views[0].recommendedImageRectHeight;
	width = info->eye_width > 0 ? info->eye_width :
		(int)(views[0].recommendedImageRectWidth * (info->scale > 0.0f ? info->scale : 1.0f) + 0.5f);
	height = info->eye_height > 0 ? info->eye_height :
		(int)(views[0].recommendedImageRectHeight * (info->scale > 0.0f ? info->scale : 1.0f) + 0.5f);
	if (width > (int)views[0].maxImageRectWidth)
		width = (int)views[0].maxImageRectWidth;
	if (height > (int)views[0].maxImageRectHeight)
		height = (int)views[0].maxImageRectHeight;
	info->eye_width = width & ~1;
	info->eye_height = height & ~1;
	if (!swapchain_create(VR_SWAPCHAIN_EYES, info->eye_width, info->eye_height, 2, info) ||
		!swapchain_create(VR_SWAPCHAIN_HUD, info->hud_width, info->hud_height, 1, info))
	{
		return 0;
	}
	info->depth = vr.has_depth && swapchain_create(VR_SWAPCHAIN_DEPTH, info->eye_width, info->eye_height, 2, info);

	space.poseInReferenceSpace.orientation.w = 1.0f;
	space.referenceSpaceType = XR_REFERENCE_SPACE_TYPE_VIEW;
	check(xrCreateReferenceSpace(vr.session, &space, &vr.view_space), "xrCreateReferenceSpace (view)");
	space.referenceSpaceType = info->standing ? XR_REFERENCE_SPACE_TYPE_STAGE : XR_REFERENCE_SPACE_TYPE_LOCAL;
	if (xrCreateReferenceSpace(vr.session, &space, &vr.play_space) != XR_SUCCESS)
	{
		space.referenceSpaceType = XR_REFERENCE_SPACE_TYPE_LOCAL;
		if (!check(xrCreateReferenceSpace(vr.session, &space, &vr.play_space), "xrCreateReferenceSpace (play)"))
			return 0;
	}
	if (!actions_create())
		return 0;
	info->refresh_rate = 0.0f;
	if (vr.has_refresh_rate)
	{
		PFN_xrRequestDisplayRefreshRateFB request = NULL;
		PFN_xrGetDisplayRefreshRateFB current = NULL;

		get_instance_proc_address(vr.instance, "xrRequestDisplayRefreshRateFB", (PFN_xrVoidFunction *)&request);
		get_instance_proc_address(vr.instance, "xrGetDisplayRefreshRateFB", (PFN_xrVoidFunction *)&current);
		/* the rate asked for (vr.refresh_rate), which the runtime may round
		to a mode of the display or refuse */
		if (request && info->refresh_rate_wanted > 0.0f)
		{
			XrResult result = request(vr.session, info->refresh_rate_wanted);

			if (result != XR_SUCCESS)
				host_logf(HOST_LOG_WARN, "vr: %.0f Hz refused: %s", info->refresh_rate_wanted, result_name(result));
		}
		if (current)
			current(vr.session, &info->refresh_rate);
	}
	host_logf(HOST_LOG_INFO, "vr: %s, eyes %dx%d (recommended %dx%d), HUD %dx%d, %s space", info->system_name,
		info->eye_width, info->eye_height, info->recommended_width, info->recommended_height,
		info->hud_width, info->hud_height,
		space.referenceSpaceType == XR_REFERENCE_SPACE_TYPE_STAGE ? "stage" : "local");
	return 1;
}

static void events_poll(void)
{
	XrEventDataBuffer event = { XR_TYPE_EVENT_DATA_BUFFER };

	while (xrPollEvent(vr.instance, &event) == XR_SUCCESS)
	{
		if (event.type == XR_TYPE_EVENT_DATA_SESSION_STATE_CHANGED)
		{
			const XrEventDataSessionStateChanged *changed = (const XrEventDataSessionStateChanged *)&event;

			vr.state = changed->state;
			host_logf(HOST_LOG_INFO, "vr: session state %d", (int)vr.state);
			if (vr.state == XR_SESSION_STATE_READY)
			{
				XrSessionBeginInfo begin = { XR_TYPE_SESSION_BEGIN_INFO };

				begin.primaryViewConfigurationType = XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO;
				vr.running = check(xrBeginSession(vr.session, &begin), "xrBeginSession");
			}
			else if (vr.state == XR_SESSION_STATE_STOPPING)
			{
				xrEndSession(vr.session);
				vr.running = 0;
			}
			else if (vr.state == XR_SESSION_STATE_EXITING || vr.state == XR_SESSION_STATE_LOSS_PENDING)
			{
				/* the runtime closes the application (quit from its dashboard,
				or it is going away): so does the game, as other VR games do */
				host_logf(HOST_LOG_INFO, "vr: the runtime ends the session; quitting");
				host_exit(0);
			}
		}
		else if (event.type == XR_TYPE_EVENT_DATA_INTERACTION_PROFILE_CHANGED)
		{
			int hand;

			for (hand = 0; hand < 2; hand++)
			{
				XrInteractionProfileState state = { XR_TYPE_INTERACTION_PROFILE_STATE };
				PFN_xrGetCurrentInteractionProfile get = NULL;
				PFN_xrPathToString to_string = NULL;
				char name[XR_MAX_PATH_LENGTH] = "";
				uint32_t length = 0;

				get_instance_proc_address(vr.instance, "xrGetCurrentInteractionProfile", (PFN_xrVoidFunction *)&get);
				get_instance_proc_address(vr.instance, "xrPathToString", (PFN_xrVoidFunction *)&to_string);
				if (get && to_string && get(vr.session, vr.hands[hand], &state) == XR_SUCCESS && state.interactionProfile)
					to_string(vr.instance, state.interactionProfile, sizeof(name), &length, name);
				host_logf(HOST_LOG_INFO, "vr: %s hand: %s", hand ? "right" : "left", name[0] ? name : "none");
			}
		}
		event.type = XR_TYPE_EVENT_DATA_BUFFER;
	}
}

int host_vr_frame_wait(struct vr_host_views *views)
{
	XrFrameWaitInfo wait = { XR_TYPE_FRAME_WAIT_INFO };
	XrFrameBeginInfo begin = { XR_TYPE_FRAME_BEGIN_INFO };

	views->should_render = 0;
	if (!vr.session)
		return 0;
	events_poll();
	if (!vr.running)
	{
		/* (until the session is ready: the game goes on without frames) */
		struct timespec pause = { 0, 5000000 };

		nanosleep(&pause, NULL);
		return 0;
	}
	if (vr.frame_begun)
		return 1;
	vr.frame.type = XR_TYPE_FRAME_STATE;
	vr.frame.next = NULL;
	if (!check(xrWaitFrame(vr.session, &wait, &vr.frame), "xrWaitFrame"))
		return 0;
	if (!check(xrBeginFrame(vr.session, &begin), "xrBeginFrame"))
		return 0;
	vr.frame_begun = 1;
	views->should_render = vr.frame.shouldRender ? 1 : 0;
	views->focused = vr.state == XR_SESSION_STATE_FOCUSED;
	views->display_period = (float)(vr.frame.predictedDisplayPeriod * 1e-9);
	views->display_elapsed = vr.last_display_time ?
		(float)((vr.frame.predictedDisplayTime - vr.last_display_time) * 1e-9) : 0.0f;
	vr.last_display_time = vr.frame.predictedDisplayTime;
	return 1;
}

static void pose_from_xr(const XrPosef *xr, int valid, struct vr_host_pose *pose)
{
	pose->position[0] = xr->position.x;
	pose->position[1] = xr->position.y;
	pose->position[2] = xr->position.z;
	pose->orientation[0] = xr->orientation.x;
	pose->orientation[1] = xr->orientation.y;
	pose->orientation[2] = xr->orientation.z;
	pose->orientation[3] = xr->orientation.w;
	pose->valid = valid;
}

int host_vr_locate(struct vr_host_views *views)
{
	XrViewLocateInfo locate = { XR_TYPE_VIEW_LOCATE_INFO };
	XrViewState state = { XR_TYPE_VIEW_STATE };
	XrView located[2] = { { XR_TYPE_VIEW }, { XR_TYPE_VIEW } };
	XrSpaceLocation location = { XR_TYPE_SPACE_LOCATION };
	uint32_t count = 0;
	int eye, hand, valid;

	if (!vr.frame_begun)
		return 0;
	locate.viewConfigurationType = XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO;
	locate.displayTime = vr.frame.predictedDisplayTime;
	locate.space = vr.play_space;
	if (xrLocateViews(vr.session, &locate, &state, 2, &count, located) != XR_SUCCESS || count != 2)
		return 0;
	valid = (state.viewStateFlags & XR_VIEW_STATE_ORIENTATION_VALID_BIT) != 0;
	for (eye = 0; eye < 2; eye++)
	{
		pose_from_xr(&located[eye].pose, valid, &views->eye[eye]);
		views->fov[eye][0] = tanf(located[eye].fov.angleLeft);
		views->fov[eye][1] = tanf(located[eye].fov.angleRight);
		views->fov[eye][2] = tanf(located[eye].fov.angleUp);
		views->fov[eye][3] = tanf(located[eye].fov.angleDown);
	}
	if (xrLocateSpace(vr.view_space, vr.play_space, vr.frame.predictedDisplayTime, &location) == XR_SUCCESS)
		pose_from_xr(&location.pose, (location.locationFlags & XR_SPACE_LOCATION_ORIENTATION_VALID_BIT) != 0, &views->head);
	for (hand = 0; hand < 2; hand++)
	{
		location.type = XR_TYPE_SPACE_LOCATION;
		location.next = NULL;
		if (xrLocateSpace(vr.aim_spaces[hand], vr.play_space, vr.frame.predictedDisplayTime, &location) == XR_SUCCESS)
			pose_from_xr(&location.pose, (location.locationFlags & XR_SPACE_LOCATION_ORIENTATION_VALID_BIT) != 0,
				&views->aim[hand]);
		location.type = XR_TYPE_SPACE_LOCATION;
		if (xrLocateSpace(vr.grip_spaces[hand], vr.play_space, vr.frame.predictedDisplayTime, &location) == XR_SUCCESS)
			pose_from_xr(&location.pose, (location.locationFlags & XR_SPACE_LOCATION_ORIENTATION_VALID_BIT) != 0,
				&views->grip[hand]);
	}
	return 1;
}

static int button(int action)
{
	XrActionStateGetInfo get = { XR_TYPE_ACTION_STATE_GET_INFO };
	XrActionStateBoolean state = { XR_TYPE_ACTION_STATE_BOOLEAN };

	get.action = vr.actions[action];
	return xrGetActionStateBoolean(vr.session, &get, &state) == XR_SUCCESS && state.isActive && state.currentState;
}

static float axis(int action)
{
	XrActionStateGetInfo get = { XR_TYPE_ACTION_STATE_GET_INFO };
	XrActionStateFloat state = { XR_TYPE_ACTION_STATE_FLOAT };

	get.action = vr.actions[action];
	return xrGetActionStateFloat(vr.session, &get, &state) == XR_SUCCESS && state.isActive ? state.currentState : 0.0f;
}

static void stick(int action, float *value)
{
	XrActionStateGetInfo get = { XR_TYPE_ACTION_STATE_GET_INFO };
	XrActionStateVector2f state = { XR_TYPE_ACTION_STATE_VECTOR2F };

	get.action = vr.actions[action];
	value[0] = value[1] = 0.0f;
	if (xrGetActionStateVector2f(vr.session, &get, &state) == XR_SUCCESS && state.isActive)
	{
		value[0] = state.currentState.x;
		value[1] = state.currentState.y;
	}
}

int host_vr_input(struct vr_host_input *input)
{
	XrActiveActionSet active = { 0 };
	XrActionsSyncInfo sync = { XR_TYPE_ACTIONS_SYNC_INFO };
	int index, hand;

	memset(input, 0, sizeof(*input));
	if (!vr.running || vr.state != XR_SESSION_STATE_FOCUSED)
		return 0;
	active.actionSet = vr.action_set;
	sync.countActiveActionSets = 1;
	sync.activeActionSets = &active;
	if (xrSyncActions(vr.session, &sync) != XR_SUCCESS)
		return 0;
	for (index = 0; index < NUMBER_OF_BUTTON_ACTIONS; index++)
	{
		if (button(index))
			input->buttons |= 1u << index;
	}
	stick(_action_left_stick, input->left_stick);
	stick(_action_right_stick, input->right_stick);
	input->left_trigger = axis(_action_left_trigger);
	input->right_trigger = axis(_action_right_trigger);
	input->left_grip = axis(_action_left_grip);
	input->right_grip = axis(_action_right_grip);
	for (hand = 0; hand < 2; hand++)
	{
		XrActionStateGetInfo get = { XR_TYPE_ACTION_STATE_GET_INFO };
		XrActionStatePose state = { XR_TYPE_ACTION_STATE_POSE };

		get.action = vr.actions[_action_aim_pose];
		get.subactionPath = vr.hands[hand];
		input->active[hand] = xrGetActionStatePose(vr.session, &get, &state) == XR_SUCCESS && state.isActive;
	}
	return 1;
}

int host_vr_acquire(int swapchain)
{
	XrSwapchainImageAcquireInfo acquire = { XR_TYPE_SWAPCHAIN_IMAGE_ACQUIRE_INFO };
	XrSwapchainImageWaitInfo wait = { XR_TYPE_SWAPCHAIN_IMAGE_WAIT_INFO };
	uint32_t index;

	if (swapchain < 0 || swapchain >= VR_SWAPCHAIN_COUNT || !vr.swapchains[swapchain])
		return -1;
	if (vr.acquired[swapchain] >= 0)
		return vr.acquired[swapchain];
	if (timing.enabled < 0)
		timing.enabled = getenv("HALO_VR_TIMING") != NULL;
	if (timing.enabled)
		timing.acquire -= seconds_now();
	if (!check(xrAcquireSwapchainImage(vr.swapchains[swapchain], &acquire, &index), "xrAcquireSwapchainImage"))
		return -1;
	wait.timeout = 1000000000; /* (a second: a lost runtime must not hang the game) */
	if (!check(xrWaitSwapchainImage(vr.swapchains[swapchain], &wait), "xrWaitSwapchainImage"))
		return -1;
	vr.acquired[swapchain] = (int)index;
	if (timing.enabled)
		timing.acquire += seconds_now();
	return (int)index;
}

void host_vr_release(int swapchain)
{
	XrSwapchainImageReleaseInfo release = { XR_TYPE_SWAPCHAIN_IMAGE_RELEASE_INFO };

	if (swapchain < 0 || swapchain >= VR_SWAPCHAIN_COUNT || !vr.swapchains[swapchain] || vr.acquired[swapchain] < 0)
		return;
	xrReleaseSwapchainImage(vr.swapchains[swapchain], &release);
	vr.acquired[swapchain] = -1;
}

static void pose_to_xr(const struct vr_host_pose *pose, XrPosef *xr)
{
	xr->position.x = pose->position[0];
	xr->position.y = pose->position[1];
	xr->position.z = pose->position[2];
	xr->orientation.x = pose->orientation[0];
	xr->orientation.y = pose->orientation[1];
	xr->orientation.z = pose->orientation[2];
	xr->orientation.w = pose->orientation[3];
}

void host_vr_frame_end(const struct vr_host_layers *layers)
{
	XrFrameEndInfo end = { XR_TYPE_FRAME_END_INFO };
	XrCompositionLayerProjection projection = { XR_TYPE_COMPOSITION_LAYER_PROJECTION };
	XrCompositionLayerProjectionView views[2];
	XrCompositionLayerDepthInfoKHR depths[2];
	XrCompositionLayerQuad quad = { XR_TYPE_COMPOSITION_LAYER_QUAD };
	const XrCompositionLayerBaseHeader *submitted[2];
	uint32_t count = 0;
	int eye;

	if (!vr.frame_begun)
		return;
	if (timing.enabled > 0)
		timing.release -= seconds_now();
	host_vr_release(VR_SWAPCHAIN_EYES);
	host_vr_release(VR_SWAPCHAIN_HUD);
	host_vr_release(VR_SWAPCHAIN_DEPTH);
	if (timing.enabled > 0)
		timing.release += seconds_now();
	if (layers && layers->projection)
	{
		for (eye = 0; eye < 2; eye++)
		{
			memset(&views[eye], 0, sizeof(views[eye]));
			views[eye].type = XR_TYPE_COMPOSITION_LAYER_PROJECTION_VIEW;
			pose_to_xr(&layers->eye[eye], &views[eye].pose);
			views[eye].fov.angleLeft = atanf(layers->fov[eye][0]);
			views[eye].fov.angleRight = atanf(layers->fov[eye][1]);
			views[eye].fov.angleUp = atanf(layers->fov[eye][2]);
			views[eye].fov.angleDown = atanf(layers->fov[eye][3]);
			views[eye].subImage.swapchain = vr.swapchains[VR_SWAPCHAIN_EYES];
			views[eye].subImage.imageRect.extent.width = vr.swapchain_width[VR_SWAPCHAIN_EYES];
			views[eye].subImage.imageRect.extent.height = vr.swapchain_height[VR_SWAPCHAIN_EYES];
			views[eye].subImage.imageArrayIndex = (uint32_t)eye;
			if (layers->depth && vr.swapchains[VR_SWAPCHAIN_DEPTH])
			{
				/* the eyes' depth, for the compositor's reprojection */
				memset(&depths[eye], 0, sizeof(depths[eye]));
				depths[eye].type = XR_TYPE_COMPOSITION_LAYER_DEPTH_INFO_KHR;
				depths[eye].subImage = views[eye].subImage;
				depths[eye].subImage.swapchain = vr.swapchains[VR_SWAPCHAIN_DEPTH];
				depths[eye].minDepth = 0.0f;
				depths[eye].maxDepth = 1.0f;
				depths[eye].nearZ = layers->near_z;
				depths[eye].farZ = layers->far_z;
				views[eye].next = &depths[eye];
			}
		}
		projection.space = vr.play_space;
		projection.viewCount = 2;
		projection.views = views;
		submitted[count++] = (const XrCompositionLayerBaseHeader *)&projection;
	}
	if (layers && layers->hud)
	{
		/* the game's HUD is drawn over the cleared image with its colour
		already multiplied by its coverage, as the compositor's default
		(premultiplied) blending expects */
		quad.layerFlags = XR_COMPOSITION_LAYER_BLEND_TEXTURE_SOURCE_ALPHA_BIT;
		quad.space = layers->hud_head_locked ? vr.view_space : vr.play_space;
		quad.eyeVisibility = XR_EYE_VISIBILITY_BOTH;
		quad.subImage.swapchain = vr.swapchains[VR_SWAPCHAIN_HUD];
		quad.subImage.imageRect.extent.width = vr.swapchain_width[VR_SWAPCHAIN_HUD];
		quad.subImage.imageRect.extent.height = vr.swapchain_height[VR_SWAPCHAIN_HUD];
		pose_to_xr(&layers->hud_pose, &quad.pose);
		quad.size.width = layers->hud_size[0];
		quad.size.height = layers->hud_size[1];
		submitted[count++] = (const XrCompositionLayerBaseHeader *)&quad;
	}
	end.displayTime = vr.frame.predictedDisplayTime;
	end.environmentBlendMode = XR_ENVIRONMENT_BLEND_MODE_OPAQUE;
	end.layerCount = count;
	end.layers = submitted;
	if (timing.enabled > 0)
		timing.end -= seconds_now();
	check(xrEndFrame(vr.session, &end), "xrEndFrame");
	if (timing.enabled > 0)
	{
		timing.end += seconds_now();
		timing_frame();
	}
	vr.frame_begun = 0;
}

void host_vr_haptic(int hand, float amplitude, float seconds)
{
	XrHapticActionInfo info = { XR_TYPE_HAPTIC_ACTION_INFO };
	XrHapticVibration vibration = { XR_TYPE_HAPTIC_VIBRATION };

	if (!vr.running || hand < 0 || hand > 1)
		return;
	info.action = vr.actions[_action_haptic];
	info.subactionPath = vr.hands[hand];
	vibration.amplitude = amplitude;
	vibration.duration = (XrDuration)(seconds * 1e9f);
	vibration.frequency = XR_FREQUENCY_UNSPECIFIED;
	xrApplyHapticFeedback(vr.session, &info, (const XrHapticBaseHeader *)&vibration);
}
