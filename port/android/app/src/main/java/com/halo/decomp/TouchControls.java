package com.halo.decomp;

import android.app.AlertDialog;
import android.content.Context;
import android.content.SharedPreferences;
import android.content.pm.PackageManager;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Paint;
import android.graphics.Path;
import android.graphics.RectF;
import android.graphics.drawable.GradientDrawable;
import android.graphics.drawable.StateListDrawable;
import android.hardware.Sensor;
import android.hardware.SensorEvent;
import android.hardware.SensorEventListener;
import android.hardware.SensorManager;
import android.hardware.input.InputManager;
import android.media.AudioAttributes;
import android.os.Build;
import android.os.SystemClock;
import android.os.VibrationEffect;
import android.os.Vibrator;
import android.os.VibratorManager;
import android.util.SparseArray;
import android.util.SparseIntArray;
import android.view.HapticFeedbackConstants;
import android.view.InputDevice;
import android.view.MotionEvent;
import android.view.View;
import android.view.WindowManager;
import android.widget.Button;
import android.widget.LinearLayout;
import android.widget.ScrollView;
import android.widget.SeekBar;
import android.widget.Switch;
import android.widget.TextView;
import android.widget.Toast;

import java.util.Arrays;
import java.util.Locale;

/**
 * The on-screen touch controls: a multitouch overlay over SDL's surface,
 * with a stick, the controller's buttons, swipe (and optionally gyroscope)
 * aiming and an editor for the layout (TouchLayout). Its state goes to the
 * game through port/android/host/host_touch.c, as port 0's controller.
 *
 * It shows only in a game, never at the main menu, over a menu (which
 * takes taps itself, port/linux/src/touch_input.c) or during a cinematic,
 * only on a touchscreen, and with input.touch_controls = "auto" only while
 * no controller is connected. While it is not shown it lets every finger
 * through to the game.
 */
public final class TouchControls extends View implements SensorEventListener, InputManager.InputDeviceListener {
    /** what a finger holds besides a control: the view swipe */
    private static final int LOOK = -5;
    private static final int NOTHING = Integer.MIN_VALUE;
    /** the toolbar's items (at the top of the screen) */
    private static final int TOOLBAR_TOGGLE = -3;
    private static final int TOOLBAR_OPTIONS = -4;
    private static final int TOOLBAR_EXPORT = -6;
    private static final int TOOLBAR_IMPORT = -7;
    private static final int VEHICLE_ENTER_BUTTON = -8;
    private Bitmap vehicleWheelIconBitmap;
    private static final int VEHICLE_DRIVER = 1;
    private static final int VEHICLE_GUNNER = 2;
    private static final int VEHICLE_PASSENGER = 3;
    private static final float TOOLBAR_RADIUS = 28;
    private static final float OPTIONS_RADIUS = 34;
    private Bitmap jumpIconBitmap;
    private Bitmap fireIconBitmap;
    private Bitmap meleeIconBitmap;
    private Bitmap grenadeIconBitmap;

    private Bitmap crouchIconBitmap;

    /** the logical grid the controls are laid out on, scaled to the display's height */
    private static final float GRID_WIDTH = 960;
    private static final float GRID_HEIGHT = 540;
    /** a control is never smaller than a finger: 24 dp of radius, 48 across */
    private static final float MINIMUM_RADIUS_DP = 24;
    /** a finger moves this far (logical units) before a press becomes a swipe */
    private static final float SLOP = 10;
    /** the move stick's dead zone and its touch area beyond its circle */
    private static final float STICK_DEAD_ZONE = 0.12f;
    private static final float STICK_REACH = 1.28f;
    private static final float STICK_KNOB_RADIUS = 24;
    // FORWARD_DRAG_PADLOCK_V21: on-foot, forward-only drag-to-lock gesture.
    private static final float AUTO_WALK_HINT_FORWARD = 0.63f;
    private static final float AUTO_WALK_HINT_SIDE = 0.58f;
    private static final float AUTO_WALK_LOCK_DISTANCE = 1.76f;
    private static final float AUTO_WALK_LOCK_RADIUS = 0.35f;
    /** a floating stick goes where a thumb lands in this part of the display (from the lower left) */
    private static final float FLOATING_ZONE_WIDTH = 0.45f;
    private static final float FLOATING_ZONE_TOP = 0.35f;
    /** radians per logical unit of swipe (the mouse's rate: xinput_sdl.c) */
    private static final float LOOK_RADIANS_PER_UNIT = 0.0022f;
    /** a label is at most this many radii wide */
    private static final float LABEL_WIDTH = 1.7f;
    private static final long POLL_MS = 16;
    /** the game is in play this long before the controls show (no flash at a level's first frame) */
    private static final long SHOW_DELAY_MS = 200;
    private static final long VIBRATION_MS = 110;
    private static final long VIBRATION_RENEW_MS = 70;

    /** the game's HALO_TOUCH_SCENE bits (port/linux/src/touch_input.c) */
    private static final int SCENE_KNOWN = 1;
    private static final int SCENE_MENUS = 2;
    private static final int SCENE_ON = 4;
    private static final int SCENE_OFF = 8;


    private static final String PREFERENCES = "touch-layout-v1";
    private static final String KEY_CONFIGURATION = "configuration";
    private static final String KEY_MODERN = "modern-ui";
    private static final String KEY_CLASSIC_CONFIGURATION = "classic-configuration";
    private boolean modernUi;
    private static final String KEY_HIDDEN = "hidden";

    /**
     * a kind of control: the SDL button bit or trigger axis it sends (-1 for
     * none), and the game's controller button it is (port/linux/game/
     * touch_game.c numbers them: input.h's _gamepad_*), which names it
     */
    private static final class Kind {
        final int bit;
        final int trigger;
        final int gamepadButton;
        final String buttonName;
        final boolean faceButton;

        Kind(int bit, int trigger, int gamepadButton, String buttonName, boolean faceButton) {
            this.bit = bit;
            this.trigger = trigger;
            this.gamepadButton = gamepadButton;
            this.buttonName = buttonName;
            this.faceButton = faceButton;
        }
    }

    /** by TouchLayout's kinds; bits follow SDL_GamepadButton, triggers are SDL axes 4 and 5 */
    private static final Kind[] KINDS = {
        new Kind(0, -1, 0, "A", true),
        new Kind(1, -1, 1, "B", true),
        new Kind(2, -1, 2, "X", true),
        new Kind(3, -1, 3, "Y", true),
        new Kind(-1, 5, 7, "RT", false),
        new Kind(-1, 4, 6, "LT", false),
        new Kind(7, -1, 14, "LS", false),
        new Kind(8, -1, 15, "RS", false),
        new Kind(9, -1, 5, "White", false),
        new Kind(10, -1, 4, "Black", false),
        new Kind(6, -1, 12, "Start", false),
        new Kind(4, -1, 13, "Back", false),
        new Kind(11, -1, 8, "Up", false),
        new Kind(12, -1, 9, "Down", false),
        new Kind(13, -1, 10, "Left", false),
        new Kind(14, -1, 11, "Right", false),
        new Kind(-1, -1, -1, "Move", false),
        new Kind(-1, 5, 7, "RT", false),
    };

    /** the game's controls (input_abstraction.c's _game_control_*), as the buttons name them */
    private static final String[] GAME_CONTROLS = {
        "Jump", "Gren. type", "Reload", "Weapon", "Melee", "Light",
        "Grenade", "Fire", "Pause", "Back", "Crouch", "Zoom",
    };
    /** the default profile's mapping, until the game sends the player's own */
    private static final int[] DEFAULT_BINDINGS = {
        0, 4, 2, 3, 1, 5, 6, 7, -1, -1, -1, -1, 8, 9, 10, 11,
    };

    private static native void nativeState(int lx, int ly, int rx, int ry, int lt, int rt, int buttons);
    private static native void nativeLook(float dx, float dy);
    private static native void nativeGyro(float dx, float dy);
    private static native void nativeLookReset();
    private static native int nativeRumble();
    private static native void nativeTouchHudActive(boolean active);
    private static native int nativeScene();
    private static native void nativeLayoutAvailable(boolean available);
    private static native boolean nativeLayoutRequested();
    private static native void nativeLayoutChosen();
    private static native void nativeWeaponSlots(int[] state);
    private static native void nativeWeaponDrawerExpanded(boolean expanded);
    private static native void nativeWeaponDisplayOrder(int[] order);
    private static native void nativeSelectWeaponSlot(int slot);
    private static native int nativeBindings(int[] controls);

    private final Paint paint = new Paint(Paint.ANTI_ALIAS_FLAG);
    private final SharedPreferences preferences;
    private final SensorManager sensors;
    private final Sensor gyroscope;
    private final Vibrator vibrator;
    private final InputManager inputManager;
    private final boolean touchscreenFeature;
    private final GyroscopeAim gyroAim = new GyroscopeAim();
    private final float[] gyroDelta = new float[2];
    private final AudioAttributes rumbleAttributes = new AudioAttributes.Builder()
        .setUsage(AudioAttributes.USAGE_GAME)
        .setContentType(AudioAttributes.CONTENT_TYPE_SONIFICATION)
        .build();

    private TouchLayout layout = new TouchLayout();
    private float sensitivity = 1f;
    /** the controls removed by the toolbar's Hide, until its Touch */
    private boolean hidden;
    private boolean editing;
    /** the option dialogs open (one opens before the last one's dismissal is told) */
    private int openDialogs;

    /** what each finger holds: a control, LOOK, or (in toolbarFingers) a toolbar item */
    private final SparseIntArray owners = new SparseIntArray();
    /** where each finger on a button went down, in pixels: dragged, it swipes */
    private final SparseArray<float[]> buttonTouches = new SparseArray<>();
    /** each finger on a toolbar item: {x, y, item}; the item acts when it lifts there */
    private final SparseArray<float[]> toolbarFingers = new SparseArray<>();
    private final int[] axes = new int[6];
    private int lookPointer = -1;
    private long lookEventTime = -1;
    private float lookX;
    private float lookY;
    /** a floating stick's centre while a thumb holds it (logical units) */
    private boolean stickFloating;
    private float stickX;
    private float stickY;
    // FORWARD_RELEASE_TO_LOCK_V22: reaching lock only prepares it; release confirms.
    private boolean autoWalkLocked;
    private boolean autoWalkHintVisible;
    private boolean autoWalkLockReady;
    private boolean autoWalkHapticDone;
    private int autoWalkHintPointer = -1;
    private int dragPointer = -1;
    private int dragControl = -1;
    private float dragOffsetX;
    private float dragOffsetY;

    /** pixels per logical unit, and the display in logical units */
    private float scale = 1;
    private float logicalWidth = GRID_WIDTH;
    private float logicalHeight = GRID_HEIGHT;
    private int insetRight;
    private int insetTop;

    private int scene;
    private boolean controllerConnected;
    private boolean touchscreen;
    private boolean deviceInputActive;
    /** the controls are on screen: shown() for SHOW_DELAY_MS */
    private boolean showing;
    private long shownSince = -1;
    private boolean gyroRegistered;
    private int lastAmplitude;
    private long lastVibration;
    private final int[] bindings = DEFAULT_BINDINGS.clone();
    private final int[] bindingsRead = new int[bindings.length];
    private int bindingsSerial;
    // [0..9] retain the existing two-weapon protocol and extra-slot occupancy.
    // [10..11] original Halo messaging sprite sequence for slots 3 and 4,
    // [12..13] original icon colors, [14..15] original icon flags.
    // The engine draws those sprites; Java never substitutes placeholder art.
    private final int[] weaponSlots = {-1, -1, -1, -1, -1, -1, -1, -1,
        -1, 0, -1, -1, -1, -1, -1, -1};
    private int lastVehicleRole;
    private static final int WEAPON_DRAWER_TOGGLE = -2;
    private final android.util.SparseIntArray slotFingers = new android.util.SparseIntArray();
    private final android.util.SparseArray<float[]> slotPositions = new android.util.SparseArray<>();
    /** Visible frame index -> actual engine inventory index. Reorders the UI only. */
    private final int[] weaponViewOrder = {0, 1, 2, 3};
    private int lastWeaponOccupancy = -1;
    private boolean weaponDrawerExpanded;

    /** reads what the game says every frame: when to show, the buttons' names, and the rumble */
    private final Runnable poll = new Runnable() {
        @Override
        public void run() {
            if (!deviceInputActive)
                return;
            scene = nativeScene();
            updateShowing();
            readBindings();
            nativeWeaponSlots(weaponSlots);
            // Native weapon silhouettes are independent of Java's slot frames.
            nativeTouchHudActive(modernUi && showing && !hidden && !editing && !vehicleOccupied());
            int seatState = vehicleRole() | (vehicleOccupied() ? 16 : 0);
            if (seatState != lastVehicleRole) {
                lastVehicleRole = seatState;
                // VEHICLE_STICK_PROMPT_V14: drop old combat buttons but keep
                // the thumb already holding Move (driver <-> on foot).
                resetForVehicleSeatChange();
            }
            // Keep the two quick slots populated by REAL weapons. On pickup
            // or drop, promote remaining weapons from the drawer automatically.
            int occupied = physicalWeaponMask();
            if ((scene & SCENE_OFF) != 0 || occupied == 0) {
                if (lastWeaponOccupancy != 0) resetWeaponViewOrder();
                lastWeaponOccupancy = 0;
            } else if (lastWeaponOccupancy != occupied) {
                if (lastWeaponOccupancy <= 0) resetWeaponViewOrder();
                WeaponSlotOrder.compact(weaponViewOrder, occupied);
                lastWeaponOccupancy = occupied;
            }
            if (!hasExtraWeapons())
                weaponDrawerExpanded = false;
            nativeWeaponDisplayOrder(weaponViewOrder);
            nativeWeaponDrawerExpanded(modernUi && showing && !hidden && !editing
                && !vehicleOccupied() && weaponDrawerExpanded && hasExtraWeapons());
            nativeLayoutAvailable(touchscreen && !controllerConnected && (scene & SCENE_OFF) == 0);
            if (nativeLayoutRequested() && openDialogs == 0)
                showInterface(true);
            invalidate();
            updateRumble();
            postDelayed(this, POLL_MS);
        }
    };

