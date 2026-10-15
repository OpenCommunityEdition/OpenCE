package com.halo.decomp;

import android.content.Context;
import android.content.SharedPreferences;
import android.content.pm.PackageManager;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Paint;
import android.graphics.RadialGradient;
import android.graphics.Rect;
import android.graphics.Shader;
import android.graphics.Typeface;
import android.graphics.drawable.BitmapDrawable;
import android.graphics.drawable.Drawable;
import android.hardware.Sensor;
import android.hardware.SensorEvent;
import android.hardware.SensorEventListener;
import android.hardware.SensorManager;
import android.hardware.input.InputManager;
import android.media.AudioAttributes;
import android.os.Build;
import android.os.Handler;
import android.os.Looper;
import android.os.SystemClock;
import android.os.VibrationEffect;
import android.os.Vibrator;
import android.os.VibratorManager;
import android.util.SparseArray;
import android.util.SparseBooleanArray;
import android.util.SparseIntArray;
import android.view.InputDevice;
import android.view.KeyEvent;
import android.view.MotionEvent;
import android.view.View;
import android.view.WindowManager;
import android.widget.FrameLayout;
import android.widget.Toast;

import org.libsdl.app.SDLActivity;

import java.io.File;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collections;
import java.util.HashMap;
import java.util.List;
import java.util.Locale;
import java.util.Map;

public final class TouchControls extends FrameLayout implements SensorEventListener, InputManager.InputDeviceListener {
    private static final int LOOK = -5;
    private static final int NOTHING = Integer.MIN_VALUE;

    private static final float GRID_WIDTH = 960;
    private static final float GRID_HEIGHT = 540;
    private static final float MINIMUM_RADIUS_DP = 24;
    private static final float SLOP = 10;
    private static final float STICK_DEAD_ZONE = 0.12f;
    private static final float STICK_REACH = 1.28f;
    private static final float STICK_KNOB_RADIUS = 26;
    private static final float FLOATING_ZONE_WIDTH = 0.45f;
    private static final float FLOATING_ZONE_TOP = 0.35f;
    private static final float LOOK_RADIANS_PER_UNIT = 0.0022f;
    private static final long POLL_MS = 16;
    private static final long SHOW_DELAY_MS = 200;
    private static final long VIBRATION_MS = 110;
    private static final long VIBRATION_RENEW_MS = 70;
    private static final long LONG_PRESS_TIMEOUT_MS = 280;
    private static final long EDIT_MODE_HOLD_MS = 450;
    private static final long TAP_PULSE_MS = 50;

    private static final int SCENE_KNOWN = 1;
    private static final int SCENE_MENUS = 2;
    private static final int SCENE_ON = 4;
    private static final int SCENE_OFF = 8;

    private static final String PREFERENCES = "touch-layout-v1";
    private static final String KEY_CONFIGURATION = "configuration";
    private static final String KEY_HIDDEN = "hidden";

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

    private static final String[] GAME_CONTROLS = {
        "Jump", "Grenade Type", "Reload", "Weapon", "Melee", "Light",
        "Grenade", "Fire", "Pause", "Back", "Crouch", "Zoom",
    };

    private static final int[] DEFAULT_BINDINGS = {
        0, 4, 2, 3, 1, 5, 6, 7, -1, -1, -1, -1, 8, 9, 10, 11,
    };

    private static native void nativeState(int lx, int ly, int rx, int ry, int lt, int rt, int buttons);
    private static native void nativeLook(float dx, float dy);
    private static native void nativeGyro(float dx, float dy);
    private static native void nativeLookReset();
    private static native int nativeRumble();
    private static native int nativeScene();
    private static native int nativeBindings(int[] controls);

    private final Paint paint = new Paint(Paint.ANTI_ALIAS_FLAG);
    private final Paint textPaint = new Paint(Paint.ANTI_ALIAS_FLAG);
    private final Paint outlinePaint = new Paint(Paint.ANTI_ALIAS_FLAG);
    private final Handler mainHandler = new Handler(Looper.getMainLooper());
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
    private TouchSettingsPanel settingsPanel;
    private float sensitivity = 1f;
    private boolean hidden;
    private boolean editing;
    private int selectedControl = -1;

    private final SparseIntArray owners = new SparseIntArray();
    private final SparseArray<float[]> buttonTouches = new SparseArray<>();
    private final SparseBooleanArray toggledStates = new SparseBooleanArray();
    private final SparseBooleanArray longPressActive = new SparseBooleanArray();
    private final SparseArray<Runnable> pendingLongPress = new SparseArray<>();
    private final SparseBooleanArray tapPulseActive = new SparseBooleanArray();

