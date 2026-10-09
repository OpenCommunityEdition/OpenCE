/*
HOST_VK.C

The Vulkan renderer's host half: Vulkan is
brought up as far as an instance and a physical device, and from that the host
decides whether the Vulkan image can run on this device or the GL ES image must.
The logical device, the swapchain and the command stream are
host_vk_render.c's and host_vk_present.c's.

What is kept for the backend is in host_vk.h. The driver is opened through
host_vk_driver.c (the phone's own, or a driver archive loaded with
libadrenotools), so no SDL_Vulkan_* call is made anywhere: SDL would load the
phone's driver beside the chosen one.
*/

#include "host.h"
#include "host_vk.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct host_vk host_vk;
int host_vk_presenting;

static unsigned validation_errors;

unsigned host_vk_validation_errors(void)
{
	return __atomic_load_n(&validation_errors, __ATOMIC_RELAXED);
}

static const char *result_name(VkResult result)
{
	switch (result)
	{
	case VK_SUCCESS: return "VK_SUCCESS";
	case VK_INCOMPLETE: return "VK_INCOMPLETE";
	case VK_ERROR_OUT_OF_HOST_MEMORY: return "VK_ERROR_OUT_OF_HOST_MEMORY";
	case VK_ERROR_OUT_OF_DEVICE_MEMORY: return "VK_ERROR_OUT_OF_DEVICE_MEMORY";
	case VK_ERROR_INITIALIZATION_FAILED: return "VK_ERROR_INITIALIZATION_FAILED";
	case VK_ERROR_LAYER_NOT_PRESENT: return "VK_ERROR_LAYER_NOT_PRESENT";
	case VK_ERROR_EXTENSION_NOT_PRESENT: return "VK_ERROR_EXTENSION_NOT_PRESENT";
	case VK_ERROR_INCOMPATIBLE_DRIVER: return "VK_ERROR_INCOMPATIBLE_DRIVER";
	default: return "an error";
	}
}