    public TouchControls(Context context) {
        super(context);
        sensors = (SensorManager) context.getSystemService(Context.SENSOR_SERVICE);
        gyroscope = sensors == null ? null : sensors.getDefaultSensor(Sensor.TYPE_GYROSCOPE);
        if (Build.VERSION.SDK_INT >= 31) {
            VibratorManager manager = (VibratorManager) context.getSystemService(Context.VIBRATOR_MANAGER_SERVICE);
            vibrator = manager == null ? null : manager.getDefaultVibrator();
        } else {
            vibrator = (Vibrator) context.getSystemService(Context.VIBRATOR_SERVICE);
        }
        inputManager = (InputManager) context.getSystemService(Context.INPUT_SERVICE);
        touchscreenFeature = context.getPackageManager().hasSystemFeature(PackageManager.FEATURE_TOUCHSCREEN);
        preferences = context.getSharedPreferences(PREFERENCES, Context.MODE_PRIVATE);
        modernUi = preferences.getBoolean(KEY_MODERN, preferences.contains(KEY_CONFIGURATION));
        loadLayout(context);
        hidden = preferences.getBoolean(KEY_HIDDEN, false);
        refreshInputDevices();
        setFocusable(false);
        setContentDescription("Halo touch controller");
        setOnApplyWindowInsetsListener((view, insets) -> {
            // (only the toolbar moves: the fingers stay held as the system bars come and go)
            int right = insetRight;
            int top = insetTop;
            insetRight = insets.getSystemWindowInsetRight();
            insetTop = insets.getSystemWindowInsetTop();
            if (insets.getDisplayCutout() != null) {
                insetRight = Math.max(insetRight, insets.getDisplayCutout().getSafeInsetRight());
                insetTop = Math.max(insetTop, insets.getDisplayCutout().getSafeInsetTop());
            }
            if (insetRight != right || insetTop != top)
                invalidate();
            return insets;
        });
    }

    private String configurationKey() {
        return modernUi ? KEY_CONFIGURATION : KEY_CLASSIC_CONFIGURATION;
    }

    private void loadLayout(Context context) {
        layout = new TouchLayout();
        if (!modernUi) layout.resetClassicDefaults();
        sensitivity = 1f;
        String configuration = preferences.getString(configurationKey(), null);
        if (configuration == null)
            return;
        try {
            TouchLayout.Configuration saved = TouchLayout.importConfiguration(configuration);
            layout = saved.layout;
            sensitivity = saved.sensitivity;
        } catch (IllegalArgumentException e) {
            Toast.makeText(context, "The saved touch layout could not be read. Using the default one.",
                Toast.LENGTH_LONG).show();
        }
    }

    // ---------- the activity's lifecycle (HaloActivity)

    public void startDeviceInput() {
        if (deviceInputActive)
            return;
        deviceInputActive = true;
        if (inputManager != null)
            inputManager.registerInputDeviceListener(this, null);
        refreshInputDevices();
        gyroAim.reset();
        updateSensors();
        post(poll);
    }

    public void stopDeviceInput() {
        deviceInputActive = false;
        removeCallbacks(poll);
        if (inputManager != null)
            inputManager.unregisterInputDeviceListener(this);
        updateSensors();
        cancelRumble();
        reset();
    }

    // ---------- the display

    @Override
    protected void onSizeChanged(int width, int height, int oldWidth, int oldHeight) {
        layoutControls();
        requestApplyInsets();
    }

    /** scales the grid to the display (its height, keeping the circles round) */
    private void layoutControls() {
        if (getWidth() <= 0 || getHeight() <= 0)
            return;
        reset();
        float width = getWidth();
        float height = getHeight();
        scale = Math.max(0.01f, Math.min(width / GRID_WIDTH, height / GRID_HEIGHT));
        logicalWidth = width / scale;
        logicalHeight = height / scale;
        layout.bounds(logicalWidth, logicalHeight);
        layout.setMinimumRadius(minimumRadius());
        invalidate();
    }

    /** a finger's width, in logical units (at most TouchLayout.MAX_MINIMUM_RADIUS) */
    private float minimumRadius() {
        float radius = MINIMUM_RADIUS_DP * getResources().getDisplayMetrics().density / scale;
        return Math.min(radius, TouchLayout.MAX_MINIMUM_RADIUS);
    }

    private float logical(float pixels) {
        return pixels / scale;
    }

    private float toolbarY() {
        return Math.max(38, insetTop / scale + 38);
    }

    private float optionsX() {
        return logicalWidth - Math.max(42, insetRight / scale + 42);
    }

    private float toolbarRadius(float radius) {
        return Math.max(radius, minimumRadius());
    }

    // ---------- when the controls show

    /**
     * whether the controls belong on screen: in a game (no menu or
     * cinematic), never without a touchscreen, and with
     * input.touch_controls = "auto" only while no controller is connected
     */
    private boolean shown() {
        if ((scene & SCENE_KNOWN) == 0 || (scene & (SCENE_MENUS | SCENE_OFF)) != 0 || !touchscreen)
            return false;
        return !controllerConnected;
    }

    /** shows the controls once shown() has held for SHOW_DELAY_MS; hides them at once */
    private void updateShowing() {
        boolean show = false;
        if (!shown()) {
            shownSince = -1;
        } else {
            long now = SystemClock.uptimeMillis();
            if (shownSince < 0)
                shownSince = now;
            show = now - shownSince >= SHOW_DELAY_MS;
        }
        if (show != showing) {
            showing = show;
            nativeTouchHudActive(modernUi && show && !hidden && !editing && !vehicleOccupied());
            reset();
            updateSensors();
        }
    }

    /** a game controller: a stick or a hat, not a phone's few gamepad keys */
    private static boolean isController(InputDevice device) {
        if (device == null || device.isVirtual())
            return false;
        if ((device.getSources() & InputDevice.SOURCE_JOYSTICK) != InputDevice.SOURCE_JOYSTICK)
            return false;
        return device.getMotionRange(MotionEvent.AXIS_X, InputDevice.SOURCE_JOYSTICK) != null
            || device.getMotionRange(MotionEvent.AXIS_HAT_X, InputDevice.SOURCE_JOYSTICK) != null;
    }

    private void refreshInputDevices() {
        boolean controller = false;
        boolean touch = touchscreenFeature;
        for (int id : InputDevice.getDeviceIds()) {
            InputDevice device = InputDevice.getDevice(id);
            if (device == null)
                continue;
            if (isController(device))
                controller = true;
            boolean touchSource = (device.getSources() & InputDevice.SOURCE_TOUCHSCREEN) == InputDevice.SOURCE_TOUCHSCREEN;
            if (!device.isVirtual() && touchSource)
                touch = true;
        }
        controllerConnected = controller;
        touchscreen = touch;
        updateShowing();
    }

    @Override
    public void onInputDeviceAdded(int deviceId) {
        refreshInputDevices();
    }

    @Override
    public void onInputDeviceRemoved(int deviceId) {
        refreshInputDevices();
    }

    @Override
    public void onInputDeviceChanged(int deviceId) {
        refreshInputDevices();
    }

    private void setHidden(boolean hidden) {
        reset();
        this.hidden = hidden;
        nativeTouchHudActive(modernUi && showing && !hidden && !editing && !vehicleOccupied());
        preferences.edit().putBoolean(KEY_HIDDEN, hidden).apply();
        updateSensors();
    }

    // ---------- the controller state the game reads

    /** lets go of every finger and sends a released controller */
    public void reset() {
        dragPointer = -1;
        dragControl = -1;
        owners.clear();
        buttonTouches.clear();
        toolbarFingers.clear();
        slotFingers.clear();
        slotPositions.clear();
        weaponDrawerExpanded = false;
        nativeWeaponDrawerExpanded(false);
        lookPointer = -1;
        lookEventTime = -1;
        stickFloating = false;
        autoWalkLocked = false;
        autoWalkHintVisible = false;
        autoWalkLockReady = false;
        autoWalkHapticDone = false;
        autoWalkHintPointer = -1;
        gyroAim.reset();
        nativeLookReset();
        Arrays.fill(axes, 0);
        publish();
        invalidate();
    }

    /** Keep the SAME Android pointer ID for Move when changing seats.
     * reset() alone releases it, and Android cannot send another ACTION_DOWN
     * while the thumb is still on the glass. Other pressed actions must reset.
     * A gunner seat cannot keep the stick since it has no movement control. */
    private void resetForVehicleSeatChange() {
        int movePointer = -1;
        int stick = TouchLayout.LEFT;
        int moveX = axes[0], moveY = axes[1];
        boolean oldFloating = stickFloating;
        float oldStickX = stickX, oldStickY = stickY;
        if (showing && !hidden && !editing && layout.shown(stick)
                && vehicleControlVisible(stick)) {
            for (int i = 0; i < owners.size(); i++) {
                if (owners.valueAt(i) == stick) {
                    movePointer = owners.keyAt(i);
                    break;
                }
            }
        }
        reset();
        if (movePointer >= 0) {
            owners.put(movePointer, stick);
            axes[0] = moveX;
            axes[1] = moveY;
            stickFloating = oldFloating;
            stickX = oldStickX;
            stickY = oldStickY;
            publish();
        }
    }

    private void publish() {
        if (editing || openDialogs > 0) {
            nativeState(0, 0, 0, 0, 0, 0, 0);
            return;
        }
        int bits = 0;
        axes[4] = 0;
        axes[5] = 0;
        for (int i = 0; i < owners.size(); i++) {
            int control = owners.valueAt(i);
            if (control == VEHICLE_ENTER_BUTTON) {
                // X is the default action key, but respect custom bindings.
                for (Kind kind : KINDS) {
                    if (kind.gamepadButton >= 0 && kind.gamepadButton < bindings.length
                            && bindings[kind.gamepadButton] == 2) {
                        if (kind.bit >= 0) bits |= 1 << kind.bit;
                        if (kind.trigger >= 0) axes[kind.trigger] = 32767;
                        break;
                    }
                }
                continue;
            }
            if (control < 0 || control >= layout.size() ||
                    redundantVehicleReload(control) || !vehicleControlVisible(control))
                continue;
            Kind kind = KINDS[layout.type(control)];
            if (kind.bit >= 0)
                bits |= 1 << kind.bit;
            if (kind.trigger >= 0)
                axes[kind.trigger] = 32767;
        }
        nativeState(axes[0], axes[1], axes[2], axes[3], axes[4], axes[5], bits);
    }

    private boolean held(int control) {
        return owners.indexOfValue(control) >= 0;
    }

    private float stickCenterX() {
        return stickFloating ? stickX : layout.x(TouchLayout.LEFT);
    }

    private float stickCenterY() {
        return stickFloating ? stickY : layout.y(TouchLayout.LEFT);
    }

    /** the floating stick, centred where a thumb landed (at rest there, even near an edge) */
    private void floatStick(float x, float y) {
        stickFloating = true;
        stickX = x;
        stickY = y;
    }

    private boolean inFloatingZone(float x, float y) {
        return x < logicalWidth * FLOATING_ZONE_WIDTH && y > logicalHeight * FLOATING_ZONE_TOP;
    }

    private void moveStick(int control, float x, float y) {
        float dx = (x - stickCenterX()) / layout.radius(control);
        float dy = (y - stickCenterY()) / layout.radius(control);
        float length = (float) Math.sqrt(dx * dx + dy * dy);
        if (length < STICK_DEAD_ZONE) {
            dx = 0;
            dy = 0;
        } else if (length > 1) {
            dx /= length;
            dy /= length;
        }
        axes[0] = Math.round(dx * 32767);
        axes[1] = Math.round(dy * 32767);
    }

    /** The temporary lock target is above the move-stick rim. All positions
     * are in logical HUD units, the same coordinates as finger movement. */
    private float autoWalkLockX() {
        float r = layout.radius(TouchLayout.LEFT);
        return Math.max(r * AUTO_WALK_LOCK_RADIUS + 3f,
            Math.min(logicalWidth - r * AUTO_WALK_LOCK_RADIUS - 3f, stickCenterX()));
    }

    private float autoWalkLockY() {
        float r = layout.radius(TouchLayout.LEFT);
        return Math.max(r * AUTO_WALK_LOCK_RADIUS + 3f,
            stickCenterY() - r * AUTO_WALK_LOCK_DISTANCE);
    }

    /** A short, gentle hardware pulse. View.performHapticFeedback(CLOCK_TICK)
     * can be silently ignored by some game/overlay configurations. The game
     * already has VIBRATE permission and a Vibrator for controller rumble.
     * This is independent of the game's optional continuous rumble toggle. */
    private void vibrateForwardLockReached() {
        if (vibrator == null || !vibrator.hasVibrator()) {
            performHapticFeedback(HapticFeedbackConstants.KEYBOARD_TAP);
            return;
        }
        try {
            int strength = vibrator.hasAmplitudeControl()
                ? 105 : VibrationEffect.DEFAULT_AMPLITUDE;
            AudioAttributes attributes = new AudioAttributes.Builder()
                .setUsage(AudioAttributes.USAGE_ASSISTANCE_SONIFICATION)
                .setContentType(AudioAttributes.CONTENT_TYPE_SONIFICATION)
                .build();
            vibrator.vibrate(VibrationEffect.createOneShot(28L, strength), attributes);
        } catch (RuntimeException ignored) {
            // Some devices disallow direct pulses: still try framework haptics.
            performHapticFeedback(HapticFeedbackConstants.KEYBOARD_TAP);
        }
    }