    private Runnable pendingEditOpenRunnable = null;
    private boolean editOpenTriggered = false;

    private final Map<String, Drawable> iconCache = new HashMap<>();
    private Drawable stickBaseDrawable;
    private Drawable stickKnobDrawable;

    private final int[] axes = new int[6];
    private int lookPointer = -1;
    private float lookX;
    private float lookY;
    private boolean stickFloating;
    private float stickX;
    private float stickY;
    private int dragPointer = -1;
    private int dragControl = -1;
    private float dragOffsetX;
    private float dragOffsetY;

    private float scale = 1;
    private float logicalWidth = GRID_WIDTH;
    private float logicalHeight = GRID_HEIGHT;
    private int insetRight;
    private int insetTop;

    private int scene;
    private boolean controllerConnected;
    private boolean touchscreen;
    private boolean deviceInputActive;
    private boolean showing;
    private long shownSince = -1;
    private boolean gyroRegistered;
    private int lastAmplitude;
    private long lastVibration;
    private final int[] bindings = DEFAULT_BINDINGS.clone();
    private final int[] bindingsRead = new int[bindings.length];
    private int bindingsSerial;

    private final Runnable poll = new Runnable() {
        @Override
        public void run() {
            if (!deviceInputActive)
                return;
            scene = nativeScene();
            updateShowing();
            readBindings();
            updateRumble();
            postDelayed(this, POLL_MS);
        }
    };