/* the validation layer's messages go to the log (tag halo, prefix "vk:"), and
errors are counted for the diagnostics of the later phases */
static VKAPI_ATTR VkBool32 VKAPI_CALL debug_callback(VkDebugUtilsMessageSeverityFlagBitsEXT severity,
	VkDebugUtilsMessageTypeFlagsEXT types, const VkDebugUtilsMessengerCallbackDataEXT *data, void *user)
{
	int priority = HOST_LOG_INFO;

	(void)user;
	if (severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
	{
		priority = HOST_LOG_ERROR;
		__atomic_add_fetch(&validation_errors, 1, __ATOMIC_RELAXED);
	}
	else if (severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
	{
		priority = HOST_LOG_WARN;
	}
	/* the swapchain asks for the identity transform on purpose (the compositor turns the picture: host_vk_present.c),
	and the layer's performance warning about it is expected */
	if (data->pMessageIdName && !strcmp(data->pMessageIdName, "WARNING-Swapchain-PreTransform"))
	{
		host_logf(HOST_LOG_INFO, "vk: [expected, performance] %s: %s", data->pMessageIdName, data->pMessage ? data->pMessage : "");
		return VK_FALSE;
	}
	host_logf(priority, "vk: [%s%s] %s: %s",
		severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT ? "error" :
		severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT ? "warning" :
		severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT ? "info" : "verbose",
		types & VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT ? ", performance" : "",
		data->pMessageIdName ? data->pMessageIdName : "-", data->pMessage ? data->pMessage : "");
	return VK_FALSE;
}

/* ---------- the startup */

/* destroys what was made and records why: the reason is the text of the log line */
static int fail(char *line, size_t size, const char *format, ...) __attribute__((format(printf, 3, 4)));

static int fail(char *line, size_t size, const char *format, ...)
{
	char reason[480];
	va_list arguments;

	va_start(arguments, format);
	vsnprintf(reason, sizeof(reason), format, arguments);
	va_end(arguments);
	if (host_vk.messenger && host_vk.vkDestroyDebugUtilsMessengerEXT)
		host_vk.vkDestroyDebugUtilsMessengerEXT(host_vk.instance, host_vk.messenger, NULL);
	if (host_vk.instance && host_vk.vkDestroyInstance)
		host_vk.vkDestroyInstance(host_vk.instance, NULL);
	free(host_vk.extensions);
	memset(&host_vk, 0, sizeof(host_vk));
	snprintf(line, size, "GL ES (Vulkan was asked for: %s)", reason);
	return 0;
}

static int has_name(const VkExtensionProperties *list, uint32_t count, const char *name)
{
	uint32_t index;

	for (index = 0; index < count; index++)
	{
		if (!strcmp(list[index].extensionName, name))
			return 1;
	}
	return 0;
}

static int has_device_extension(const char *name)
{
	uint32_t index;

	for (index = 0; index < host_vk.extension_count; index++)
	{
		if (!strcmp(host_vk.extensions[index], name))
			return 1;
	}
	return 0;
}

/* the instance functions, through the instance; a missing one is logged by
name. Where the instance is 1.0-level, properties2 is the extension's. */
static int load_instance_functions(void)
{
	int missing = 0;

#define X(name) \
	host_vk.name = (PFN_##name)host_vk.get_instance_proc(host_vk.instance, #name); \
	if (!host_vk.name && (!strcmp(#name, "vkGetPhysicalDeviceFeatures2") || !strcmp(#name, "vkGetPhysicalDeviceProperties2"))) \
		host_vk.name = (PFN_##name)host_vk.get_instance_proc(host_vk.instance, #name "KHR"); \
	if (!host_vk.name) \
	{ \
		host_logf(HOST_LOG_ERROR, "vk: the instance has no %s", #name); \
		missing++; \
	}
	HOST_VK_INSTANCE_FUNCTIONS(X)
#undef X
	return !missing;
}

static void version_text(uint32_t version, char *text, size_t size)
{
	snprintf(text, size, "%u.%u.%u", VK_VERSION_MAJOR(version), VK_VERSION_MINOR(version), VK_VERSION_PATCH(version));
}

/* a physical device for the renderer: a graphics queue family, VK_KHR_swapchain
and dynamic rendering (Vulkan 1.3's feature, or VK_KHR_dynamic_rendering with
what it depends on before 1.2). Returns 1 and fills the state, or 0 and says
what the device lacked in why. */
static int device_usable(VkPhysicalDevice device, char *why, size_t why_size)
{
	VkPhysicalDeviceProperties properties;
	VkQueueFamilyProperties *families;
	VkExtensionProperties *available = NULL;
	uint32_t family_count = 0, count = 0, family, queue_family = 0, api, index;
	VkPhysicalDeviceFeatures2 features = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2 };
	VkPhysicalDeviceVulkan13Features features13 = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES };
	VkPhysicalDeviceDynamicRenderingFeaturesKHR dynamic_rendering =
		{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_FEATURES_KHR };
	int graphics = 0, swapchain, dynamic = 0;
	VkResult result;

	host_vk.vkGetPhysicalDeviceProperties(device, &properties);
	api = properties.apiVersion < host_vk.instance_api ? properties.apiVersion : host_vk.instance_api;

	host_vk.vkGetPhysicalDeviceQueueFamilyProperties(device, &family_count, NULL);
	families = malloc(sizeof(*families) * (family_count ? family_count : 1));
	if (!families)
	{
		snprintf(why, why_size, "out of memory");
		return 0;
	}
	host_vk.vkGetPhysicalDeviceQueueFamilyProperties(device, &family_count, families);
	for (family = 0; family < family_count; family++)
	{
		if ((families[family].queueFlags & VK_QUEUE_GRAPHICS_BIT) && families[family].queueCount > 0)
		{
			graphics = 1;
			queue_family = family;
			break;
		}
	}
	free(families);
	if (!graphics)
	{
		snprintf(why, why_size, "%s has no graphics queue family", properties.deviceName);
		return 0;
	}

	do
	{
		free(available);
		count = 0;
		result = host_vk.vkEnumerateDeviceExtensionProperties(device, NULL, &count, NULL);
		available = malloc(sizeof(*available) * (count ? count : 1));
		if (result == VK_SUCCESS && available)
			result = host_vk.vkEnumerateDeviceExtensionProperties(device, NULL, &count, available);
	}
	while (result == VK_INCOMPLETE);
	if (result != VK_SUCCESS || !available)
	{
		free(available);
		snprintf(why, why_size, "%s: vkEnumerateDeviceExtensionProperties failed (%d, %s)", properties.deviceName,
			(int)result, result_name(result));
		return 0;
	}
	swapchain = has_name(available, count, VK_KHR_SWAPCHAIN_EXTENSION_NAME);
	if (!swapchain)
	{
		free(available);
		snprintf(why, why_size, "%s lacks %s", properties.deviceName, VK_KHR_SWAPCHAIN_EXTENSION_NAME);
		return 0;
	}

	/* dynamic rendering: the core feature from 1.3; before it the extension,
	which needs depth/stencil resolve (core in 1.2) and so renderpass2 (core in
	1.2), and before 1.1 multiview and maintenance2 */
	if (api >= VK_API_VERSION_1_3)
	{
		features.pNext = &features13;
	}
	else
	{
		static const char *const needs[] = { VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME, VK_KHR_DEPTH_STENCIL_RESOLVE_EXTENSION_NAME,
			VK_KHR_CREATE_RENDERPASS_2_EXTENSION_NAME, VK_KHR_MULTIVIEW_EXTENSION_NAME, VK_KHR_MAINTENANCE_2_EXTENSION_NAME };
		uint32_t wanted = api >= VK_API_VERSION_1_2 ? 1 : api >= VK_API_VERSION_1_1 ? 3 : 5;

		for (index = 0; index < wanted; index++)
		{
			if (!has_name(available, count, needs[index]))
			{
				snprintf(why, why_size, "%s (Vulkan %u.%u) lacks %s, for dynamic rendering", properties.deviceName,
					VK_VERSION_MAJOR(api), VK_VERSION_MINOR(api), needs[index]);
				free(available);
				return 0;
			}
		}
		features.pNext = &dynamic_rendering;
	}
	host_vk.vkGetPhysicalDeviceFeatures2(device, &features);
	dynamic = api >= VK_API_VERSION_1_3 ? features13.dynamicRendering : dynamic_rendering.dynamicRendering;
	if (!dynamic)
	{
		snprintf(why, why_size, "%s does not support dynamic rendering (Vulkan %u.%u)", properties.deviceName,
			VK_VERSION_MAJOR(api), VK_VERSION_MINOR(api));
		free(available);
		return 0;
	}

	/* it will do: keep what the backend needs of it */
	host_vk.physical = device;
	host_vk.queue_family = queue_family;
	host_vk.properties = properties;
	host_vk.api = api;
	host_vk.extension_count = count;
	host_vk.extensions = calloc(count ? count : 1, VK_MAX_EXTENSION_NAME_SIZE);
	if (!host_vk.extensions)
	{
		free(available);
		host_vk.physical = VK_NULL_HANDLE;
		snprintf(why, why_size, "out of memory");
		return 0;
	}
	for (index = 0; index < count; index++)
		snprintf(host_vk.extensions[index], VK_MAX_EXTENSION_NAME_SIZE, "%s", available[index].extensionName);
	free(available);
	return 1;
}

int host_vk_startup(const char *vk_driver, int validation, char *line, size_t size)
{
	char description[400], check[300], why[400] = "", versions[32], device_api[32];
	uint32_t loader_version = VK_API_VERSION_1_0, count = 0, index;
	PFN_vkEnumerateInstanceVersion enumerate_version;
	PFN_vkEnumerateInstanceExtensionProperties enumerate_extensions;
	PFN_vkEnumerateInstanceLayerProperties enumerate_layers;
	PFN_vkCreateInstance create_instance;
	VkExtensionProperties *available = NULL;
	VkLayerProperties *layers = NULL;
	const char *extensions[8];
	uint32_t extension_count = 0;
	const char *layer_name = "VK_LAYER_KHRONOS_validation";
	int layer_found = 0, debug_utils = 0;
	VkApplicationInfo application = { VK_STRUCTURE_TYPE_APPLICATION_INFO };
	VkInstanceCreateInfo info = { VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO };
	VkPhysicalDevice *devices = NULL;
	VkResult result;

	memset(&host_vk, 0, sizeof(host_vk));

	/* 1. the driver */
	host_vk.get_instance_proc = host_vk_driver_open(vk_driver ? vk_driver : "", description, sizeof(description));
	if (!host_vk.get_instance_proc)
		return fail(line, size, "no Vulkan driver could be opened (%s)", description);

	create_instance = (PFN_vkCreateInstance)host_vk.get_instance_proc(NULL, "vkCreateInstance");
	enumerate_version = (PFN_vkEnumerateInstanceVersion)host_vk.get_instance_proc(NULL, "vkEnumerateInstanceVersion");
	enumerate_extensions = (PFN_vkEnumerateInstanceExtensionProperties)
		host_vk.get_instance_proc(NULL, "vkEnumerateInstanceExtensionProperties");
	enumerate_layers = (PFN_vkEnumerateInstanceLayerProperties)
		host_vk.get_instance_proc(NULL, "vkEnumerateInstanceLayerProperties");
	if (!create_instance || !enumerate_extensions || !enumerate_layers)
		return fail(line, size, "the Vulkan loader lacks %s (%s)",
			!create_instance ? "vkCreateInstance" : !enumerate_extensions ? "vkEnumerateInstanceExtensionProperties" :
			"vkEnumerateInstanceLayerProperties", description);

	/* 2. an instance: 1.3 if the loader offers it, otherwise what it offers (1.1
	or 1.2; a 1.0 loader gets properties2 as the extension) */
	if (enumerate_version)
		enumerate_version(&loader_version);
	host_vk.instance_api = loader_version < VK_API_VERSION_1_3 ? loader_version : VK_API_VERSION_1_3;
	version_text(host_vk.instance_api, versions, sizeof(versions));

	do
	{
		free(available);
		count = 0;
		result = enumerate_extensions(NULL, &count, NULL);
		available = malloc(sizeof(*available) * (count ? count : 1));
		if (result == VK_SUCCESS && available)
			result = enumerate_extensions(NULL, &count, available);
	}
	while (result == VK_INCOMPLETE);
	if (result != VK_SUCCESS || !available)
	{
		free(available);
		return fail(line, size, "vkEnumerateInstanceExtensionProperties failed (%d, %s)", (int)result, result_name(result));
	}
	if (!has_name(available, count, VK_KHR_SURFACE_EXTENSION_NAME) ||
		!has_name(available, count, VK_KHR_ANDROID_SURFACE_EXTENSION_NAME))
	{
		free(available);
		return fail(line, size, "the instance does not offer %s and %s (%s)", VK_KHR_SURFACE_EXTENSION_NAME,
			VK_KHR_ANDROID_SURFACE_EXTENSION_NAME, description);
	}
	extensions[extension_count++] = VK_KHR_SURFACE_EXTENSION_NAME;
	extensions[extension_count++] = VK_KHR_ANDROID_SURFACE_EXTENSION_NAME;
	if (host_vk.instance_api < VK_API_VERSION_1_1)
	{
		if (!has_name(available, count, VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME))
		{
			free(available);
			return fail(line, size, "a Vulkan %s loader without %s (%s)", versions,
				VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME, description);
		}
		extensions[extension_count++] = VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME;
	}
	debug_utils = has_name(available, count, VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
	free(available);

	if (validation)
	{
		do
		{
			free(layers);
			count = 0;
			result = enumerate_layers(&count, NULL);
			layers = malloc(sizeof(*layers) * (count ? count : 1));
			if (result == VK_SUCCESS && layers)
				result = enumerate_layers(&count, layers);
		}
		while (result == VK_INCOMPLETE);
		if (result == VK_SUCCESS && layers)
		{
			for (index = 0; index < count; index++)
			{
				if (!strcmp(layers[index].layerName, layer_name))
					layer_found = 1;
			}
		}
		free(layers);
		if (layer_found && debug_utils)
		{
			extensions[extension_count++] = VK_EXT_DEBUG_UTILS_EXTENSION_NAME;
			info.enabledLayerCount = 1;
			info.ppEnabledLayerNames = &layer_name;
			host_vk.validation = 1;
		}
		else
		{
			host_logf(HOST_LOG_WARN, "vk: debug.vk_validation is on, but %s is %s", !layer_found ? layer_name : VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
				"not there; the renderer runs without validation");
		}
	}

	application.pApplicationName = "Halo";
	application.apiVersion = host_vk.instance_api;
	info.pApplicationInfo = &application;
	info.enabledExtensionCount = extension_count;
	info.ppEnabledExtensionNames = extensions;
	result = create_instance(&info, NULL, &host_vk.instance);
	if (result != VK_SUCCESS)
	{
		host_vk.instance = VK_NULL_HANDLE;
		return fail(line, size, "vkCreateInstance failed (%d, %s) on %s", (int)result, result_name(result), description);
	}
	if (!load_instance_functions())
		return fail(line, size, "the instance lacks entry points (see the log, prefix \"vk:\")");
	if (host_vk.validation)
	{
		VkDebugUtilsMessengerCreateInfoEXT messenger = { VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT };

		host_vk.vkCreateDebugUtilsMessengerEXT = (PFN_vkCreateDebugUtilsMessengerEXT)
			host_vk.get_instance_proc(host_vk.instance, "vkCreateDebugUtilsMessengerEXT");
		host_vk.vkDestroyDebugUtilsMessengerEXT = (PFN_vkDestroyDebugUtilsMessengerEXT)
			host_vk.get_instance_proc(host_vk.instance, "vkDestroyDebugUtilsMessengerEXT");
		messenger.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT |
			VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT;
		messenger.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
			VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
		messenger.pfnUserCallback = debug_callback;
		if (!host_vk.vkCreateDebugUtilsMessengerEXT || !host_vk.vkDestroyDebugUtilsMessengerEXT ||
			host_vk.vkCreateDebugUtilsMessengerEXT(host_vk.instance, &messenger, NULL, &host_vk.messenger) != VK_SUCCESS)
		{
			host_vk.messenger = VK_NULL_HANDLE;
			host_logf(HOST_LOG_WARN, "vk: no debug messenger; the layer's messages will not be logged");
		}
	}

	/* 3. the driver asked for must be the one that loaded */
	if (!host_vk_driver_verify(check, sizeof(check)))
		return fail(line, size, "%s", check);

	/* 4. a physical device with what the renderer needs: the first that has it */
	count = 0;
	do
	{
		free(devices);
		count = 0;
		result = host_vk.vkEnumeratePhysicalDevices(host_vk.instance, &count, NULL);
		devices = malloc(sizeof(*devices) * (count ? count : 1));
		if (result == VK_SUCCESS && devices)
			result = host_vk.vkEnumeratePhysicalDevices(host_vk.instance, &count, devices);
	}
	while (result == VK_INCOMPLETE);
	if (result != VK_SUCCESS || !devices)
	{
		free(devices);
		return fail(line, size, "vkEnumeratePhysicalDevices failed (%d, %s) on %s", (int)result, result_name(result),
			description);
	}
	if (!count)
	{
		free(devices);
		return fail(line, size, "%s has no physical device", description);
	}
	for (index = 0; index < count; index++)
	{
		char reason[200];

		if (device_usable(devices[index], reason, sizeof(reason)))
			break;
		host_logf(HOST_LOG_INFO, "vk: physical device %u is not usable: %s", index, reason);
		snprintf(why + strlen(why), sizeof(why) - strlen(why), "%s%s", why[0] ? "; " : "", reason);
	}
	free(devices);
	if (index == count)
		return fail(line, size, "no physical device on %s will do: %s", description, why);

	/* 5. the decision: kept for the backend, and the one log line */
	{
		VkPhysicalDeviceProperties2 properties2 = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2 };
		VkPhysicalDeviceDriverProperties driver = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DRIVER_PROPERTIES };

		if (host_vk.api >= VK_API_VERSION_1_2 || has_device_extension(VK_KHR_DRIVER_PROPERTIES_EXTENSION_NAME))
		{
			properties2.pNext = &driver;
			host_vk.vkGetPhysicalDeviceProperties2(host_vk.physical, &properties2);
		}
		host_vk.driver_id = driver.driverID;
		version_text(host_vk.properties.apiVersion, device_api, sizeof(device_api));
		snprintf(line, size, "Vulkan on %s, %s, Vulkan %s, %s %s%s", description, host_vk.properties.deviceName, device_api,
			driver.driverName[0] ? driver.driverName : "(driver name not reported)", driver.driverInfo,
			host_vk.validation ? ", validation on" : "");
		/* a driver's info can run over several lines; the log line is one */
		for (index = 0; line[index]; index++)
		{
			if (line[index] == '\n' || line[index] == '\r')
				line[index] = ' ';
		}
	}
	snprintf(host_vk.line, sizeof(host_vk.line), "%s", line);
	return 1;
}