    /** Only the Move pointer may reach this temporary target. Reaching it
     * HIGHLIGHTS the lock (and lightly ticks once); it does NOT start auto-walk.
     * Only releasing the SAME finger on the lock can commit the action. */
    private void trackForwardLockArm(int pointer, float x, float y) {
        if (autoWalkLocked) return;
        if (!modernUi || !showing || hidden || editing || vehicleOccupied()
                || owners.get(pointer, NOTHING) != TouchLayout.LEFT) {
            autoWalkHintVisible = false;
            autoWalkLockReady = false;
            autoWalkHintPointer = -1;
            return;
        }
        float radius = layout.radius(TouchLayout.LEFT);
        if (radius <= 0) {
            autoWalkHintVisible = false;
            autoWalkLockReady = false;
            autoWalkHintPointer = -1;
            return;
        }
        float dx = (x - stickCenterX()) / radius;
        float dy = (y - stickCenterY()) / radius;
        if (dy > -AUTO_WALK_HINT_FORWARD || Math.abs(dx) > AUTO_WALK_HINT_SIDE) {
            autoWalkHintVisible = false;
            autoWalkLockReady = false;
            autoWalkHintPointer = -1;
            return;
        }
        autoWalkHintVisible = true;
        autoWalkHintPointer = pointer;
        float hitRadius = Math.max(18f, radius * AUTO_WALK_LOCK_RADIUS);
        boolean reached = inside(x, y, autoWalkLockX(), autoWalkLockY(), hitRadius);
        if (reached && !autoWalkLockReady && !autoWalkHapticDone) {
            // FORWARD_LOCK_HAPTIC_V23: real short vibration on arrival, once
            // per drag gesture; reaching does not engage auto-walk.
            vibrateForwardLockReached();
            autoWalkHapticDone = true;
        }
        autoWalkLockReady = reached;
    }

    // ---------- fingers

    private static boolean inside(float x, float y, float cx, float cy, float radius) {
        return (x - cx) * (x - cx) + (y - cy) * (y - cy) <= radius * radius;
    }

    /** D-pad directions belong to controller navigation, not the mobile gameplay HUD. */
    private boolean isGameplayDpad(int type) {
        return type >= 12 && type <= 15;
    }

    private boolean isWeaponSwitchControl(int type) {
        int button = KINDS[type].gamepadButton;
        return button >= 0 && button < bindings.length && bindings[button] == 3;
    }

    /** Read the role from the same native state used by the vehicle ENTER cue.
     * Bit 6 carries the seat-tag allow-weapons capability for passengers. */
    private int vehicleRole() {
        if (!vehicleOccupied()) return 0;
        return (weaponSlots[9] >> 4) & 3;
    }

    /** VEHICLE_WEAPON_PICKUP_PRIORITY_V11: real Hold-X ground-item prompts
     * always use the infantry HUD, even with stale vehicle bits. */
    private boolean groundItemPrompt() {
        return (weaponSlots[9] & 512) != 0;
    }

    /** VEHICLE_FLIP_RELOAD_PROMPT_V18: only native action "flip vehicle". */
    private boolean vehicleFlipPrompt() {
        return modernUi && !editing && !vehicleOccupied()
            && (weaponSlots[9] & 2048) != 0;
    }

    /** A valid occupant has BOTH a native occupied bit and a resolved seat
     * role (driver/gunner/passenger). Never hide everything for role zero. */
    private boolean vehicleOccupied() {
        return modernUi && !editing && !groundItemPrompt()
            && (weaponSlots[9] & 256) != 0 && (weaponSlots[9] & 48) != 0;
    }

    /** VEHICLE_GHOST_SECONDARY_V12: native bit from the real occupied vehicle. */
    private boolean vehicleIsGhost() {
        return vehicleOccupied() && (weaponSlots[9] & 1024) != 0;
    }

    private boolean passengerCanFire() {
        return (weaponSlots[9] & 64) != 0;
    }

    /** v5: Bit 7 is set by the native engine ONLY when a driver's seat
     * also controls the vehicle's weapon/gunner (never by vehicle name). */
    private boolean vehicleDriverHasWeapons() {
        return (weaponSlots[9] & 128) != 0;
    }

    /** In EVERY vehicle: no Jump, Crouch, Melee or weapon switching.
     * Driver of Warthog: steering + wheel/EXIT. Armed driver: Fire and
     * secondary Fire. Gunner: Fire/secondary Fire/Zoom. Armed passenger:
     * Fire/Zoom. Unknown seats preserve steering and both trigger buttons
     * rather than accidentally blocking a modded vehicle's weapons.
     * Real control bindings are honored instead of assuming buttons A/B/RT. */
    /** VEHICLE_RELOAD_DUPLICATE_FIX_V9: the wheel already sends Reload/Use
     * (game action 2) for EXIT. Hide the old standalone action only while
     * seated in Modern HUD; do not assume the physical button is always X. */
    private boolean redundantVehicleReload(int control) {
        if (!vehicleOccupied() || control < 0 || control >= layout.size())
            return false;
        int type = layout.type(control);
        if (type < 0 || type >= KINDS.length) return false;
        int button = KINDS[type].gamepadButton;
        // Also hide the physical X pictogram if a custom binding moved Reload.
        if (button == 12 || button == 13) return false; // Back / Pause
        return button == 2 || (button >= 0 && button < bindings.length
            && bindings[button] == 2);
    }

    private boolean vehicleControlVisible(int control) {
        if (!vehicleOccupied()) return true;
        int role = vehicleRole();
        int type = layout.type(control);
        if (type == TouchLayout.LEFT)
            return role == VEHICLE_DRIVER || role == 0;
        if (type < 0 || type >= KINDS.length) return false;
        Kind kind = KINDS[type];
        if (kind.gamepadButton < 0 || kind.gamepadButton >= bindings.length)
            return false;
        int action = bindings[kind.gamepadButton];
        // System/menu actions must remain visible and touchable in every seat.
        if (kind.gamepadButton == 12 || kind.gamepadButton == 13 ||
                action == 8 || action == 9) return true; // Pause / Back
        // Only the contextual steering wheel should provide X while seated.
        if (kind.gamepadButton == 2 || action == 2) return false;
        if (role == VEHICLE_DRIVER)
            return vehicleDriverHasWeapons() &&
                (action == 7 || (action == 6 && !vehicleIsGhost()));
        if (role == VEHICLE_GUNNER)
            return action == 7 || action == 6 || action == 11;
        if (role == VEHICLE_PASSENGER)
            return passengerCanFire() && (action == 7 || action == 11);
        // Unknown seat: prefer usable driving/guns without infantry buttons.
        return action == 7 || action == 6;
    }

    /** Show ENTER on foot when Halo resolves an available seat, or EXIT
     * throughout the time our unit actually occupies a vehicle seat. */
    private boolean vehicleEnterAvailable() {
        return modernUi && showing && !hidden && !editing
            && !groundItemPrompt()
            && ((weaponSlots[9] & 8) != 0 || vehicleOccupied());
    }

    /** Put the contextual button immediately LEFT of the grenade-throw control,
     * following that control if the user moved/resized it in the layout editor. */
    private float[] vehicleEnterPosition() {
        if (!vehicleEnterAvailable()) return null;
        for (int i = 0; i < layout.size(); i++) {
            if ((!vehicleOccupied() && !layout.shown(i)) ||
                    layout.type(i) == TouchLayout.LEFT) continue;
            int type = layout.type(i);
            if (type < 0 || type >= KINDS.length) continue;
            Kind kind = KINDS[type];
            if (kind.gamepadButton < 0 || kind.gamepadButton >= bindings.length
                    || bindings[kind.gamepadButton] != 6) continue;
            float radius = Math.max(22f, Math.min(32f, layout.radius(i) * 0.78f));
            float x = layout.x(i) - layout.radius(i) - radius - 8f;
            if (x < radius + 2f) {
                if (!vehicleOccupied()) return null;
                x = radius + 2f;
            }
            return new float[] {x, layout.y(i), radius};
        }
        // A modded profile might unbind Grenade entirely. Still allow EXIT.
        if (vehicleOccupied()) {
            float radius = 28f;
            return new float[] {logicalWidth * 0.74f, logicalHeight * 0.43f, radius};
        }
        return null;
    }

    /** Transparent wheel for both ENTER and EXIT, same contextual action. */
    private Bitmap getVehicleWheelIconBitmap() {
        if (vehicleWheelIconBitmap == null)
            vehicleWheelIconBitmap = BitmapFactory.decodeResource(
                getResources(), R.drawable.vehicle_steering_wheel);
        return vehicleWheelIconBitmap;
    }

    /** Replace the native blue X pictogram of the vehicle-entry message.
     * The native message text and seat/vehicle names remain untouched. The
     * wheel is only an overlay on its pictogram, not a new game action.
     * Use fractions of the viewport because Halo scales its HUD independently
     * of Android's user-customisable gameplay buttons. */
    private void drawVehiclePromptWheel(Canvas canvas) {
        if (!modernUi || !showing || hidden || editing || vehicleOccupied()
                || groundItemPrompt() || (weaponSlots[9] & 8) == 0)
            return;
        Bitmap wheel = getVehicleWheelIconBitmap();
        if (wheel == null) return;
        // VEHICLE_WHEEL_PROMPT_ALIGN_V15: align the wheel with the *blue X*
        // in Halo's native enter-seat prompt, not below the prompt text.
        // Calibrated on the supplied 957x544 Android screenshot; fractions
        // keep the placement relative to the same logical viewport.
        // VEHICLE_WHEEL_PROMPT_ALIGN_V16: subir un poco más para cubrir la X azul.
        // VEHICLE_WHEEL_PROMPT_SIZE_V17: más grande SOLO sobre la X del mensaje.
        // El botón ENTER/EXIT de la pantalla conserva su tamaño original.
        float x = logicalWidth * 0.139f;
        float y = logicalHeight * 0.214f;
        float radius = Math.max(17f, Math.min(24f, logicalHeight * 0.041f));
        // Cover the blue X in the game's message, never the surrounding text.
        paint.setStyle(Paint.Style.FILL);
        paint.setColor(0xFF19344B);
        paint.setAlpha(255);
        canvas.drawCircle(x, y, radius, paint);
        paint.setColor(Color.WHITE);
        paint.setAlpha(255);
        paint.setFilterBitmap(true);
        float half = radius * 0.77f;
        canvas.drawBitmap(wheel, null,
            new RectF(x - half, y - half, x + half, y + half), paint);
    }

    private void drawVehicleEnterButton(Canvas canvas) {
        float[] pos = vehicleEnterPosition();
        if (pos == null) return;
        boolean active = owners.indexOfValue(VEHICLE_ENTER_BUTTON) >= 0;
        // Preserve the original translucent circle and press feedback.
        circle(canvas, pos[0], pos[1], pos[2], "", active, 10f, layout.opacity);
        Bitmap wheel = getVehicleWheelIconBitmap();
        if (wheel == null) return;
        paint.setStyle(Paint.Style.FILL);
        paint.setColor(Color.WHITE);
        paint.setAlpha(Math.round(255f * Math.max(0.55f, layout.opacity)));
        paint.setFilterBitmap(true);
        float half = pos[2] * 0.69f;
        canvas.drawBitmap(wheel, null,
            new RectF(pos[0] - half, pos[1] - half,
                      pos[0] + half, pos[1] + half), paint);
    }

    /** what is under a point (logical units): a toolbar item, a control, or NOTHING */
    private int hit(float x, float y) {
        if (inside(x, y, optionsX(), toolbarY(), toolbarRadius(OPTIONS_RADIUS)))
            return TOOLBAR_OPTIONS;
        if (editing && inside(x, y, optionsX() - 156, toolbarY(), toolbarRadius(OPTIONS_RADIUS)))
            return TOOLBAR_EXPORT;
        if (editing && inside(x, y, optionsX() - 78, toolbarY(), toolbarRadius(OPTIONS_RADIUS)))
            return TOOLBAR_IMPORT;
        if (!editing && inside(x, y, logicalWidth / 2, toolbarY(), toolbarRadius(TOOLBAR_RADIUS)))
            return TOOLBAR_TOGGLE;
        if (hidden && !editing)
            return NOTHING;
        float[] vehicle = vehicleEnterPosition();
        if (vehicle != null && inside(x, y, vehicle[0], vehicle[1], vehicle[2]))
            return VEHICLE_ENTER_BUTTON;
        for (int i = 0; i < layout.size(); i++) {
            if (layout.shown(i) && layout.type(i) != TouchLayout.LEFT
                    && !redundantVehicleReload(i) && vehicleControlVisible(i)
                    && (!modernUi || !isGameplayDpad(layout.type(i)))
                    && (!modernUi || editing || !isWeaponSwitchControl(layout.type(i)))
                    && inside(x, y, layout.x(i), layout.y(i), layout.radius(i)))
                return i;
        }
        int stick = TouchLayout.LEFT;
        float reach = editing ? 1 : STICK_REACH;
        if (layout.shown(stick) && vehicleControlVisible(stick) && !held(stick)
                && inside(x, y, layout.x(stick), layout.y(stick), layout.radius(stick) * reach))
            return stick;
        return NOTHING;
    }