    public TouchControls(Context context) {
        super(context);
        setWillNotDraw(false);
        textPaint.setAntiAlias(true);
        textPaint.setTypeface(Typeface.create(Typeface.DEFAULT, Typeface.BOLD));
        textPaint.setTextAlign(Paint.Align.CENTER);
        outlinePaint.setAntiAlias(true);
        outlinePaint.setStyle(Paint.Style.STROKE);

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
        loadLayout(context);
        hidden = preferences.getBoolean(KEY_HIDDEN, false);
        refreshInputDevices();
        refreshAllDrawables();

        settingsPanel = new TouchSettingsPanel(context, this);
        settingsPanel.setVisibility(GONE);
        addView(settingsPanel);

        setFocusable(false);
        setContentDescription("Halo touch controller");
        setOnApplyWindowInsetsListener((view, insets) -> {
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

    private void loadLayout(Context context) {
        String configuration = preferences.getString(KEY_CONFIGURATION, null);
        if (configuration == null)
            return;
        try {
            TouchLayout.Configuration saved = TouchLayout.importConfiguration(configuration);
            layout = saved.layout;
            sensitivity = saved.sensitivity;
        } catch (IllegalArgumentException e) {
            Toast.makeText(context, "Saved touch layout corrupted. Using default.", Toast.LENGTH_LONG).show();
        }
    }

    public TouchLayout getLayout() { return layout; }
    public int getSelectedControl() { return selectedControl; }

    public void selectControl(int idx) {
        this.selectedControl = idx;
        if (settingsPanel != null) {
            settingsPanel.bindControl(idx);
        }
        invalidate();
    }

    public float getSensitivity() { return sensitivity; }

    public void setSensitivity(float s) {
        this.sensitivity = s;
        saveAndInvalidate();
    }

    public boolean isTouchControlsHidden() { return hidden; }
    public void setTouchControlsHidden(boolean hide) { setHidden(hide); }

    public void updateSensorsRuntime() {
        updateSensors();
    }

    public void saveAndInvalidate() {
        saveLayout();
        updateSensors();
        invalidate();
    }

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

    public boolean isEditing() { return editing; }

    public void startEditing() {
        editing = true;
        setHidden(false);
        selectedControl = -1;
        settingsPanel.setVisibility(VISIBLE);
        settingsPanel.bindControl(-1);
        updateSensors();
        invalidate();
    }

    public void stopEditing() {
        if (editing) {
            saveLayout();
            reset();
            editing = false;
            selectedControl = -1;
            settingsPanel.setVisibility(GONE);
            settingsPanel.bindControl(-1);
            updateSensors();
            invalidate();
        }
    }

    @Override
    protected void onSizeChanged(int width, int height, int oldWidth, int oldHeight) {
        super.onSizeChanged(width, height, oldWidth, oldHeight);
        layoutControls();
        requestApplyInsets();
    }

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

    private float minimumRadius() {
        float radius = MINIMUM_RADIUS_DP * getResources().getDisplayMetrics().density / scale;
        return Math.min(radius, TouchLayout.MAX_MINIMUM_RADIUS);
    }

    private float logical(float pixels) { return pixels / scale; }

    private boolean shown() {
        if ((scene & SCENE_KNOWN) == 0 || (scene & (SCENE_MENUS | SCENE_OFF)) != 0 || !touchscreen)
            return false;
        return (scene & SCENE_ON) != 0 || !controllerConnected;
    }

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
            reset();
            updateSensors();
        }
    }

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

    @Override public void onInputDeviceAdded(int deviceId) { refreshInputDevices(); }
    @Override public void onInputDeviceRemoved(int deviceId) { refreshInputDevices(); }
    @Override public void onInputDeviceChanged(int deviceId) { refreshInputDevices(); }

    private void setHidden(boolean hidden) {
        reset();
        this.hidden = hidden;
        preferences.edit().putBoolean(KEY_HIDDEN, hidden).apply();
        updateSensors();
    }

    public void reset() {
        dragPointer = -1;
        dragControl = -1;
        owners.clear();
        buttonTouches.clear();
        for (int i = 0; i < pendingLongPress.size(); i++)
            mainHandler.removeCallbacks(pendingLongPress.valueAt(i));
        pendingLongPress.clear();
        longPressActive.clear();
        tapPulseActive.clear();

        if (pendingEditOpenRunnable != null) {
            mainHandler.removeCallbacks(pendingEditOpenRunnable);
            pendingEditOpenRunnable = null;
        }
        editOpenTriggered = false;

        lookPointer = -1;
        stickFloating = false;
        gyroAim.reset();
        nativeLookReset();
        Arrays.fill(axes, 0);
        publish();
        invalidate();
    }

    private void publish() {
        if (editing) {
            nativeState(0, 0, 0, 0, 0, 0, 0);
            return;
        }
        int bits = 0;
        axes[4] = 0;
        axes[5] = 0;

        for (int i = 0; i < layout.size(); i++) {
            if (!layout.shown(i) || layout.type(i) == TouchLayout.LEFT || layout.type(i) == TouchLayout.TB_HIDE)
                continue;

            int activeKind = -1;
            int mode = layout.mode(i);
            boolean isHeld = held(i);

            if (mode == TouchLayout.MODE_TOGGLE) {
                if (toggledStates.get(i, false))
                    activeKind = layout.type(i);
            } else if (mode == TouchLayout.MODE_TAP_AND_HOLD) {
                if (longPressActive.get(i, false)) {
                    activeKind = layout.longType(i) >= 0 ? layout.longType(i) : -1;
                } else if (tapPulseActive.get(i, false)) {
                    activeKind = layout.type(i);
                }
            } else {
                if (isHeld)
                    activeKind = layout.type(i);
            }

            if (activeKind >= 0 && activeKind < KINDS.length) {
                Kind k = KINDS[activeKind];
                if (k.bit >= 0)
                    bits |= 1 << k.bit;
                if (k.trigger >= 0)
                    axes[k.trigger] = 32767;
            }
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

    private static boolean inside(float x, float y, float cx, float cy, float radius) {
        return (x - cx) * (x - cx) + (y - cy) * (y - cy) <= radius * radius;
    }

    private int hit(float x, float y) {
        if (hidden && !editing) {
            for (int i = 0; i < layout.size(); i++) {
                if (layout.type(i) == TouchLayout.TB_HIDE && layout.shown(i)) {
                    if (inside(x, y, layout.x(i), layout.y(i), layout.radius(i)))
                        return i;
                }
            }
            return NOTHING;
        }

        for (int i = layout.size() - 1; i >= 0; i--) {
            if (!layout.shown(i)) continue;
            int t = layout.type(i);

            if (t == TouchLayout.LEFT) {
                float reach = editing ? 1f : STICK_REACH;
                if (!held(i) && inside(x, y, layout.x(i), layout.y(i), layout.radius(i) * reach))
                    return i;
            } else {
                if (inside(x, y, layout.x(i), layout.y(i), layout.radius(i)))
                    return i;
            }
        }
        return NOTHING;
    }

    private boolean inGestureZone(float px, float py, boolean sidesOnly) {
        int[] insets = ((HaloActivity) getContext()).getSystemGestureInsetsPixels();
        if (px < insets[0] || px >= getWidth() - insets[2])
            return true;
        return !sidesOnly && (py < insets[1] || py >= getHeight() - insets[3]);
    }

    @Override
    public boolean onInterceptTouchEvent(MotionEvent event) {
        if (settingsPanel.getVisibility() == VISIBLE) {
            Rect rect = new Rect();
            settingsPanel.getHitRect(rect);
            if (rect.contains((int) event.getX(), (int) event.getY())) {
                return false;
            }
        }
        return super.onInterceptTouchEvent(event);
    }

    @Override
    public boolean onTouchEvent(MotionEvent event) {
        int action = event.getActionMasked();
        if (!showing && !editing) {
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
        int index = event.getActionIndex();
        int id = event.getPointerId(index);
        switch (event.getActionMasked()) {
            case MotionEvent.ACTION_DOWN:
            case MotionEvent.ACTION_POINTER_DOWN:
                fingerDown(id, event.getX(index), event.getY(index));
                break;
            case MotionEvent.ACTION_MOVE:
                for (int i = 0; i < event.getPointerCount(); i++)
                    fingerMoved(event.getPointerId(i), event.getX(i), event.getY(i));
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

    private void fingerDown(int id, float px, float py) {
        if (inGestureZone(px, py, true))
            return;
        float x = logical(px);
        float y = logical(py);
        int target = hit(x, y);

        int stick = TouchLayout.LEFT;
        if (target == NOTHING && layout.floatingStick && !hidden && layout.shown(stick) && !held(stick)
                && inFloatingZone(x, y) && !inGestureZone(px, py, false)) {
            floatStick(x, y);
            target = stick;
        }

        if (target != NOTHING) {
            int type = layout.type(target);
            if (type == TouchLayout.TB_HIDE) {
                owners.put(id, target);
                buttonTouches.put(id, new float[] {px, py});
                editOpenTriggered = false;
                pendingEditOpenRunnable = () -> {
                    editOpenTriggered = true;
                    startEditing();
                    owners.delete(id);
                    buttonTouches.remove(id);
                };
                mainHandler.postDelayed(pendingEditOpenRunnable, EDIT_MODE_HOLD_MS);
                return;
            }

            owners.put(id, target);
            if (type == TouchLayout.LEFT) {
                moveStick(target, x, y);
            } else {
                buttonTouches.put(id, new float[] {px, py});
                handleControlPress(target);
            }
        } else if (!hidden && lookPointer < 0 && !inGestureZone(px, py, false)) {
            startLook(id, px, py);
        }
    }

    private void handleControlPress(final int control) {
        int mode = layout.mode(control);
        if (mode == TouchLayout.MODE_TOGGLE) {
            boolean current = toggledStates.get(control, false);
            toggledStates.put(control, !current);
            int ck = layout.customKeycode(control);
            if (ck > 0) {
                if (!current) SDLActivity.onNativeKeyDown(ck);
                else SDLActivity.onNativeKeyUp(ck);
            }
        } else if (mode == TouchLayout.MODE_TAP_AND_HOLD) {
            longPressActive.put(control, false);
            tapPulseActive.put(control, false);
            Runnable r = () -> {
                longPressActive.put(control, true);
                int lk = layout.longKeycode(control);
                if (lk > 0) SDLActivity.onNativeKeyDown(lk);
                publish();
                invalidate();
            };
            pendingLongPress.put(control, r);
            mainHandler.postDelayed(r, LONG_PRESS_TIMEOUT_MS);
        } else {
            int ck = layout.customKeycode(control);
            if (ck > 0) SDLActivity.onNativeKeyDown(ck);
        }
    }

    private void handleControlRelease(final int control) {
        int mode = layout.mode(control);
        if (mode == TouchLayout.MODE_TAP_AND_HOLD) {
            Runnable r = pendingLongPress.get(control);
            if (r != null) {
                mainHandler.removeCallbacks(r);
                pendingLongPress.remove(control);
            }

            if (longPressActive.get(control, false)) {
                int lk = layout.longKeycode(control);
                if (lk > 0) SDLActivity.onNativeKeyUp(lk);
                longPressActive.delete(control);
            } else {
                tapPulseActive.put(control, true);
                int ck = layout.customKeycode(control);
                if (ck > 0) SDLActivity.onNativeKeyDown(ck);
                publish();
                invalidate();

                mainHandler.postDelayed(() -> {
                    tapPulseActive.delete(control);
                    if (ck > 0) SDLActivity.onNativeKeyUp(ck);
                    publish();
                    invalidate();
                }, TAP_PULSE_MS);
            }
        } else if (mode == TouchLayout.MODE_STANDARD) {
            int ck = layout.customKeycode(control);
            if (ck > 0) SDLActivity.onNativeKeyUp(ck);
        }
    }

    private void startLook(int id, float px, float py) {
        lookPointer = id;
        lookX = px;
        lookY = py;
        if (owners.get(id, NOTHING) == NOTHING)
            owners.put(id, LOOK);
    }

    private boolean movedPastSlop(float[] origin, float px, float py) {
        return Math.hypot(px - origin[0], py - origin[1]) > SLOP * scale;
    }

    private void fingerMoved(int id, float px, float py) {
        int control = owners.get(id, NOTHING);
        if (control >= 0 && layout.type(control) == TouchLayout.LEFT)
            moveStick(control, logical(px), logical(py));

        float[] origin = buttonTouches.get(id);
        if (control >= 0 && layout.type(control) == TouchLayout.TB_HIDE && origin != null) {
            if (movedPastSlop(origin, px, py) && pendingEditOpenRunnable != null) {
                mainHandler.removeCallbacks(pendingEditOpenRunnable);
                pendingEditOpenRunnable = null;
            }
        }

        if (origin != null && lookPointer >= 0 && lookPointer != id) {
            origin[0] = px;
            origin[1] = py;
        }
        if (lookPointer < 0 && origin != null && movedPastSlop(origin, px, py)) {
            lookPointer = id;
            lookX = origin[0];
            lookY = origin[1];
        }
        if (id == lookPointer) {
            nativeLook(logical(px - lookX) * sensitivity, logical(py - lookY) * sensitivity);
            lookX = px;
            lookY = py;
        }
    }

    private void fingerUp(int id, float px, float py) {
        int control = owners.get(id, NOTHING);
        if (control >= 0) {
            int type = layout.type(control);
            if (type == TouchLayout.TB_HIDE) {
                if (pendingEditOpenRunnable != null) {
                    mainHandler.removeCallbacks(pendingEditOpenRunnable);
                    pendingEditOpenRunnable = null;
                }
                if (!editOpenTriggered) {
                    setHidden(!hidden);
                }
                owners.delete(id);
                buttonTouches.remove(id);
                return;
            }

            if (type == TouchLayout.LEFT) {
                axes[0] = 0;
                axes[1] = 0;
                stickFloating = false;
            } else {
                handleControlRelease(control);
            }
        }
        if (id == lookPointer)
            lookPointer = -1;
        owners.delete(id);
        buttonTouches.remove(id);
    }

    private void editTouch(MotionEvent event) {
        int action = event.getActionMasked();
        int index = event.getActionIndex();
        int id = event.getPointerId(index);

        if (action == MotionEvent.ACTION_DOWN || action == MotionEvent.ACTION_POINTER_DOWN) {
            if (dragPointer >= 0) return;
            float x = logical(event.getX(index));
            float y = logical(event.getY(index));
            int target = hit(x, y);

            if (target >= 0) {
                selectedControl = target;
                settingsPanel.bindControl(target);

                dragPointer = id;
                dragControl = target;
                dragOffsetX = x - layout.x(target);
                dragOffsetY = y - layout.y(target);
                invalidate();
            } else {
                selectedControl = -1;
                settingsPanel.bindControl(-1);
                invalidate();
            }
        } else if (action == MotionEvent.ACTION_MOVE && dragPointer >= 0) {
            int pointer = event.findPointerIndex(dragPointer);
            if (pointer >= 0) {
                layout.move(dragControl, logical(event.getX(pointer)) - dragOffsetX,
                    logical(event.getY(pointer)) - dragOffsetY);
                invalidate();
            }
        } else if (action == MotionEvent.ACTION_CANCEL
                || ((action == MotionEvent.ACTION_UP || action == MotionEvent.ACTION_POINTER_UP) && id == dragPointer)) {
            dragPointer = -1;
            dragControl = -1;
            saveLayout();
        }
    }

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

    private String label(int type) {
        if (type < 0) return "";
        if (type == TouchLayout.TB_HIDE) return "Controls Toggle";
        if (type == TouchLayout.TYPE_CUSTOM) return "Custom";

        if (type >= KINDS.length) return "";
        Kind kind = KINDS[type];
        if (kind.gamepadButton < 0)
            return kind.buttonName;
        int control = bindings[kind.gamepadButton];
        if (control < 0 || control >= GAME_CONTROLS.length)
            return kind.buttonName;
        return kind.faceButton ? kind.buttonName + " / " + GAME_CONTROLS[control] : GAME_CONTROLS[control];
    }

    public String controlName(int type) {
        if (type == TouchLayout.LEFT) return "Move Stick";
        return label(type);
    }

    private String buttonDisplayLabel(int control) {
        String custom = layout.customLabel(control);
        if (custom != null && !custom.trim().isEmpty()) {
            return custom.trim();
        }
        return label(layout.type(control));
    }

    private boolean saveLayout() {
        boolean saved = preferences.edit().putString(KEY_CONFIGURATION, exportLayout()).commit();
        if (!saved)
            Toast.makeText(getContext(), "Touch layout could not be saved.", Toast.LENGTH_LONG).show();
        return saved;
    }

    public String exportLayout() {
        return layout.exportConfiguration(sensitivity);
    }

    public void importLayout(String text) {
        TouchLayout.Configuration imported = TouchLayout.importConfiguration(text);
        imported.layout.bounds(logicalWidth, logicalHeight);
        imported.layout.setMinimumRadius(minimumRadius());
        String normalized = imported.layout.exportConfiguration(imported.sensitivity);
        if (!preferences.edit().putString(KEY_CONFIGURATION, normalized).commit())
            throw new IllegalArgumentException("Could not save imported layout");
        reset();
        layout = imported.layout;
        sensitivity = imported.sensitivity;
        setHidden(false);
        cancelRumble();
        refreshAllDrawables();
        updateSensors();
        invalidate();
    }

    public File getExternalFilesDirectory() {
        File dir = getContext().getExternalFilesDir(null);
        if (dir == null) dir = getContext().getFilesDir();
        return dir;
    }

    public List<String> listCustomExternalIcons() {
        List<String> list = new ArrayList<>();
        File baseDir = getExternalFilesDirectory();
        File iconsDir = new File(baseDir, "icons");
        if (iconsDir.exists() && iconsDir.isDirectory())
            collectImages(iconsDir, list, "icons/");
        Collections.sort(list);
        return list;
    }

    public List<String> listAvailableCustomIcons() {
        List<String> list = new ArrayList<>();
        File baseDir = getExternalFilesDirectory();
        collectImages(baseDir, list, "");
        File iconsDir = new File(baseDir, "icons");
        if (iconsDir.exists() && iconsDir.isDirectory())
            collectImages(iconsDir, list, "icons/");

        String[] builtins = {
            "tc_ic_a", "tc_ic_b", "tc_ic_x", "tc_ic_y",
            "tc_ic_rt", "tc_ic_lt", "tc_ic_ls", "tc_ic_rs",
            "tc_ic_white", "tc_ic_black", "tc_ic_start", "tc_ic_back",
            "tc_ic_hide", "tc_stick_base", "tc_stick_knob"
        };
        for (String b : builtins) {
            int id = getContext().getResources().getIdentifier(b, "drawable", getContext().getPackageName());
            if (id != 0 && !list.contains(b)) {
                list.add(b);
            }
        }

        Collections.sort(list);
        return list;
    }

    private void collectImages(File dir, List<String> result, String prefix) {
        File[] files = dir.listFiles();
        if (files == null) return;
        for (File f : files) {
            if (!f.isDirectory()) {
                String n = f.getName().toLowerCase(Locale.US);
                if (n.endsWith(".png") || n.endsWith(".webp") || n.endsWith(".jpg"))
                    result.add(prefix + f.getName());
            }
        }
    }

    private Drawable loadExternalDrawable(String filename) {
        if (filename == null || filename.trim().isEmpty()) return null;
        if (iconCache.containsKey(filename)) return iconCache.get(filename);

        File baseDir = getExternalFilesDirectory();
        File target = new File(filename.startsWith("/") ? filename : baseDir.getAbsolutePath() + "/" + filename);
        if (!target.exists()) {
            target = new File(baseDir, "icons/" + filename);
        }
        if (target.exists() && target.isFile()) {
            try {
                Bitmap bmp = BitmapFactory.decodeFile(target.getAbsolutePath());
                if (bmp != null) {
                    Drawable d = new BitmapDrawable(getResources(), bmp);
                    iconCache.put(filename, d);
                    return d;
                }
            } catch (Exception ignored) {}
        }

        String resourceName = filename;
        if (resourceName.contains(".")) {
            resourceName = resourceName.substring(0, resourceName.lastIndexOf('.'));
        }
        resourceName = resourceName.toLowerCase(Locale.US).replace("-", "_").replace(" ", "_");

        int resId = getContext().getResources().getIdentifier(resourceName, "drawable", getContext().getPackageName());
        if (resId == 0 && !resourceName.startsWith("tc_ic_")) {
            resId = getContext().getResources().getIdentifier("tc_ic_" + resourceName, "drawable", getContext().getPackageName());
        }

        if (resId != 0) {
            try {
                Drawable d = getContext().getDrawable(resId);
                if (d != null) {
                    d = d.mutate();
                    iconCache.put(filename, d);
                    return d;
                }
            } catch (Exception ignored) {}
        }

        return null;
    }

    public void refreshAllDrawables() {
        iconCache.clear();
        for (int i = 0; i < layout.size(); i++) {
            if (layout.type(i) == TouchLayout.LEFT) {
                stickBaseDrawable = loadExternalDrawable(layout.iconFile(i));
                stickKnobDrawable = loadExternalDrawable(layout.knobFile(i));
                break;
            }
        }
        if (stickBaseDrawable == null) stickBaseDrawable = loadExternalDrawable("tc_stick_base.png");
        if (stickKnobDrawable == null) stickKnobDrawable = loadExternalDrawable("tc_stick_knob.png");
        invalidate();
    }

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
        if (!deviceInputActive || !showing || hidden || !layout.gyroscopeEnabled || editing) {
            gyroAim.reset();
            return;
        }
        int rotation = ((WindowManager) getContext().getSystemService(Context.WINDOW_SERVICE))
            .getDefaultDisplay().getRotation();
        if (gyroAim.sample(event.timestamp, event.values[0], event.values[1], rotation, gyroDelta)) {
            nativeGyro(-gyroDelta[0] / LOOK_RADIANS_PER_UNIT * sensitivity,
                -gyroDelta[1] / LOOK_RADIANS_PER_UNIT * sensitivity);
        }
    }

    @Override public void onAccuracyChanged(Sensor sensor, int accuracy) {}

    private void updateRumble() {
        if (vibrator == null || !vibrator.hasVibrator())
            return;
        boolean active = showing && !hidden && layout.rumbleEnabled && !editing;
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
            } catch (RuntimeException ignored) {}
        }
        lastAmplitude = 0;
    }

    private static int applyAlpha(int color, float opacity) {
        int alpha = Math.round(Color.alpha(color) * opacity);
        return (color & 0x00ffffff) | (Math.max(0, Math.min(255, alpha)) << 24);
    }

    private void drawGraphicButton(Canvas canvas, float x, float y, float radius, String label,
                                   boolean active, boolean toggled, float textSize, float op, Drawable customIcon) {
        if (customIcon != null) {
            customIcon.setBounds((int)(x - radius), (int)(y - radius), (int)(x + radius), (int)(y + radius));
            customIcon.setAlpha((int)(op * 255));
            customIcon.draw(canvas);
            return;
        }

        paint.setStyle(Paint.Style.FILL);
        int centerColor = active ? applyAlpha(0xFF00E5FF, op * 0.75f) : (toggled ? applyAlpha(0xFF0091EA, op * 0.65f) : applyAlpha(0xFF141E28, op * 0.70f));
        int edgeColor = active ? applyAlpha(0xFF0091EA, op * 0.90f) : applyAlpha(0xFF090D12, op * 0.85f);
        paint.setShader(new RadialGradient(x, y, radius, centerColor, edgeColor, Shader.TileMode.CLAMP));
        canvas.drawCircle(x, y, radius, paint);
        paint.setShader(null);

        outlinePaint.setStrokeWidth(active || toggled ? 3f : 1.8f);
        int strokeColor = active ? 0xFFFFFFFF : (toggled ? 0xFF00E5FF : applyAlpha(0xFF4FC3F7, op * 0.80f));
        outlinePaint.setColor(applyAlpha(strokeColor, op));
        canvas.drawCircle(x, y, radius, outlinePaint);

        textPaint.setColor(toggled ? 0xFF00E5FF : Color.WHITE);
        textPaint.setAlpha((int)(Math.max(op, 0.5f) * 255));
        textPaint.setTextSize(textSize);

        float maxTextWidth = radius * 1.55f;
        float measuredWidth = textPaint.measureText(label);
        if (measuredWidth > maxTextWidth) {
            textSize = textSize * (maxTextWidth / measuredWidth);
            textPaint.setTextSize(textSize);
        }

        float yOffset = (textPaint.descent() + textPaint.ascent()) / 2f;
        canvas.drawText(label, x, y - yOffset, textPaint);
    }

    private void drawGraphicStick(Canvas canvas) {
        int stick = TouchLayout.LEFT;
        float radius = layout.radius(stick);
        float size = layout.sizeScale(stick);
        float op = layout.controlOpacity(stick);
        boolean isHeld = held(stick);

        float x = Math.max(radius, Math.min(logicalWidth - radius, stickCenterX()));
        float y = Math.max(radius, Math.min(logicalHeight - radius, stickCenterY()));

        Drawable baseDrawable = loadExternalDrawable(layout.iconFile(stick));
        if (baseDrawable == null) baseDrawable = stickBaseDrawable;

        if (baseDrawable != null) {
            baseDrawable.setBounds((int)(x - radius), (int)(y - radius), (int)(x + radius), (int)(y + radius));
            baseDrawable.setAlpha((int)(op * 255));
            baseDrawable.draw(canvas);
        } else {
            paint.setStyle(Paint.Style.FILL);
            paint.setShader(new RadialGradient(x, y, radius, applyAlpha(0xFF142434, op * 0.40f), applyAlpha(0xFF070B0E, op * 0.80f), Shader.TileMode.CLAMP));
            canvas.drawCircle(x, y, radius, paint);
            paint.setShader(null);

            outlinePaint.setStrokeWidth(2.2f);
            outlinePaint.setColor(applyAlpha(isHeld ? 0xFF00E5FF : 0xFF37474F, op));
            canvas.drawCircle(x, y, radius, outlinePaint);
        }

        if (editing && selectedControl >= 0 && layout.type(selectedControl) == TouchLayout.LEFT) {
            outlinePaint.setStrokeWidth(3.5f);
            outlinePaint.setColor(Color.YELLOW);
            canvas.drawCircle(x, y, radius + 4f, outlinePaint);
        }

        float kx = x + axes[0] / 32767f * radius;
        float ky = y + axes[1] / 32767f * radius;
        float kr = Math.max(STICK_KNOB_RADIUS * size, radius * 0.38f);

        Drawable knobDrawable = loadExternalDrawable(layout.knobFile(stick));
        if (knobDrawable == null) knobDrawable = stickKnobDrawable;

        if (knobDrawable != null) {
            knobDrawable.setBounds((int)(kx - kr), (int)(ky - kr), (int)(kx + kr), (int)(ky + kr));
            knobDrawable.setAlpha((int)(op * 255));
            knobDrawable.draw(canvas);
        } else {
            paint.setStyle(Paint.Style.FILL);
            int kCenter = isHeld ? applyAlpha(0xFF4FC3F7, op) : applyAlpha(0xFF263238, op);
            int kEdge = isHeld ? applyAlpha(0xFF0288D1, op) : applyAlpha(0xFF102027, op);
            paint.setShader(new RadialGradient(kx, ky, kr, kCenter, kEdge, Shader.TileMode.CLAMP));
            canvas.drawCircle(kx, ky, kr, paint);
            paint.setShader(null);

            outlinePaint.setStrokeWidth(2f);
            outlinePaint.setColor(applyAlpha(isHeld ? 0xFFFFFFFF : 0xFF4FC3F7, op));
            canvas.drawCircle(kx, ky, kr, outlinePaint);
        }
    }

    @Override
    protected void onDraw(Canvas canvas) {
        if (!showing && !editing)
            return;

        canvas.save();
        canvas.scale(scale, scale);

        if (!hidden || editing) {
            // 1. Joystick
            if (layout.shown(TouchLayout.LEFT)) {
                drawGraphicStick(canvas);
            }

            // 2. Buttons
            for (int i = 0; i < layout.size(); i++) {
                if (!layout.shown(i) || layout.type(i) == TouchLayout.LEFT)
                    continue;

                int type = layout.type(i);
                float textSize = 10.5f * Math.max(layout.sizeScale(i), layout.radius(i) / 38f);
                float op = layout.controlOpacity(i);
                boolean isHeld = held(i) || dragControl == i;
                boolean isToggled = toggledStates.get(i, false);

                String iconKey = layout.iconFile(i);
                if ((iconKey == null || iconKey.trim().isEmpty()) && type < KINDS.length) {
                    iconKey = "tc_ic_" + KINDS[type].buttonName.toLowerCase(Locale.US);
                } else if ((iconKey == null || iconKey.trim().isEmpty()) && type == TouchLayout.TB_HIDE) {
                    iconKey = "tc_ic_hide";
                }
                Drawable custom = loadExternalDrawable(iconKey);

                drawGraphicButton(canvas, layout.x(i), layout.y(i), layout.radius(i),
                        buttonDisplayLabel(i), isHeld, isToggled, Math.min(textSize, 15), op, custom);

                if (editing && i == selectedControl) {
                    outlinePaint.setStrokeWidth(3.5f);
                    outlinePaint.setColor(Color.YELLOW);
                    canvas.drawCircle(layout.x(i), layout.y(i), layout.radius(i) + 4f, outlinePaint);
                }
            }
        } else {
            // Controls hidden in play mode: draw only the Touch toggle button
            for (int i = 0; i < layout.size(); i++) {
                if (layout.type(i) == TouchLayout.TB_HIDE && layout.shown(i)) {
                    float textSize = 10.5f * Math.max(layout.sizeScale(i), layout.radius(i) / 38f);
                    float op = layout.controlOpacity(i); // Uses the exact control opacity set by the user
                    String iconKey = layout.iconFile(i);
                    if (iconKey == null || iconKey.trim().isEmpty()) {
                        iconKey = "tc_ic_hide";
                    }
                    Drawable custom = loadExternalDrawable(iconKey);
                    drawGraphicButton(canvas, layout.x(i), layout.y(i), layout.radius(i),
                            "Touch", false, false, Math.min(textSize, 15), op, custom);
                    break;
                }
            }
        }

        canvas.restore();
        super.onDraw(canvas);
    }
}