    /**
     * whether a finger went down where Android keeps its edge gestures; in
     * sticky full screen the first swipe in from an edge only shows the
     * system bars, and Android also hands it over as an ordinary finger
     */
    private boolean inGestureZone(float x, float y, boolean sidesOnly) {
        int[] insets = ((HaloActivity) getContext()).getSystemGestureInsetsPixels();
        if (x < insets[0] || x >= getWidth() - insets[2])
            return true;
        return !sidesOnly && (y < insets[1] || y >= getHeight() - insets[3]);
    }

    @Override
    public boolean onTouchEvent(MotionEvent event) {
        int action = event.getActionMasked();
        if (!showing && !editing && openDialogs == 0) {
            // the menus and cinematics take the fingers (SDL's surface below)
            if (action == MotionEvent.ACTION_DOWN)
                return false;
            if (owners.size() > 0 || lookPointer >= 0)
                reset();
            return true;
        }
        if (editing)
            editTouch(event);
        else
            gameTouch(event);
        publish();
        invalidate();
        return true;
    }

    @Override
    public boolean performClick() {
        super.performClick();
        return true;
    }

    private void gameTouch(MotionEvent event) {
        if (event.getActionMasked() == MotionEvent.ACTION_CANCEL) slotFingers.clear();
        int index = event.getActionIndex();
        int id = event.getPointerId(index);
        switch (event.getActionMasked()) {
            case MotionEvent.ACTION_DOWN:
            case MotionEvent.ACTION_POINTER_DOWN:
                fingerDown(id, event.getX(index), event.getY(index));
                break;
            case MotionEvent.ACTION_MOVE:
                for (int i = 0; i < event.getPointerCount(); i++)
                    fingerMoved(event.getPointerId(i), event.getX(i), event.getY(i), event.getEventTime());
                break;
            case MotionEvent.ACTION_UP:
            case MotionEvent.ACTION_POINTER_UP:
                fingerUp(id, event.getX(index), event.getY(index));
                break;
            case MotionEvent.ACTION_CANCEL:
                reset();
                break;
            default:
                break;
        }
    }

    /** a finger down in a game, at a point in pixels */
    private RectF weaponFrame(int slot) {
        float w = 102f, h = 43f, gap = 4f;
        float start = logicalWidth * 0.50f - w - gap / 2f;
        float top = logicalHeight - h - 7f;
        // The expanded drawer sits immediately above the original two frames.
        if (slot >= 2) top -= h + 6f;
        float left = start + (slot % 2) * (w + gap);
        return new RectF(left, top, left + w, top + h);
    }

    /** The drawer exists only when the engine really has weapon slot 3 or 4. */
    private boolean hasExtraWeapons() {
        return (weaponSlots[9] & 3) != 0;
    }

    /** Halo is currently showing a pickup/swap prompt for a weapon. */
    private boolean hasWeaponPickupPrompt() {
        // VEHICLE_PROMPT_ICON_BIT_COLLISION_V13
        // Before: bit 8 (256) indicated a ground pickup, but v8 reused it
        // for an occupied vehicle seat. That painted a floating Reload icon
        // on every vehicle even after its actual button was hidden.
        // v11 defines bit 9 (512) exclusively for an on-foot item prompt.
        return groundItemPrompt() && !vehicleOccupied();
    }

    private int physicalWeaponMask() {
        int mask = 0;
        if (weaponSlots[0] >= 0) mask |= 1;
        if (weaponSlots[1] >= 0) mask |= 2;
        if ((weaponSlots[9] & 1) != 0) mask |= 4;
        if ((weaponSlots[9] & 2) != 0) mask |= 8;
        return mask;
    }

    private boolean hasWeaponAtFrame(int frame) {
        return (physicalWeaponMask() & (1 << weaponViewOrder[frame])) != 0;
    }

    private void resetWeaponViewOrder() {
        for (int i = 0; i < 4; i++) weaponViewOrder[i] = i;
    }

    private int frameAt(float x, float y) {
        int count = weaponDrawerExpanded && hasExtraWeapons() ? 4 : 2;
        for (int i = 0; i < count; i++)
            if (weaponFrame(i).contains(x, y)) return i;
        return -1;
    }

    private void moveWeaponBetweenFrames(int source, int destination) {
        // Only promote a weapon FROM the drawer into one of the two quick
        // frames. The displaced quick weapon automatically becomes inventory.
        // The engine inventory itself stays untouched; weaponViewOrder tells
        // the native selection gate which two physical slots are currently
        // the quick pair.
        if (source < 2 || destination < 0 || destination >= 2) return;
        boolean selectMovedWeapon =
            weaponSlots[2] == weaponViewOrder[destination];
        int moved = WeaponSlotOrder.move(weaponViewOrder, physicalWeaponMask(),
            source, destination);
        if (moved < 0) return;
        nativeWeaponDisplayOrder(weaponViewOrder);
        if (selectMovedWeapon) nativeSelectWeaponSlot(moved);
    }

    private RectF weaponDrawerArrowFrame() {
        RectF primary = weaponFrame(0);
        float right = primary.left - 5f;
        float cy = primary.centerY();
        return new RectF(right - 32f, cy - 17f, right, cy + 17f);
    }

    private void fingerDown(int id, float px, float py) {
        if (inGestureZone(px, py, true))
            return;
        float x = logical(px);
        float y = logical(py);
        if (modernUi && !editing && !hidden && !vehicleOccupied()) {
            if (hasExtraWeapons() && weaponDrawerArrowFrame().contains(x, y)) {
                slotFingers.put(id, WEAPON_DRAWER_TOGGLE);
                return;
            }
            // Capture even empty visible frames so a weapon can be dropped
            // into them. They do not capture aiming or gameplay input.
            int slot = frameAt(x, y);
            if (slot != -1) {
                slotFingers.put(id, slot);
                slotPositions.put(id, new float[] {x, y});
                return;
            }
        }
        int target = hit(x, y);
        int stick = TouchLayout.LEFT;
        if (target == NOTHING && layout.floatingStick && !hidden && layout.shown(stick)
                && vehicleControlVisible(stick) && !held(stick)
                && inFloatingZone(x, y) && !inGestureZone(px, py, false)) {
            floatStick(x, y);
            target = stick;
        }
        if (target == TOOLBAR_OPTIONS || target == TOOLBAR_TOGGLE) {
            toolbarFingers.put(id, new float[] {px, py, target});
        } else if (target == VEHICLE_ENTER_BUTTON) {
            owners.put(id, target);
        } else if (target != NOTHING) {
            owners.put(id, target);
            if (layout.type(target) == TouchLayout.LEFT) {
                // A new thumb on Move instantly cancels the previous auto-walk.
                autoWalkLocked = false;
                autoWalkHintVisible = false;
                autoWalkLockReady = false;
                autoWalkHapticDone = false;
                autoWalkHintPointer = -1;
                moveStick(target, x, y);
                trackForwardLockArm(id, x, y);
            } else
                buttonTouches.put(id, new float[] {px, py});
        } else if (!hidden && lookPointer < 0 && !inGestureZone(px, py, false)) {
            startLook(id, px, py);
        }
    }

    private void startLook(int id, float px, float py) {
        lookPointer = id;
        lookX = px;
        lookY = py;
        lookEventTime = -1;
        if (owners.get(id, NOTHING) == NOTHING)
            owners.put(id, LOOK);
    }

    private boolean movedPastSlop(float[] origin, float px, float py) {
        return Math.hypot(px - origin[0], py - origin[1]) > SLOP * scale;
    }

    private void fingerMoved(int id, float px, float py, long eventTime) {
        if (slotFingers.get(id, -1) >= 0) {
            float[] position = slotPositions.get(id);
            if (position != null) {
                position[0] = logical(px);
                position[1] = logical(py);
            }
            return;
        }
        int control = owners.get(id, NOTHING);
        if (control >= 0 && layout.type(control) == TouchLayout.LEFT) {
            if (!autoWalkLocked) {
                moveStick(control, logical(px), logical(py));
                trackForwardLockArm(id, logical(px), logical(py));
            }
        }
        float[] toolbar = toolbarFingers.get(id);
        if (toolbar != null && movedPastSlop(toolbar, px, py)) {
            // a swipe that began on the toolbar aims; the item does not act
            toolbarFingers.remove(id);
            if (!hidden && lookPointer < 0)
                startLook(id, toolbar[0], toolbar[1]);
        }
        float[] origin = buttonTouches.get(id);
        if (origin != null && lookPointer >= 0 && lookPointer != id) {
            // while another finger aims, a held button's finger may only take over from where it is
            origin[0] = px;
            origin[1] = py;
        }
        if (lookPointer < 0 && origin != null && movedPastSlop(origin, px, py)) {
            // a held button (either Fire too) also aims
            lookPointer = id;
            lookX = origin[0];
            lookY = origin[1];
            lookEventTime = -1;
        }
        if (id == lookPointer) {
            float dx = logical(px - lookX);
            float dy = logical(py - lookY);
            // Response depends on finger speed, not on holding a fixed direction.
            // Every output remains proportional to the actual finger displacement:
            // no target tracking, inertia, or turn rate when the finger stops.
            long elapsed = lookEventTime < 0 ? 16 : Math.max(1, eventTime - lookEventTime);
            float speed = (float) Math.hypot(dx, dy) * 1000f / elapsed;
            float blend = Math.min(1f, speed / 900f);
            float response = 0.78f + 0.67f * blend;
            nativeLook(dx * sensitivity * response, dy * sensitivity * response);
            lookX = px;
            lookY = py;
            lookEventTime = eventTime;
        }
    }

    private void fingerUp(int id, float px, float py) {
        int slot = slotFingers.get(id, -1);
        if (slot != -1) {
            slotFingers.delete(id);
            slotPositions.remove(id);
            float x = logical(px), y = logical(py);
            if (slot == WEAPON_DRAWER_TOGGLE) {
                if (!vehicleOccupied() && hasExtraWeapons() && weaponDrawerArrowFrame().contains(x, y)) {
                    weaponDrawerExpanded = !weaponDrawerExpanded;
                    nativeWeaponDrawerExpanded(modernUi && showing && !hidden && !editing
                        && !vehicleOccupied() && weaponDrawerExpanded);
                }
            } else {
                int destination = frameAt(x, y);
                if (destination >= 0 && destination != slot)
                    moveWeaponBetweenFrames(slot, destination);
                else if (destination == slot && slot < 2 && hasWeaponAtFrame(slot))
                    nativeSelectWeaponSlot(weaponViewOrder[slot]);
            }
            performClick();
            return;
        }

        float[] toolbar = toolbarFingers.get(id);
        if (toolbar != null) {
            toolbarFingers.remove(id);
            int item = (int) toolbar[2];
            if (hit(logical(px), logical(py)) == item)
                toolbarAction(item);
            return;
        }
        int control = owners.get(id, NOTHING);
        if (control >= 0 && layout.type(control) == TouchLayout.LEFT) {
            // Release, not reaching the target, commits auto-walk. Verify the
            // last UP position (Android may send UP without a final MOVE).
            if (!autoWalkLocked && autoWalkHintVisible && autoWalkHintPointer == id)
                trackForwardLockArm(id, logical(px), logical(py));
            boolean engage = modernUi && showing && !hidden && !editing
                && !vehicleOccupied() && autoWalkHintVisible
                && autoWalkHintPointer == id && autoWalkLockReady
                && inside(logical(px), logical(py), autoWalkLockX(), autoWalkLockY(),
                    Math.max(18f, layout.radius(TouchLayout.LEFT) * AUTO_WALK_LOCK_RADIUS));
            autoWalkLocked = engage;
            axes[0] = 0;
            axes[1] = engage ? -32767 : 0;
            autoWalkHintVisible = false;
            autoWalkLockReady = false;
            autoWalkHapticDone = false;
            autoWalkHintPointer = -1;
            stickFloating = false;
        }
        if (id == lookPointer) {
            lookPointer = -1;
            lookEventTime = -1;
        }
        owners.delete(id);
        buttonTouches.remove(id);
    }

    private void toolbarAction(int item) {
        if (item == TOOLBAR_OPTIONS)
            showOptions();
        else if (item == TOOLBAR_TOGGLE)
            setHidden(!hidden);
        performClick();
    }

    /** a finger in the layout editor: drags controls; Save, Export and Import act at once */
    private void editTouch(MotionEvent event) {
        int action = event.getActionMasked();
        int index = event.getActionIndex();
        int id = event.getPointerId(index);
        if (action == MotionEvent.ACTION_DOWN || action == MotionEvent.ACTION_POINTER_DOWN) {
            if (dragPointer >= 0)
                return;
            float x = logical(event.getX(index));
            float y = logical(event.getY(index));
            int target = hit(x, y);
            if (target == TOOLBAR_OPTIONS) {
                if (saveLayout()) {
                    reset();
                    editing = false;
                    performClick();
                }
            } else if (target == TOOLBAR_EXPORT || target == TOOLBAR_IMPORT) {
                reset();
                ((HaloActivity) getContext()).chooseLayoutFile(target == TOOLBAR_EXPORT, exportLayout());
            } else if (target >= 0) {
                dragPointer = id;
                dragControl = target;
                dragOffsetX = x - layout.x(target);
                dragOffsetY = y - layout.y(target);
            }
        } else if (action == MotionEvent.ACTION_MOVE && dragPointer >= 0) {
            int pointer = event.findPointerIndex(dragPointer);
            if (pointer >= 0) {
                layout.move(dragControl, logical(event.getX(pointer)) - dragOffsetX,
                    logical(event.getY(pointer)) - dragOffsetY);
            }
        } else if (action == MotionEvent.ACTION_CANCEL
                || ((action == MotionEvent.ACTION_UP || action == MotionEvent.ACTION_POINTER_UP) && id == dragPointer)) {
            dragPointer = -1;
            dragControl = -1;
        }
    }

    // ---------- the buttons' names, from the player's profile

    private void readBindings() {
        int serial = nativeBindings(bindingsRead);
        if (serial == 0 || serial == bindingsSerial)
            return;
        bindingsSerial = serial;
        if (!Arrays.equals(bindingsRead, bindings)) {
            System.arraycopy(bindingsRead, 0, bindings, 0, bindings.length);
            invalidate();
        }
    }

    /** a control's name: its button and what the profile has it do ("A / Jump", "Fire", "RT") */
    private String label(int type) {
        Kind kind = KINDS[type];
        if (kind.gamepadButton < 0)
            return kind.buttonName;
        int control = bindings[kind.gamepadButton];
        if (control < 0 || control >= GAME_CONTROLS.length)
            return kind.buttonName;
        return kind.faceButton ? kind.buttonName + " / " + GAME_CONTROLS[control] : GAME_CONTROLS[control];
    }

    private String controlName(int type) {
        return type == TouchLayout.LEFT ? "Move stick" : label(type);
    }

    // ---------- the layout, saved and in files

    private boolean saveLayout() {
        boolean saved = preferences.edit().putString(configurationKey(), exportLayout()).commit();
        if (!saved)
            Toast.makeText(getContext(), "The touch layout could not be saved. Try again.", Toast.LENGTH_LONG).show();
        return saved;
    }

    public String exportLayout() {
        return layout.exportConfiguration(sensitivity);
    }

    /** replaces the layout with a layout file's (saved first: it throws if the file is not valid) */
    public void importLayout(String text) {
        TouchLayout.Configuration imported = TouchLayout.importConfiguration(text);
        imported.layout.bounds(logicalWidth, logicalHeight);
        imported.layout.setMinimumRadius(minimumRadius());
        String normalized = imported.layout.exportConfiguration(imported.sensitivity);
        if (!preferences.edit().putString(configurationKey(), normalized).commit())
            throw new IllegalArgumentException("Could not save the imported layout");
        reset();
        layout = imported.layout;
        sensitivity = imported.sensitivity;
        setHidden(false);
        cancelRumble();
        invalidate();
    }

    // ---------- the options

    /** opens an option dialog; the controls are released while any is open, and saved when the last closes */
    private void openDialog(AlertDialog dialog) {
        dialog.setOnDismissListener(d -> {
            openDialogs--;
            if (openDialogs > 0)
                return;
            openDialogs = 0;
            saveLayout();
            reset();
            updateSensors();
        });
        openDialogs++;
        reset();
        dialog.show();
    }

    private void showOptions() {
        String[] items = {"General", "Edit buttons layout", "Look sensitivity",
            "Touch interface: " + (modernUi ? "Modern" : "Classic")};
        AlertDialog dialog = new AlertDialog.Builder(getContext())
            .setTitle("Options")
            .setItems(items, (d, which) -> {
                if (which == 0) {
                    post(this::showGeneral);
                } else if (which == 1) {
                    editing = true;
                    setHidden(false);
                } else if (which == 2) {
                    post(this::showSensitivity);
                } else {
                    // TOUCH_OPTIONS_DIRECT_SWITCH_V19: toggle without campaign welcome dialog.
                    post(() -> selectInterface(!modernUi));
                }
            })
            .setNegativeButton("Close", null)
            .create();
        openDialog(dialog);
    }

    private boolean selectInterface(boolean modern) {
        if (modern == modernUi) return true;
        if (!saveLayout()) return false;
        if (!preferences.edit().putBoolean(KEY_MODERN, modern).commit()) {
            Toast.makeText(getContext(), "The interface could not be saved.", Toast.LENGTH_LONG).show();
            return false;
        }
        reset();
        modernUi = modern;
        loadLayout(getContext());
        layout.bounds(logicalWidth, logicalHeight);
        layout.setMinimumRadius(minimumRadius());
        nativeTouchHudActive(modernUi && showing && !hidden && !editing && !vehicleOccupied());
        cancelRumble();
        updateSensors();
        invalidate();
        return true;
    }

    /** Draw the saved buttons themselves, so previews follow layout edits and bindings. */
    private final class InterfacePreview extends View {
        private final boolean modern;
        private final TouchLayout preview;
        private final float previewWidth;
        private final float previewHeight;

        InterfacePreview(boolean modern) {
            super(TouchControls.this.getContext());
            this.modern = modern;
            TouchLayout candidate = new TouchLayout();
            if (!modern) candidate.resetClassicDefaults();
            String saved = modern == modernUi ? exportLayout() : preferences.getString(
                modern ? KEY_CONFIGURATION : KEY_CLASSIC_CONFIGURATION, null);
            if (saved != null) {
                try {
                    candidate = TouchLayout.importConfiguration(saved).layout;
                } catch (IllegalArgumentException ignored) {
                    // The same default layout is used when switching to an unreadable save.
                }
            }
            preview = candidate;
            previewWidth = logicalWidth > 0 ? logicalWidth : GRID_WIDTH;
            previewHeight = logicalHeight > 0 ? logicalHeight : GRID_HEIGHT;
            preview.bounds(previewWidth, previewHeight);
            preview.setMinimumRadius(minimumRadius());
            setContentDescription(modern ? "Modern touch button layout preview" : "Classic touch button layout preview");
        }

        @Override
        protected void onMeasure(int widthSpec, int heightSpec) {
            int width = MeasureSpec.getSize(widthSpec);
            setMeasuredDimension(width, resolveSize(Math.round(width * previewHeight / previewWidth), heightSpec));
        }

        @Override
        protected void onDraw(Canvas canvas) {
            Paint previous = new Paint(paint);
            canvas.save();
            canvas.drawColor(0xff050d1c);
            canvas.scale(getWidth() / previewWidth, getHeight() / previewHeight);
            paint.setStyle(Paint.Style.STROKE);
            paint.setStrokeWidth(2);
            paint.setColor(0xff2896ff);
            canvas.drawCircle(previewWidth / 2, previewHeight / 2, 7, paint);
            for (int i = 0; i < preview.size(); i++) {
                int type = preview.type(i);
                if (!preview.shown(i) || (modern && (isGameplayDpad(type) || isWeaponSwitchControl(type))))
                    continue;
                float x = preview.x(i), y = preview.y(i), radius = preview.radius(i);
                if (type == TouchLayout.LEFT) {
                    circle(canvas, x, y, radius, "", false, 11, 1);
                    circle(canvas, x, y, radius * 0.4f, "", false, 11, 1);
                } else if (modern) {
                    tacticalButton(canvas, x, y, radius, type, false, 1);
                } else {
                    circle(canvas, x, y, radius, label(type), false,
                        Math.min(11 * Math.max(preview.sizeScale(i), radius / 36f), 16), 1);
                }
            }
            if (modern) {
                // Match the collapsed weapon drawer button in the gameplay HUD.
                float arrowX = previewWidth * 0.5f - 104f - 21f;
                float arrowY = previewHeight - 28.5f;
                paint.setStyle(Paint.Style.STROKE);
                paint.setStrokeWidth(2f);
                paint.setColor(0xffcad1d4);
                canvas.drawRect(arrowX - 16f, arrowY - 17f, arrowX + 16f, arrowY + 17f, paint);
                canvas.drawLine(arrowX - 7f, arrowY + 4f, arrowX, arrowY - 4f, paint);
                canvas.drawLine(arrowX, arrowY - 4f, arrowX + 7f, arrowY + 4f, paint);
                for (int slot = 0; slot < 2; slot++) {
                    float left = previewWidth * 0.5f - 104 + slot * 106;
                    paint.setStyle(Paint.Style.STROKE);
                    paint.setStrokeWidth(2);
                    paint.setColor(slot == 0 ? 0xffffd64b : 0xffcad1d4);
                    canvas.drawRect(left, previewHeight - 50, left + 102, previewHeight - 7, paint);
                    paint.setStyle(Paint.Style.FILL);
                    paint.setTextSize(14);
                    paint.setTextAlign(Paint.Align.CENTER);
                    canvas.drawText("0" + (slot + 1), left + 51, previewHeight - 23, paint);
                }
            }
            canvas.restore();
            paint.set(previous);
        }
    }

    private GradientDrawable haloPanel(boolean highlighted) {
        // Same panel gradient and text blue as tools/port_settings.py.
        GradientDrawable background = new GradientDrawable(GradientDrawable.Orientation.TOP_BOTTOM,
            new int[] {highlighted ? 0xff164777 : 0xff0a2649, 0xff050d1c});
        background.setStroke(Math.max(1, Math.round(getResources().getDisplayMetrics().density)),
            highlighted ? 0xffb9e4ff : 0xff2896ff);
        return background;
    }

    private TextView haloText(String value, int size) {
        TextView view = text(value);
        view.setTextColor(0xff2896ff);
        view.setTextSize(size);
        return view;
    }

    private Button haloButton(String label, View.OnClickListener listener) {
        Button view = button(label, listener);
        view.setTextColor(Color.WHITE);
        view.setBackgroundTintList(null);
        StateListDrawable background = new StateListDrawable();
        background.addState(new int[] {android.R.attr.state_pressed}, haloPanel(true));
        background.addState(new int[] {android.R.attr.state_focused}, haloPanel(true));
        background.addState(new int[] {}, haloPanel(false));
        view.setBackground(background);
        return view;
    }

    private void showInterface() {
        showInterface(false);
    }

    private void showInterface(boolean beforeGame) {
        LinearLayout panel = new LinearLayout(getContext());
        panel.setOrientation(LinearLayout.VERTICAL);
        int padding = Math.round(12 * getResources().getDisplayMetrics().density);
        panel.setPadding(padding, padding, padding, padding);
        panel.setBackground(haloPanel(false));
        panel.addView(haloText("CHOOSE YOUR TOUCH LAYOUT", 22));
        panel.addView(haloText(beforeGame
            ? "Choose your touch controls to start the game."
            : "Choose your touch controls. You can change them later in Options.", 14));
        LinearLayout choices = new LinearLayout(getContext());
        choices.setOrientation(LinearLayout.HORIZONTAL);
        panel.addView(choices);
        ScrollView scroll = new ScrollView(getContext());
        scroll.addView(panel);
        AlertDialog dialog = new AlertDialog.Builder(getContext())
            .setView(scroll)
            .setCancelable(!beforeGame)
            .create();
        for (int index = 0; index < 2; index++) {
            final boolean modern = index == 1;
            LinearLayout card = new LinearLayout(getContext());
            card.setOrientation(LinearLayout.VERTICAL);
            card.setPadding(padding / 2, padding / 2, padding / 2, padding / 2);
            choices.addView(card, new LinearLayout.LayoutParams(0, LinearLayout.LayoutParams.WRAP_CONTENT, 1));
            TextView title = haloText(modern ? "MODERN" : "CLASSIC", 20);
            title.setTextSize(20);
            card.addView(title);
            card.addView(new InterfacePreview(modern), new LinearLayout.LayoutParams(
                LinearLayout.LayoutParams.MATCH_PARENT, LinearLayout.LayoutParams.WRAP_CONTENT));
            card.addView(haloText(modern ? "Action icons and two weapon slots." : "A, B, X, Y and weapon cycling.", 14));
            card.addView(haloButton(modern ? "Use Modern" : "Use Classic", view -> {
                if (selectInterface(modern)) {
                    dialog.dismiss();
                    if (beforeGame) nativeLayoutChosen();
                }
            }));
        }
        openDialog(dialog);
        dialog.getWindow().setBackgroundDrawableResource(android.R.color.transparent);
        dialog.getWindow().setLayout(Math.round(getResources().getDisplayMetrics().widthPixels * 0.94f),
            WindowManager.LayoutParams.WRAP_CONTENT);
    }

    private TextView text(String value) {
        TextView view = new TextView(getContext());
        view.setText(value);
        return view;
    }

    private Button button(String label, View.OnClickListener listener) {
        Button button = new Button(getContext());
        button.setText(label);
        button.setOnClickListener(listener);
        return button;
    }

    private void showGeneral() {
        cancelRumble();
        LinearLayout panel = new LinearLayout(getContext());
        panel.setOrientation(LinearLayout.VERTICAL);
        panel.setPadding(24, 16, 24, 16);

        boolean hasRumble = vibrator != null && vibrator.hasVibrator();
        Switch rumble = new Switch(getContext());
        rumble.setText(hasRumble ? "Vibration" : "Vibration (not on this phone)");
        rumble.setChecked(layout.rumbleEnabled);
        rumble.setEnabled(hasRumble);
        rumble.setOnCheckedChangeListener((view, enabled) -> {
            layout.rumbleEnabled = enabled;
            cancelRumble();
            saveLayout();
        });
        panel.addView(rumble);

        Switch gyro = new Switch(getContext());
        gyro.setText(gyroscope != null ? "Gyroscope aiming (experimental)" : "Gyroscope aiming (not on this phone)");
        gyro.setChecked(layout.gyroscopeEnabled);
        gyro.setEnabled(gyroscope != null);
        gyro.setOnCheckedChangeListener((view, enabled) -> {
            layout.gyroscopeEnabled = enabled;
            gyroAim.reset();
            saveLayout();
        });
        panel.addView(gyro);

        Switch floating = new Switch(getContext());
        floating.setText("Floating move stick: it goes where your thumb lands in the lower left");
        floating.setChecked(layout.floatingStick);
        floating.setOnCheckedChangeListener((view, enabled) -> {
            layout.floatingStick = enabled;
            saveLayout();
        });
        panel.addView(floating);

        TextView opacityLabel = text("");
        SeekBar opacity = new SeekBar(getContext());
        int steps = Math.round((TouchLayout.MAX_OPACITY - TouchLayout.MIN_OPACITY) * 20);
        opacity.setMax(steps);
        opacity.setProgress(Math.round((layout.opacity - TouchLayout.MIN_OPACITY) * 20));
        Runnable showOpacity = () -> opacityLabel.setText(String.format(Locale.US, "Opacity: %d%%",
            Math.round(layout.opacity * 100)));
        showOpacity.run();
        opacity.setOnSeekBarChangeListener(new SeekBar.OnSeekBarChangeListener() {
            @Override
            public void onProgressChanged(SeekBar bar, int progress, boolean user) {
                layout.setOpacity(Math.min(TouchLayout.MAX_OPACITY, TouchLayout.MIN_OPACITY + progress / 20f));
                showOpacity.run();
                invalidate();
            }

            @Override
            public void onStartTrackingTouch(SeekBar bar) {
            }

            @Override
            public void onStopTrackingTouch(SeekBar bar) {
                saveLayout();
            }
        });
        panel.addView(opacityLabel);
        panel.addView(opacity);

        AlertDialog dialog = new AlertDialog.Builder(getContext())
            .setTitle("General")
            .setView(panel)
            .setPositiveButton("Done", null)
            .create();
        panel.addView(button("Hide or add buttons", view -> {
            dialog.dismiss();
            post(this::showButtonManager);
        }));
        panel.addView(button("Edit buttons size", view -> {
            dialog.dismiss();
            post(this::showButtonSizes);
        }));
        openDialog(dialog);
    }

    private String rowName(int control, boolean withHidden) {
        String name = controlName(layout.type(control));
        if (control >= TouchLayout.BASE_COUNT)
            name += " (copy)";
        if (withHidden && !layout.shown(control))
            name += " (hidden)";
        return name;
    }

    private LinearLayout row(int control, boolean withHidden) {
        LinearLayout row = new LinearLayout(getContext());
        row.setPadding(16, 4, 16, 4);
        TextView name = text(rowName(control, withHidden));
        name.setTextColor(Color.WHITE);
        row.addView(name, new LinearLayout.LayoutParams(0, LinearLayout.LayoutParams.WRAP_CONTENT, 1));
        return row;
    }

    private void showButtonSizes() {
        LinearLayout list = new LinearLayout(getContext());
        list.setOrientation(LinearLayout.VERTICAL);
        for (int i = 0; i < layout.size(); i++) {
            final int control = i;
            LinearLayout row = row(control, true);
            TextView value = text("");
            value.setTextColor(Color.WHITE);
            Button smaller = new Button(getContext());
            Button larger = new Button(getContext());
            Runnable refresh = () -> {
                int percent = Math.round(layout.sizeScale(control) * 100);
                value.setText(percent + "%");
                smaller.setEnabled(percent > Math.round(TouchLayout.MIN_SIZE * 100));
                larger.setEnabled(percent < Math.round(TouchLayout.MAX_SIZE * 100));
            };
            smaller.setText("-");
            smaller.setOnClickListener(view -> resize(control, -10, refresh));
            larger.setText("+");
            larger.setOnClickListener(view -> resize(control, 10, refresh));
            row.addView(smaller);
            row.addView(value);
            row.addView(larger);
            list.addView(row);
            refresh.run();
        }
        ScrollView scroll = new ScrollView(getContext());
        scroll.addView(list);
        openDialog(new AlertDialog.Builder(getContext())
            .setTitle("Edit buttons size")
            .setView(scroll)
            .setPositiveButton("Done", null)
            .create());
    }

    private void resize(int control, int percent, Runnable refresh) {
        int size = Math.round(layout.sizeScale(control) * 100) + percent;
        size = Math.max(Math.round(TouchLayout.MIN_SIZE * 100), Math.min(Math.round(TouchLayout.MAX_SIZE * 100), size));
        layout.setSize(control, size / 100f);
        saveLayout();
        refresh.run();
        invalidate();
    }

    private void showButtonManager() {
        LinearLayout list = new LinearLayout(getContext());
        list.setOrientation(LinearLayout.VERTICAL);
        Runnable[] refresh = new Runnable[1];
        refresh[0] = () -> {
            list.removeAllViews();
            for (int i = 0; i < layout.size(); i++) {
                final int control = i;
                LinearLayout row = row(control, false);
                row.addView(button(layout.shown(control) ? "Hide" : "Show", view -> {
                    reset();
                    layout.setShown(control, !layout.shown(control));
                    saveLayout();
                    refresh[0].run();
                    invalidate();
                }));
                if (layout.type(control) != TouchLayout.LEFT) {
                    row.addView(button("Duplicate", view -> {
                        reset();
                        if (layout.duplicate(control) < 0)
                            toast("At most " + TouchLayout.MAX_CONTROLS + " controls. Reset removes the copies.");
                        else
                            saveLayout();
                        refresh[0].run();
                        invalidate();
                    }));
                }
                list.addView(row);
            }
        };
        refresh[0].run();
        ScrollView scroll = new ScrollView(getContext());
        scroll.addView(list);
        AlertDialog dialog = new AlertDialog.Builder(getContext())
            .setTitle("Hide or add buttons")
            .setView(scroll)
            .setPositiveButton("Done", null)
            .setNegativeButton("Reset", null)
            .setNeutralButton("Add button", null)
            .create();
        openDialog(dialog);
        dialog.getButton(AlertDialog.BUTTON_NEGATIVE).setOnClickListener(view -> confirmReset(refresh[0]));
        dialog.getButton(AlertDialog.BUTTON_NEUTRAL).setOnClickListener(view -> chooseButtonToAdd(refresh[0]));
    }

    private void confirmReset(Runnable refresh) {
        new AlertDialog.Builder(getContext())
            .setTitle("Reset the buttons?")
            .setMessage("This puts every button back in its place and size, shows them all and removes the copies.")
            .setNegativeButton("Cancel", null)
            .setPositiveButton("Reset", (d, which) -> {
                reset();
                if (modernUi) layout.resetDefaults();
                else layout.resetClassicDefaults();
                setHidden(false);
                saveLayout();
                refresh.run();
                invalidate();
            })
            .show();
    }

    private void chooseButtonToAdd(Runnable refresh) {
        String[] names = new String[TouchLayout.BASE_COUNT];
        for (int i = 0; i < names.length; i++)
            names[i] = controlName(i);
        new AlertDialog.Builder(getContext())
            .setTitle("Add button")
            .setItems(names, (d, type) -> {
                reset();
                if (layout.add(type) < 0)
                    toast(type == TouchLayout.LEFT ? "The move stick already shows." : "At most " + TouchLayout.MAX_CONTROLS + " controls.");
                else
                    saveLayout();
                refresh.run();
                invalidate();
            })
            .setNegativeButton("Cancel", null)
            .show();
    }

    private void showSensitivity() {
        LinearLayout panel = new LinearLayout(getContext());
        panel.setOrientation(LinearLayout.VERTICAL);
        panel.setPadding(32, 16, 32, 16);
        TextView value = text("");
        SeekBar slider = new SeekBar(getContext());
        int steps = Math.round((TouchLayout.MAX_SENSITIVITY - TouchLayout.MIN_SENSITIVITY) / 0.025f);
        slider.setMax(steps);
        slider.setProgress(Math.round((sensitivity - TouchLayout.MIN_SENSITIVITY) / 0.025f));
        Runnable showValue = () -> value.setText(String.format(Locale.US, "Look sensitivity: %.2fx", sensitivity));
        showValue.run();
        slider.setOnSeekBarChangeListener(new SeekBar.OnSeekBarChangeListener() {
            @Override
            public void onProgressChanged(SeekBar bar, int progress, boolean user) {
                sensitivity = Math.min(TouchLayout.MAX_SENSITIVITY, TouchLayout.MIN_SENSITIVITY + progress * 0.025f);
                showValue.run();
            }

            @Override
            public void onStartTrackingTouch(SeekBar bar) {
            }

            @Override
            public void onStopTrackingTouch(SeekBar bar) {
                saveLayout();
            }
        });
        panel.addView(value);
        panel.addView(slider);
        openDialog(new AlertDialog.Builder(getContext())
            .setTitle("Look sensitivity")
            .setView(panel)
            .setPositiveButton("Done", null)
            .create());
    }

    private void toast(String message) {
        Toast.makeText(getContext(), message, Toast.LENGTH_LONG).show();
    }

    // ---------- the gyroscope and the vibration

    private void updateSensors() {
        boolean needed = deviceInputActive && showing && !hidden && layout.gyroscopeEnabled && gyroscope != null;
        if (needed && !gyroRegistered) {
            gyroAim.reset();
            gyroRegistered = sensors.registerListener(this, gyroscope, SensorManager.SENSOR_DELAY_GAME);
        } else if (!needed && gyroRegistered) {
            sensors.unregisterListener(this);
            gyroRegistered = false;
            gyroAim.reset();
        }
    }

    @Override
    public void onSensorChanged(SensorEvent event) {
        if (!deviceInputActive || !showing || hidden || !layout.gyroscopeEnabled || editing || openDialogs > 0) {
            gyroAim.reset();
            return;
        }
        int rotation = ((WindowManager) getContext().getSystemService(Context.WINDOW_SERVICE))
            .getDefaultDisplay().getRotation();
        if (gyroAim.sample(event.timestamp, event.values[0], event.values[1], rotation, gyroDelta)) {
            // as a swipe of this many logical units (which the profile's invert leaves alone)
            nativeGyro(-gyroDelta[0] / LOOK_RADIANS_PER_UNIT * sensitivity,
                -gyroDelta[1] / LOOK_RADIANS_PER_UNIT * sensitivity);
        }
    }

    @Override
    public void onAccuracyChanged(Sensor sensor, int accuracy) {
    }

    /** the phone vibrates as port 0's controller would, while the controls show */
    private void updateRumble() {
        if (vibrator == null || !vibrator.hasVibrator())
            return;
        boolean active = showing && !hidden && layout.rumbleEnabled && !editing && openDialogs == 0;
        int amplitude = active ? nativeRumble() : 0;
        if (amplitude == 0) {
            cancelRumble();
            return;
        }
        long now = SystemClock.uptimeMillis();
        if (amplitude == lastAmplitude && now - lastVibration < VIBRATION_RENEW_MS)
            return;
        try {
            int strength = vibrator.hasAmplitudeControl() ? amplitude : VibrationEffect.DEFAULT_AMPLITUDE;
            vibrator.vibrate(VibrationEffect.createOneShot(VIBRATION_MS, strength), rumbleAttributes);
            lastAmplitude = amplitude;
            lastVibration = now;
        } catch (RuntimeException e) {
            cancelRumble();
        }
    }

    private void cancelRumble() {
        if (lastAmplitude != 0 && vibrator != null) {
            try {
                vibrator.cancel();
            } catch (RuntimeException ignored) {
                // the vibrator may already be gone with the activity
            }
        }
        lastAmplitude = 0;
    }

    // ---------- drawing

    private static int alpha(int color, float opacity) {
        int alpha = Math.round(Color.alpha(color) * opacity);
        return (color & 0x00ffffff) | (Math.max(0, Math.min(255, alpha)) << 24);
    }

    private void circle(Canvas canvas, float x, float y, float radius, String label, boolean active,
            float textSize, float opacity) {
        paint.setStyle(Paint.Style.FILL);
        paint.setColor(alpha(active ? 0x9983d9ff : 0x55304050, opacity));
        canvas.drawCircle(x, y, radius, paint);
        paint.setStyle(Paint.Style.STROKE);
        paint.setStrokeWidth(2);
        paint.setColor(alpha(active ? 0xffaee7ff : 0xffffffff, opacity));
        canvas.drawCircle(x, y, radius, paint);
        paint.setStyle(Paint.Style.FILL);
        paint.setColor(alpha(Color.WHITE, Math.max(opacity, 0.5f)));
        paint.setTextAlign(Paint.Align.CENTER);
        paint.setTextSize(textSize);
        // (a long name, "Y / Weapon", is made smaller to fit inside the circle)
        float width = paint.measureText(label);
        if (width > radius * LABEL_WIDTH) {
            textSize *= radius * LABEL_WIDTH / width;
            paint.setTextSize(textSize);
        }
        canvas.drawText(label, x, y + textSize * 0.36f, paint);
    }

    private Bitmap getCrouchIconBitmap() {
        if (crouchIconBitmap == null)
            crouchIconBitmap = BitmapFactory.decodeResource(getResources(), R.drawable.crouch_soldier);
        return crouchIconBitmap;
    }

    private Bitmap getJumpIconBitmap() {
        if (jumpIconBitmap == null)
            jumpIconBitmap = BitmapFactory.decodeResource(getResources(), R.drawable.jump_soldier);
        return jumpIconBitmap;
    }


    private Bitmap getFireIconBitmap() {
        if (fireIconBitmap == null) {
            fireIconBitmap = BitmapFactory.decodeResource(getResources(), R.drawable.fire_projectile);
        }
        return fireIconBitmap;
    }


    private Bitmap getMeleeIconBitmap() {
        if (meleeIconBitmap == null) {
            meleeIconBitmap = BitmapFactory.decodeResource(getResources(), R.drawable.melee_punch);
        }
        return meleeIconBitmap;
    }

    private Bitmap getGrenadeIconBitmap() {
        if (grenadeIconBitmap == null) {
            grenadeIconBitmap = BitmapFactory.decodeResource(getResources(), R.drawable.grenade_throw);
        }
        return grenadeIconBitmap;
    }

    /** Original tactical HUD pictograms; only in gameplay, never in Halo's touch menus. */
    private void tacticalButton(Canvas canvas, float x, float y, float radius, int type, boolean active, float opacity) {
        int action = -1;
        Kind kind = KINDS[type];
        if (kind.gamepadButton >= 0 && kind.gamepadButton < bindings.length)
            action = bindings[kind.gamepadButton];
        // Renderer safeguard: only the steering wheel presents EXIT while seated.
        if (vehicleOccupied() && kind.gamepadButton != 12 && kind.gamepadButton != 13
                && (action == 2 || kind.gamepadButton == 2)) return;
        if (action < 0 || action >= GAME_CONTROLS.length) {
            circle(canvas, x, y, radius, kind.buttonName, active, 11, opacity);
            return;
        }
        // A smoke-grey ring with white line-art, like a contemporary mobile shooter.
        paint.setStyle(Paint.Style.FILL);
        paint.setColor(alpha(active ? 0x9998bac8 : 0x66313c43, opacity));
        canvas.drawCircle(x, y, radius, paint);
        paint.setStyle(Paint.Style.STROKE);
        paint.setStrokeWidth(1.6f);
        paint.setColor(alpha(Color.WHITE, opacity * (active ? 0.90f : 0.52f)));
        canvas.drawCircle(x, y, radius, paint);
        canvas.save();
        canvas.translate(x, y);
        float s = radius / 42f;
        canvas.scale(s, s);
        paint.setStrokeWidth(2.6f);
        paint.setStrokeCap(Paint.Cap.ROUND);
        paint.setStrokeJoin(Paint.Join.ROUND);
        paint.setColor(alpha(Color.WHITE, Math.max(0.55f, opacity)));
        paint.setStyle(Paint.Style.STROKE);
        Path path = new Path();
        switch (action) {
            case 7: { // FIRE: original monochrome projectile icon
                Bitmap fire = getFireIconBitmap();
                if (fire != null) {
                    paint.setStyle(Paint.Style.FILL);
                    paint.setColor(alpha(Color.WHITE, Math.max(0.55f, opacity)));
                    float maxW = 52f;
                    float maxH = 52f;
                    float bw = fire.getWidth();
                    float bh = fire.getHeight();
                    float bitmapScale = Math.min(maxW / bw, maxH / bh);
                    float dx = bw * bitmapScale * 0.5f;
                    float dy = bh * bitmapScale * 0.5f;
                    RectF dst = new RectF(-dx, -dy, dx, dy);
                    canvas.drawBitmap(fire, null, dst, paint);
                    paint.setColor(alpha(Color.WHITE, Math.max(0.55f, opacity)));
                    paint.setStyle(Paint.Style.STROKE);
                } else {
                    line(canvas, -20, 3, 18, 3);
                    line(canvas, -17, -4, 13, -4);
                    line(canvas, -14, -4, -14, -9);
                    line(canvas, 13, -4, 22, 0);
                    line(canvas, 0, 4, -4, 15);
                    line(canvas, -4, 15, 6, 15);
                    line(canvas, -18, 5, -21, 12);
                }
                break;
            }
            case 11: // ADS / reticle
                canvas.drawCircle(0, 0, 12, paint);
                canvas.drawCircle(0, 0, 2.5f, paint);
                line(canvas, -23, 0, -15, 0); line(canvas, 15, 0, 23, 0);
                line(canvas, 0, -23, 0, -15); line(canvas, 0, 15, 0, 23);
                break;
            case 2: // RELOAD: cartridges surrounded by two circular arrows
                paint.setStrokeWidth(3.0f);
                canvas.drawArc(new RectF(-21, -21, 21, 21), 208, 140, false, paint);
                canvas.drawArc(new RectF(-21, -21, 21, 21), 27, 140, false, paint);
                path.moveTo(18, -17); path.lineTo(22, -5); path.lineTo(10, -9);
                canvas.drawPath(path, paint);
                path.reset();
                path.moveTo(-18, 17); path.lineTo(-22, 5); path.lineTo(-10, 9);
                canvas.drawPath(path, paint);
                // Three unmistakable rounds, not a grenade.
                paint.setStyle(Paint.Style.FILL);
                for (int k = -1; k <= 1; k++) {
                    float bx = k * 9f;
                    canvas.drawRoundRect(new RectF(bx - 3, -10, bx + 3, 10), 1.2f, 1.2f, paint);
                    path.reset();
                    path.moveTo(bx - 3, -10);
                    path.lineTo(bx, -15);
                    path.lineTo(bx + 3, -10);
                    path.close();
                    canvas.drawPath(path, paint);
                }
                paint.setStyle(Paint.Style.STROKE);
                break;
            case 0: { // JUMP: original monochrome jump icon
                Bitmap jump = getJumpIconBitmap();
                if (jump != null) {
                    paint.setStyle(Paint.Style.FILL);
                    paint.setColor(alpha(Color.WHITE, Math.max(0.55f, opacity)));
                    float bw = jump.getWidth();
                    float bh = jump.getHeight();
                    float bitmapScale = Math.min(58f / bw, 58f / bh);
                    float dx = bw * bitmapScale * 0.5f;
                    float dy = bh * bitmapScale * 0.5f;
                    canvas.drawBitmap(jump, null, new RectF(-dx, -dy, dx, dy), paint);
                    paint.setStyle(Paint.Style.STROKE);
                }
                break;
            }
            case 10: { // CROUCH: original monochrome crouch icon
                Bitmap crouch = getCrouchIconBitmap();
                if (crouch != null) {
                    paint.setStyle(Paint.Style.FILL);
                    paint.setColor(alpha(Color.WHITE, Math.max(0.55f, opacity)));
                    float maxW = 58f;
                    float maxH = 54f;
                    float bw = crouch.getWidth();
                    float bh = crouch.getHeight();
                    float bitmapScale = Math.min(maxW / bw, maxH / bh);
                    float dx = bw * bitmapScale * 0.5f;
                    float dy = bh * bitmapScale * 0.5f;
                    RectF dst = new RectF(-dx, -dy + 1.5f, dx, dy + 1.5f);
                    canvas.drawBitmap(crouch, null, dst, paint);
                    paint.setColor(alpha(Color.WHITE, Math.max(0.55f, opacity)));
                    paint.setStyle(Paint.Style.STROKE);
                } else {
                    paint.setStyle(Paint.Style.FILL);
                    canvas.drawCircle(2, -16, 5, paint);
                    canvas.drawRoundRect(new RectF(-1, -21, 9, -17), 2, 2, paint);
                    path.reset();
                    path.moveTo(0, -10); path.lineTo(12, -5);
                    path.lineTo(14, 5); path.lineTo(4, 8);
                    path.lineTo(-5, 0); path.close();
                    canvas.drawPath(path, paint);
                    canvas.drawRoundRect(new RectF(-22, -10, 2, -7), 1, 1, paint);
                    canvas.drawRect(-14, -13, -10, -10, paint);
                    paint.setStyle(Paint.Style.STROKE);
                    paint.setStrokeWidth(4f);
                    line(canvas, 0, -6, -8, -8);
                    line(canvas, 6, -4, -3, -8);
                    line(canvas, 5, 5, -11, 8);
                    line(canvas, -11, 8, -19, 15);
                    line(canvas, 5, 5, 16, 11);
                    line(canvas, 16, 11, 12, 18);
                    paint.setStrokeWidth(2.6f);
                }
                break;
            }
            case 6: { // GRENADE: original monochrome throw icon
                // SECONDARY_VEHICLE_FIRE_V7: Halo CE uses the grenade/LT action as
                // the vehicle weapon's SECONDARY trigger (not a weapon switch).
                // Only change the artwork while piloting an armed vehicle.
                if (vehicleRole() == VEHICLE_DRIVER && vehicleDriverHasWeapons()
                        && !vehicleIsGhost()) {
                    paint.setStyle(Paint.Style.STROKE);
                    paint.setStrokeWidth(2.7f);
                    // Secondary weapon: twin cannon barrels with a flash.
                    canvas.drawRoundRect(new RectF(-21f, -13f, 7f, -4f), 2f, 2f, paint);
                    canvas.drawRoundRect(new RectF(-21f, 4f, 7f, 13f), 2f, 2f, paint);
                    canvas.drawLine(7f, -8f, 17f, -8f, paint);
                    canvas.drawLine(7f, 8f, 17f, 8f, paint);
                    canvas.drawLine(17f, -8f, 23f, -17f, paint);
                    canvas.drawLine(17f, -8f, 25f, -8f, paint);
                    canvas.drawLine(17f, 8f, 23f, 17f, paint);
                    canvas.drawLine(17f, 8f, 25f, 8f, paint);
                    break;
                }
                Bitmap grenade = getGrenadeIconBitmap();
                if (grenade != null) {
                    paint.setStyle(Paint.Style.FILL);
                    paint.setColor(alpha(Color.WHITE, Math.max(0.55f, opacity)));
                    float maxW = 50f;
                    float maxH = 42f;
                    float bw = grenade.getWidth();
                    float bh = grenade.getHeight();
                    float bitmapScale = Math.min(maxW / bw, maxH / bh);
                    float dx = bw * bitmapScale * 0.5f;
                    float dy = bh * bitmapScale * 0.5f;
                    canvas.drawBitmap(grenade, null, new RectF(-dx, -dy, dx, dy), paint);
                    paint.setStyle(Paint.Style.STROKE);
                } else {
                    canvas.drawOval(new RectF(-12, -12, 12, 20), paint);
                    canvas.drawRoundRect(new RectF(-5, -19, 6, -12), 2, 2, paint);
                    canvas.drawArc(new RectF(3, -23, 16, -11), 175, 175, false, paint);
                    line(canvas, -11, 0, 11, 0); line(canvas, 0, -10, 0, 18);
                }
                break;
            }
            case 3: // SWITCH WEAPON
                line(canvas, -19, -9, 18, -9); line(canvas, 18, -9, 11, -15);
                line(canvas, 18, -9, 11, -3);
                line(canvas, 19, 9, -18, 9); line(canvas, -18, 9, -11, 3);
                line(canvas, -18, 9, -11, 15);
                break;
            case 4: { // MELEE: original monochrome melee icon
                Bitmap melee = getMeleeIconBitmap();
                if (melee != null) {
                    paint.setStyle(Paint.Style.FILL);
                    paint.setColor(alpha(Color.WHITE, Math.max(0.55f, opacity)));
                    float maxW = 58f;
                    float maxH = 58f;
                    float bw = melee.getWidth();
                    float bh = melee.getHeight();
                    float bitmapScale = Math.min(maxW / bw, maxH / bh);
                    float dx = bw * bitmapScale * 0.5f;
                    float dy = bh * bitmapScale * 0.5f;
                    RectF dst = new RectF(-dx, -dy, dx, dy);
                    canvas.drawBitmap(melee, null, dst, paint);
                    paint.setColor(alpha(Color.WHITE, Math.max(0.55f, opacity)));
                    paint.setStyle(Paint.Style.STROKE);
                } else {
                    canvas.drawCircle(0, 0, 5, paint);
                    for (int i = 0; i < 8; i++) {
                        double a = Math.PI * 2 * i / 8;
                        line(canvas, (float)Math.cos(a) * 10, (float)Math.sin(a) * 10,
                            (float)Math.cos(a) * 21, (float)Math.sin(a) * 21);
                    }
                }
                break;
            }
            case 5: // FLASHLIGHT
                canvas.drawRect(-18, -7, -4, 8, paint);
                line(canvas, -4, -5, 19, -18); line(canvas, -4, 6, 19, 19);
                line(canvas, 19, -18, 19, 19);
                break;
            case 1: // GRENADE TYPE
                canvas.drawCircle(-7, 4, 9, paint); canvas.drawCircle(10, -3, 8, paint);
                canvas.drawArc(new RectF(-22, -22, 22, 22), 200, 135, false, paint);
                break;
            default: // Leave uncommon inputs (menu/system actions) recognizable.
                canvas.restore();
                circle(canvas, x, y, radius, label(type), active, 11, opacity);
                return;
        }
        canvas.restore();
    }

    private void line(Canvas canvas, float x1, float y1, float x2, float y2) {
        canvas.drawLine(x1, y1, x2, y2, paint);
    }

    /** a toolbar item: never fainter than half, so that it can always be found */
    private void toolbarItem(Canvas canvas, float x, float radius, String label) {
        circle(canvas, x, toolbarY(), toolbarRadius(radius), label, false, 11, Math.max(layout.opacity, 0.5f));
    }

    private void drawToolbar(Canvas canvas) {
        if (editing) {
            toolbarItem(canvas, optionsX(), OPTIONS_RADIUS, "Save");
            toolbarItem(canvas, optionsX() - 156, OPTIONS_RADIUS, "Export");
            toolbarItem(canvas, optionsX() - 78, OPTIONS_RADIUS, "Import");
            paint.setColor(Color.WHITE);
            paint.setTextSize(14);
            canvas.drawText("Drag the controls, then push Save", logicalWidth / 2, toolbarY() + 58, paint);
        } else {
            toolbarItem(canvas, logicalWidth / 2, TOOLBAR_RADIUS, hidden ? "Touch" : "Hide");
            toolbarItem(canvas, optionsX(), OPTIONS_RADIUS, "Options");
        }
    }

    private void drawStick(Canvas canvas) {
        int stick = TouchLayout.LEFT;
        float radius = layout.radius(stick);
        float size = layout.sizeScale(stick);
        // (a floating stick near an edge is drawn whole, nearer the middle than its centre)
        float x = Math.max(radius, Math.min(logicalWidth - radius, stickCenterX()));
        float y = Math.max(radius, Math.min(logicalHeight - radius, stickCenterY()));
        circle(canvas, x, y, radius, "Move", false, 11 * size, layout.opacity);
        circle(canvas, x + axes[0] / 32767f * radius, y + axes[1] / 32767f * radius,
            Math.max(STICK_KNOB_RADIUS * size, radius * 0.375f), "", held(stick), 11, layout.opacity);
        // Only draw the lock while the same thumb is PUSHING FORWARD.
        // Once engaged, the temporary padlock disappears; no text overlays.
        if (modernUi && showing && !hidden && !editing && !vehicleOccupied()
                && autoWalkHintVisible && !autoWalkLocked
                && autoWalkHintPointer >= 0
                && owners.get(autoWalkHintPointer, NOTHING) == stick) {
            Paint saved = new Paint(paint);
            float cx = autoWalkLockX(), cy = autoWalkLockY();
            float r = Math.max(18f, radius * AUTO_WALK_LOCK_RADIUS);
            paint.setStyle(Paint.Style.FILL);
            paint.setColor(alpha(0xB21A3646, layout.opacity));
            canvas.drawCircle(cx, cy, r, paint);
            paint.setStyle(Paint.Style.STROKE);
            paint.setStrokeWidth(2.2f);
            paint.setColor(alpha(autoWalkLockReady ? 0xFFB8F7FF : 0xFF8CE4FF,
                layout.opacity));
            canvas.drawCircle(cx, cy, r, paint);
            // Bright ring means RELEASE HERE to engage; no new asset.
            if (autoWalkLockReady) {
                paint.setStrokeWidth(3.7f);
                canvas.drawCircle(cx, cy, r - 3f, paint);
            }
            // Simple unlocked-padlock glyph. No PNG or extra touch button.
            paint.setColor(alpha(Color.WHITE, layout.opacity));
            paint.setStrokeWidth(2.6f);
            float w = r * 0.75f;
            RectF body = new RectF(cx - w / 2f, cy - r * 0.06f,
                cx + w / 2f, cy + r * 0.62f);
            canvas.drawRoundRect(body, 2.5f, 2.5f, paint);
            RectF shackle = new RectF(cx - r * 0.27f, cy - r * 0.49f,
                cx + r * 0.27f, cy + r * 0.23f);
            canvas.drawArc(shackle, 180f, 180f, false, paint);
            paint.set(saved);
        }
    }

    /** White, compact gun silhouettes drawn from the engine's weapon IDs. */
    private void drawWeaponSilhouette(Canvas canvas, int type, float x, float y) {
        paint.setColor(alpha(Color.WHITE, layout.opacity));
        paint.setStrokeWidth(2.3f);
        paint.setStrokeCap(Paint.Cap.ROUND);
        paint.setStyle(Paint.Style.STROKE);
        float length = (type == 2 || type == 3) ? 31f : (type == 8 ? 67f : 54f);
        float left = x - length / 2f;
        canvas.drawLine(left, y, left + length, y, paint);
        canvas.drawLine(left + 5f, y - 6f, left + length - 8f, y - 6f, paint);
        canvas.drawLine(left + 11f, y, left + 8f, y + 12f, paint);
        canvas.drawLine(left + 8f, y + 12f, left + 19f, y + 12f, paint);
        canvas.drawLine(left + length - 11f, y, left + length - 9f, y + 5f, paint);
        if (type == 7) { // sniper optic
            canvas.drawRect(left + 22f, y - 12f, left + 36f, y - 8f, paint);
        } else if (type == 8) { // launcher tube
            canvas.drawRect(left, y - 6f, left + length, y + 1f, paint);
        } else if (type == 5) { // needler spikes
            for (int k = 0; k < 4; k++)
                canvas.drawLine(left + 20f + 7f*k, y - 6f, left + 18f + 7f*k, y - 13f, paint);
        }
        paint.setStyle(Paint.Style.FILL);
    }

    /**
     * Halo's original state message contains the Xbox X glyph. On the modern
     * touch HUD, X is the reload/action control, so cover only that glyph and
     * draw the same reload pictogram used by the on-screen reload button.
     * The native message text and weapon icon stay untouched/localized.
     */
    private void drawWeaponPickupReloadPrompt(Canvas canvas) {
        // VEHICLE_FLIP_RELOAD_PROMPT_V18: original Reload icon for flip,
        // normal on-foot weapon pickup artwork unchanged.
        if (!modernUi || hidden || editing ||
            (!hasWeaponPickupPrompt() && !vehicleFlipPrompt())) return;

        // Halo's state-message button is left anchored. Touch logical units
        // already scale from screen height, matching the native HUD. The
        // 1600x720 capture places the X at ~208,160 physical px = 156,120
        // logical units, and the same constants scale correctly at 540p/1080p.
        boolean flip = vehicleFlipPrompt();
        // The flip X shares Halo's state-message anchor with Enter.
        // Keep the old constants EXACTLY for weapon pickups.
        float x = flip ? logicalWidth * 0.139f : 156f;
        float y = flip ? logicalHeight * 0.214f : 120f;
        float radius = flip ? Math.max(17f, Math.min(24f, logicalHeight * 0.041f)) : 18f;

        // Opaque enough to replace the native blue X disc underneath.
        paint.setStyle(Paint.Style.FILL);
        // Flip needs an opaque cover for the native blue Xbox X glyph.
        paint.setColor(flip ? 0xFF19344B : alpha(0xE6313C43, layout.opacity));
        canvas.drawCircle(x, y, radius, paint);
        paint.setStyle(Paint.Style.STROKE);
        paint.setStrokeWidth(1.6f);
        paint.setColor(alpha(Color.WHITE, Math.max(0.72f, layout.opacity)));
        canvas.drawCircle(x, y, radius, paint);

        canvas.save();
        canvas.translate(x, y);
        float s = radius / 42f;
        canvas.scale(s, s);
        paint.setStrokeWidth(3.0f);
        paint.setStrokeCap(Paint.Cap.ROUND);
        paint.setStrokeJoin(Paint.Join.ROUND);
        paint.setColor(alpha(Color.WHITE, Math.max(0.72f, layout.opacity)));
        paint.setStyle(Paint.Style.STROKE);
        Path path = new Path();
        canvas.drawArc(new RectF(-21, -21, 21, 21), 208, 140, false, paint);
        canvas.drawArc(new RectF(-21, -21, 21, 21), 27, 140, false, paint);
        path.moveTo(18, -17); path.lineTo(22, -5); path.lineTo(10, -9);
        canvas.drawPath(path, paint);
        path.reset();
        path.moveTo(-18, 17); path.lineTo(-22, 5); path.lineTo(-10, 9);
        canvas.drawPath(path, paint);
        paint.setStyle(Paint.Style.FILL);
        for (int k = -1; k <= 1; k++) {
            float bx = k * 9f;
            canvas.drawRoundRect(new RectF(bx - 3, -10, bx + 3, 10), 1.2f, 1.2f, paint);
            path.reset();
            path.moveTo(bx - 3, -10);
            path.lineTo(bx, -15);
            path.lineTo(bx + 3, -10);
            path.close();
            canvas.drawPath(path, paint);
        }
        canvas.restore();
        paint.setStyle(Paint.Style.FILL);
    }

    private void drawWeaponDrawerArrow(Canvas canvas) {
        RectF arrow = weaponDrawerArrowFrame();
        paint.setStyle(Paint.Style.FILL);
        paint.setColor(alpha(0x26202930, layout.opacity));
        canvas.drawRoundRect(arrow, 3f, 3f, paint);
        paint.setStyle(Paint.Style.STROKE);
        paint.setStrokeWidth(1.3f);
        paint.setColor(alpha(0xFFCAD1D4, layout.opacity));
        canvas.drawRoundRect(arrow, 3f, 3f, paint);
        float cx = arrow.centerX(), cy = arrow.centerY();
        float apex = cy + (weaponDrawerExpanded ? 4f : -4f);
        float ends = cy + (weaponDrawerExpanded ? -4f : 4f);
        paint.setStrokeWidth(2.5f);
        canvas.drawLine(cx - 8f, ends, cx, apex, paint);
        canvas.drawLine(cx, apex, cx + 8f, ends, paint);
        paint.setStyle(Paint.Style.FILL);
    }

    /** Gameplay only; original Halo ammo, shields and radar remain untouched. */
    private void drawWeaponSlotFrames(Canvas canvas) {
        if (!modernUi || hidden || editing) return;
        if (hasExtraWeapons())
            drawWeaponDrawerArrow(canvas);
        if (weaponDrawerExpanded && hasExtraWeapons()) {
            for (int slot = 2; slot < 4; slot++) {
                RectF frame = weaponFrame(slot);
                paint.setStyle(Paint.Style.FILL);
                paint.setColor(alpha(0x26202930, layout.opacity));
                canvas.drawRoundRect(frame, 2f, 2f, paint);
                paint.setStyle(Paint.Style.STROKE);
                paint.setStrokeWidth(weaponSlots[2] == weaponViewOrder[slot] ? 2f : 1.1f);
                paint.setColor(alpha(weaponSlots[2] == weaponViewOrder[slot]
                    ? 0xFFFFD64B : 0xFF8E9CA3, layout.opacity));
                canvas.drawRoundRect(frame, 2f, 2f, paint);
                paint.setStyle(Paint.Style.FILL);
                paint.setTextAlign(Paint.Align.LEFT);
                paint.setTextSize(10f);
                canvas.drawText("0" + (slot + 1),
                    frame.left + 6f, frame.top + 12f, paint);
            }
        }
        for (int slot = 0; slot < 2; slot++) {
            RectF frame = weaponFrame(slot);
            boolean active = weaponSlots[2] == weaponViewOrder[slot];
            boolean equipped = hasWeaponAtFrame(slot);
            paint.setStyle(Paint.Style.FILL);
            paint.setColor(alpha(0x26202930, layout.opacity));
            canvas.drawRoundRect(frame, 2f, 2f, paint);
            paint.setStyle(Paint.Style.STROKE);
            paint.setStrokeWidth(active ? 2.0f : 1.1f);
            paint.setColor(alpha(active ? 0xFFFFD64B : 0xFFCAD1D4, layout.opacity));
            canvas.drawRoundRect(frame, 2f, 2f, paint);
            paint.setStyle(Paint.Style.FILL);
            if (equipped) {
                // Original Halo messaging icon is drawn by the native HUD underneath.
                paint.setColor(alpha(Color.WHITE, layout.opacity));
            } else {
                paint.setColor(alpha(0xFF8E9CA3, layout.opacity));
            }
            paint.setTextAlign(Paint.Align.LEFT);
            paint.setTextSize(10f);
            canvas.drawText("0" + (slot + 1), frame.left + 6f, frame.top + 12f, paint);
        }
        // Highlight the destination frame during drag; the original icons
        // remain rendered by the native HUD in their current positions.
        for (int i = 0; i < slotFingers.size(); i++) {
            int source = slotFingers.valueAt(i);
            if (source < 0 || !hasWeaponAtFrame(source)) continue;
            float[] position = slotPositions.get(slotFingers.keyAt(i));
            if (position == null) continue;
            int destination = frameAt(position[0], position[1]);
            if (source < 2 || destination < 0 || destination >= 2 ||
                    !WeaponSlotOrder.canMove(weaponViewOrder, physicalWeaponMask(),
                        source, destination)) continue;
            paint.setStyle(Paint.Style.STROKE);
            paint.setStrokeWidth(2.5f);
            paint.setColor(alpha(0xFF59DCC6, layout.opacity));
            canvas.drawRoundRect(weaponFrame(destination), 3f, 3f, paint);
        }
        paint.setStyle(Paint.Style.FILL);
    }

    @Override
    protected void onDraw(Canvas canvas) {
        if (!showing && !editing)
            return;
        canvas.save();
        canvas.scale(scale, scale);
        if (!hidden || editing) {
            if (layout.shown(TouchLayout.LEFT) && vehicleControlVisible(TouchLayout.LEFT))
                drawStick(canvas);
            for (int i = 0; i < layout.size(); i++) {
                if (!layout.shown(i) || layout.type(i) == TouchLayout.LEFT
                        || redundantVehicleReload(i) || !vehicleControlVisible(i)
                        || (modernUi && isGameplayDpad(layout.type(i)))
                        || (modernUi && !editing && isWeaponSwitchControl(layout.type(i))))
                    continue;
                if (!modernUi) {
                    float textSize = 11 * Math.max(layout.sizeScale(i), layout.radius(i) / 36f);
                    circle(canvas, layout.x(i), layout.y(i), layout.radius(i), label(layout.type(i)),
                        held(i) || dragControl == i, Math.min(textSize, 16), layout.opacity);
                } else tacticalButton(canvas, layout.x(i), layout.y(i), layout.radius(i), layout.type(i),
                    held(i) || dragControl == i, layout.opacity);
            }
        }
        if (!vehicleOccupied())
            drawWeaponSlotFrames(canvas);
        drawWeaponPickupReloadPrompt(canvas);
        drawVehicleEnterButton(canvas);
        drawVehiclePromptWheel(canvas);
        drawToolbar(canvas);
        canvas.restore();
    }
}
