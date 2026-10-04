package com.halo.decomp;

import android.app.AlertDialog;
import android.media.AudioAttributes;
import android.hardware.Sensor;
import android.hardware.SensorEvent;
import android.hardware.SensorEventListener;
import android.hardware.SensorManager;
import android.os.Build;
import android.os.SystemClock;
import android.os.VibrationEffect;
import android.os.Vibrator;
import android.os.VibratorManager;
import android.view.WindowManager;
import android.widget.Switch;
import android.content.Context;
import android.widget.LinearLayout;
import android.widget.ScrollView;
import android.widget.SeekBar;
import android.widget.TextView;
import android.content.SharedPreferences;
import android.content.pm.PackageManager;
import android.hardware.input.InputManager;
import android.view.InputDevice;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Paint;
import android.util.SparseIntArray;
import android.util.SparseArray;
import android.view.MotionEvent;
import android.view.View;
import android.widget.Toast;

/**
 * The on-screen touch controls: a multitouch overlay over SDL's surface,
 * with a stick, the controller's buttons, swipe (and optionally gyroscope)
 * aiming and full-display layout editing. Its state goes to the game
 * through port/android/host/host_touch.c, as port 0's controller.
 *
 * It shows only in a game, never over the menus or a cinematic (which take
 * taps themselves, port/linux/src/touch_input.c), and with
 * input.touch_controls = "auto" only on a touchscreen while no controller
 * is connected. While hidden it lets every finger through to the game.
 */
public final class TouchControls extends View implements SensorEventListener, InputManager.InputDeviceListener {
    private static final int LEFT = TouchLayout.LEFT, LOOK = -5;
    private static final int TOGGLE = -3, EDIT = -4, EXPORT = -6, IMPORT = -7;
    private static final class Button {
        final String label;
        final float radius;
        final int bit, trigger;
        Button(String label, float radius, int bit, int trigger) {
            this.label = label; this.radius = radius;
            this.bit = bit; this.trigger = trigger;
        }
    }
    // Button bits follow SDL_GamepadButton; triggers are SDL axes 4 and 5.
    private final Button[] buttons = {
        new Button("A / Jump", 36, 0, -1),
        new Button("B / Melee", 32, 1, -1),
        new Button("X / Reload", 34, 2, -1),
        new Button("Y / Weapon", 32, 3, -1),
        new Button("Fire", 39, -1, 5),
        new Button("Grenade", 35, -1, 4),
        new Button("Crouch", 32, 7, -1),
        new Button("Zoom", 32, 8, -1),
        new Button("Light", 27, 9, -1),
        new Button("Gren. type", 29, 10, -1),
        new Button("Pause", 28, 6, -1),
        new Button("Back", 28, 4, -1),
        new Button("Up", 25, 11, -1),
        new Button("Down", 25, 12, -1),
        new Button("Left", 25, 13, -1),
        new Button("Right", 25, 14, -1),
        new Button("", 64, -1, -1), // movement stick keeps its saved index
        new Button("Fire", 39, -1, 5)
    };
    private final Paint paint = new Paint(Paint.ANTI_ALIAS_FLAG);
    private final SparseIntArray owners = new SparseIntArray();
    private final SparseArray<float[]> buttonTouches = new SparseArray<>();
    private final int[] axes = new int[6];
    private TouchLayout layout = new TouchLayout();
    private final SharedPreferences preferences;
    private boolean editing, optionsOpen;
    private final SensorManager sensors;
    private final Sensor gyroscope;
    private final Vibrator vibrator;
    private final GyroscopeAim gyroAim = new GyroscopeAim();
    private final float[] gyroDelta = new float[2];
    private boolean deviceInputActive, gyroRegistered;
    private int lastAmplitude;
    private long lastVibration;
    private final AudioAttributes rumbleAttributes = new AudioAttributes.Builder()
        .setUsage(AudioAttributes.USAGE_GAME).setContentType(AudioAttributes.CONTENT_TYPE_SONIFICATION).build();
    private static native int nativeRumble();
    /** reads what the game says every frame: when to show, and the rumble */
    private final Runnable poll = new Runnable() {
        public void run() {
            if (!deviceInputActive) return;
            boolean wasShown = shown();
            scene = nativeScene();
            if (shown() != wasShown) {
                reset(); updateSensors();
            }
            int amplitude = shown() && layout.rumbleEnabled && !editing && !optionsOpen ? nativeRumble() : 0;
            if (vibrator != null && vibrator.hasVibrator()) {
                if (amplitude == 0) cancelRumble();
                else if (amplitude != lastAmplitude || SystemClock.uptimeMillis()-lastVibration >= 70) {
                    try {
                        vibrator.vibrate(VibrationEffect.createOneShot(110,
                            vibrator.hasAmplitudeControl() ? amplitude : VibrationEffect.DEFAULT_AMPLITUDE), rumbleAttributes);
                        lastAmplitude = amplitude; lastVibration = SystemClock.uptimeMillis();
                    } catch (RuntimeException e) { cancelRumble(); }
                }
            }
            postDelayed(this, 16);
        }
    };
    private int lookPointer = -1;
    private float lookX, lookY;
    private float sensitivity;
    private float logicalWidth = 960, logicalHeight = 540;
    private static native void nativeLook(float dx, float dy);
    private static native void nativeLookReset();
    private int dragPointer = -1, dragControl = -1;
    private float dragOffsetX, dragOffsetY;
    private float scale = 1, offsetX, offsetY;
    private int insetLeft, insetRight, insetTop, insetBottom;
    private boolean visible = true;

    private static native void nativeState(int lx, int ly, int rx, int ry,
                                          int lt, int rt, int buttons);

    /** the game's HALO_TOUCH_SCENE bits (port/linux/src/touch_input.c) */
    private static native int nativeScene();
    private static final int SCENE_KNOWN = 1, SCENE_MENUS = 2, SCENE_ON = 4, SCENE_OFF = 8;
    private int scene;
    private final InputManager inputManager;
    private final boolean touchscreenFeature;
    private boolean controllerConnected, touchscreen;

    public TouchControls(Context context) {
        super(context);
        sensors = (SensorManager) context.getSystemService(Context.SENSOR_SERVICE);
        gyroscope = sensors == null ? null : sensors.getDefaultSensor(Sensor.TYPE_GYROSCOPE);
        if (Build.VERSION.SDK_INT >= 31) {
            VibratorManager manager = (VibratorManager) context.getSystemService(Context.VIBRATOR_MANAGER_SERVICE);
            vibrator = manager == null ? null : manager.getDefaultVibrator();
        } else vibrator = (Vibrator) context.getSystemService(Context.VIBRATOR_SERVICE);
        inputManager = (InputManager) context.getSystemService(Context.INPUT_SERVICE);
        touchscreenFeature = context.getPackageManager().hasSystemFeature(PackageManager.FEATURE_TOUCHSCREEN);
        refreshInputDevices();
        preferences = context.getSharedPreferences("touch-layout-v1", Context.MODE_PRIVATE);
        String configuration = preferences.getString("configuration", null);
        if (configuration != null) {
            try {
                TouchLayout.Configuration saved = TouchLayout.importConfiguration(configuration);
                layout = saved.layout; sensitivity = saved.sensitivity;
            } catch (IllegalArgumentException e) {
                Toast.makeText(context, "Saved layout could not be loaded. Using defaults.", Toast.LENGTH_LONG).show();
            }
        } else {
            for (int i = 0; i < layout.size(); i++)
                if (i != TouchLayout.FIRE_LEFT || preferences.getBoolean("swipe-layout", false))
                    layout.restore(i, preferences.getFloat("x"+i, layout.x(i)),
                                      preferences.getFloat("y"+i, layout.y(i)));
        }
        sensitivity = preferences.getFloat("look-sensitivity", sensitivity > 0 ? sensitivity : 1f);
        if (!Float.isFinite(sensitivity) || sensitivity < 0.25f || sensitivity > 4f) sensitivity = 1f;
        setFocusable(false);
        setContentDescription("Halo touch controller");
        setOnApplyWindowInsetsListener((view, insets) -> {
            insetLeft = insets.getSystemWindowInsetLeft();
            insetRight = insets.getSystemWindowInsetRight();
            insetTop = insets.getSystemWindowInsetTop();
            insetBottom = insets.getSystemWindowInsetBottom();
            if (android.os.Build.VERSION.SDK_INT >= 28 && insets.getDisplayCutout() != null) {
                insetLeft = Math.max(insetLeft, insets.getDisplayCutout().getSafeInsetLeft());
                insetRight = Math.max(insetRight, insets.getDisplayCutout().getSafeInsetRight());
                insetTop = Math.max(insetTop, insets.getDisplayCutout().getSafeInsetTop());
                insetBottom = Math.max(insetBottom, insets.getDisplayCutout().getSafeInsetBottom());
            }
            layoutControls();
            return insets;
        });
    }

    private void layoutControls() {
        if (getWidth() <= 0 || getHeight() <= 0) return;
        reset();
        // Keep circles proportional while allowing placement across the complete display.
        float width = Math.max(1, getWidth()), height = Math.max(1, getHeight());
        scale = Math.max(0.01f, Math.min(width / 960f, height / 540f));
        offsetX = offsetY = 0;
        logicalWidth = width / scale; logicalHeight = height / scale;
        layout.bounds(logicalWidth, logicalHeight);
        invalidate();
    }

    @Override protected void onSizeChanged(int w, int h, int oldw, int oldh) {
        layoutControls();
        requestApplyInsets();
    }

    public void reset() {
        dragPointer = dragControl = -1;
        owners.clear();
        buttonTouches.clear();
        lookPointer = -1;
        gyroAim.reset();
        nativeLookReset();
        java.util.Arrays.fill(axes, 0);
        publish();
        invalidate();
    }

    private void publish() {
        if (editing || optionsOpen) {
            nativeState(0, 0, 0, 0, 0, 0, 0);
            return;
        }
        int bits = 0;
        axes[4] = axes[5] = 0;
        for (int i = 0; i < owners.size(); i++) {
            int control = owners.valueAt(i);
            if (control < 0 || control >= layout.size()) continue;
            Button b = buttons[layout.type(control)];
            if (b.bit >= 0) bits |= 1 << b.bit;
            if (b.trigger >= 0) axes[b.trigger] = 32767;
        }
        nativeState(axes[0], axes[1], axes[2], axes[3], axes[4], axes[5], bits);
    }

    private boolean held(int control) {
        return owners.indexOfValue(control) >= 0;
    }

    private static boolean inside(float x, float y, float cx, float cy, float radius) {
        return (x-cx)*(x-cx) + (y-cy)*(y-cy) <= radius*radius;
    }

    private int hit(float x, float y) {
        if (inside(x, y, optionsX(), toolbarY(), 34)) return EDIT;
        if (editing && inside(x, y, optionsX()-156, toolbarY(), 34)) return EXPORT;
        if (editing && inside(x, y, optionsX()-78, toolbarY(), 34)) return IMPORT;
        if (inside(x, y, logicalWidth/2, toolbarY(), 28)) return TOGGLE;
        if (!visible) return Integer.MIN_VALUE;
        for (int i = 0; i < layout.size(); i++) {
            if (!layout.shown(i) || layout.type(i) == LEFT) continue;
            Button b = buttons[layout.type(i)];
            if (inside(x, y, layout.x(i), layout.y(i), layout.radius(i))) return i;
        }
        if (layout.shown(LEFT) && !held(LEFT) && inside(x, y, layout.x(LEFT), layout.y(LEFT), layout.radius(LEFT)*1.28f)) return LEFT;
        return Integer.MIN_VALUE;
    }

    private void moveStick(int control, float x, float y) {
        int axis = 0;
        float dx = (x - layout.x(control)) / layout.radius(control);
        float dy = (y - layout.y(control)) / layout.radius(control);
        float length = (float)Math.sqrt(dx*dx + dy*dy);
        if (length < 0.12f) { dx = 0; dy = 0; }
        else if (length > 1) { dx /= length; dy /= length; }
        axes[axis] = Math.round(dx * 32767);
        axes[axis+1] = Math.round(dy * 32767);
    }

    /**
     * whether the controls are on screen: in a game (no menu or cinematic),
     * never without a touchscreen, and with input.touch_controls = "auto"
     * only while no controller is connected
     */
    private boolean shown() {
        if ((scene & SCENE_KNOWN) == 0 || (scene & (SCENE_MENUS | SCENE_OFF)) != 0 || !touchscreen)
            return false;
        return (scene & SCENE_ON) != 0 || !controllerConnected;
    }

    /** a game controller: a stick or a hat, not a phone's few gamepad keys */
    private static boolean isController(InputDevice device) {
        if (device == null || device.isVirtual()) return false;
        int sources = device.getSources();
        if ((sources & InputDevice.SOURCE_JOYSTICK) != InputDevice.SOURCE_JOYSTICK) return false;
        return device.getMotionRange(MotionEvent.AXIS_X, InputDevice.SOURCE_JOYSTICK) != null
            || device.getMotionRange(MotionEvent.AXIS_HAT_X, InputDevice.SOURCE_JOYSTICK) != null;
    }

    private void refreshInputDevices() {
        boolean controller = false, touch = touchscreenFeature;
        for (int id : InputDevice.getDeviceIds()) {
            InputDevice device = InputDevice.getDevice(id);
            if (device == null) continue;
            if (isController(device)) controller = true;
            if (!device.isVirtual() && (device.getSources() & InputDevice.SOURCE_TOUCHSCREEN) == InputDevice.SOURCE_TOUCHSCREEN)
                touch = true;
        }
        boolean wasShown = shown();
        controllerConnected = controller; touchscreen = touch;
        if (shown() != wasShown) {
            reset(); updateSensors();
        }
    }

    @Override public void onInputDeviceAdded(int deviceId) { refreshInputDevices(); }
    @Override public void onInputDeviceRemoved(int deviceId) { refreshInputDevices(); }
    @Override public void onInputDeviceChanged(int deviceId) { refreshInputDevices(); }

    /**
     * whether a finger went down where Android keeps its edge gestures; in
     * sticky full screen the first swipe in from an edge only shows the
     * system bars, and Android also hands it over as an ordinary finger
     */
    private boolean inGestureZone(float x, float y, boolean sidesOnly) {
        int[] insets = ((HaloActivity)getContext()).getSystemGestureInsetsPixels();
        if (x < insets[0] || x >= getWidth()-insets[2]) return true;
        return !sidesOnly && (y < insets[1] || y >= getHeight()-insets[3]);
    }

    @Override public boolean onTouchEvent(MotionEvent event) {
        int action = event.getActionMasked(), index = event.getActionIndex();
        int id = event.getPointerId(index);
        float x = (event.getX(index)-offsetX)/scale, y = (event.getY(index)-offsetY)/scale;
        if (!shown() && !editing && !optionsOpen) {
            // the menus and cinematics take the fingers (SDL's surface below)
            if (action == MotionEvent.ACTION_DOWN) return false;
            if (owners.size() > 0 || lookPointer >= 0) { reset(); invalidate(); }
            return true;
        }
        if (editing) return editTouch(event, action, index, id, x, y);
        if ((action == MotionEvent.ACTION_DOWN || action == MotionEvent.ACTION_POINTER_DOWN)
                && inGestureZone(event.getX(index), event.getY(index), true)) {
            return true;
        }
        if (action == MotionEvent.ACTION_DOWN || action == MotionEvent.ACTION_POINTER_DOWN) {
            int control = hit(x, y);
            if (control == EDIT) {
                showOptions(); performClick();
            } else if (control == TOGGLE) {
                reset(); visible = !visible; performClick();
            } else if (control != Integer.MIN_VALUE) {
                owners.put(id, control);
                if (control >= 0 && layout.type(control) == LEFT) moveStick(control, x, y);
                else buttonTouches.put(id, new float[]{event.getX(index), event.getY(index)});
            } else if (visible && lookPointer < 0 && !inGestureZone(event.getX(index), event.getY(index), false)) {
                lookPointer = id; lookX = event.getX(index); lookY = event.getY(index);
                owners.put(id, LOOK);
            }
        } else if (action == MotionEvent.ACTION_MOVE) {
            for (int i = 0; i < event.getPointerCount(); i++) {
                int control = owners.get(event.getPointerId(i), Integer.MIN_VALUE);
                if (control >= 0 && layout.type(control) == LEFT)
                    moveStick(control, (event.getX(i)-offsetX)/scale, (event.getY(i)-offsetY)/scale);
                int pointer = event.getPointerId(i);
                float[] origin = buttonTouches.get(pointer);
                if (lookPointer < 0 && origin != null &&
                        Math.hypot(event.getX(i)-origin[0], event.getY(i)-origin[1]) > 10*scale) {
                    // A held action button can also aim, including either Fire button.
                    lookPointer = pointer; lookX = origin[0]; lookY = origin[1];
                }
                if (pointer == lookPointer) {
                    float nx = event.getX(i), ny = event.getY(i);
                    nativeLook((nx-lookX)/scale*sensitivity, (ny-lookY)/scale*sensitivity);
                    lookX = nx; lookY = ny;
                }
            }
        } else if (action == MotionEvent.ACTION_UP || action == MotionEvent.ACTION_POINTER_UP) {
            int control = owners.get(id, Integer.MIN_VALUE);
            if (control >= 0 && layout.type(control) == LEFT) {
                int axis = 0;
                axes[axis] = axes[axis+1] = 0;
            }
            if (id == lookPointer) lookPointer = -1;
            owners.delete(id);
            buttonTouches.remove(id);
        } else if (action == MotionEvent.ACTION_CANCEL) {
            reset();
        }
        publish(); invalidate();
        return true;
    }

    @Override public boolean performClick() { super.performClick(); return true; }

    private boolean editTouch(MotionEvent event, int action, int index, int id, float x, float y) {
        if (action == MotionEvent.ACTION_DOWN || action == MotionEvent.ACTION_POINTER_DOWN) {
            if (dragPointer < 0) {
                int control = hit(x, y);
                if (control == EDIT) {
                    if (saveLayout()) {
                        reset(); editing = false; performClick();
                    }
                } else if (control == EXPORT || control == IMPORT) {
                    reset();
                    ((HaloActivity)getContext()).chooseLayoutFile(control == EXPORT, exportLayout());
                } else if (control >= 0) {
                    dragPointer = id; dragControl = control;
                    dragOffsetX = x-layout.x(control); dragOffsetY = y-layout.y(control);
                }
            }
        } else if (action == MotionEvent.ACTION_MOVE && dragPointer >= 0) {
            int pointer = event.findPointerIndex(dragPointer);
            if (pointer >= 0)
                layout.move(dragControl, (event.getX(pointer)-offsetX)/scale-dragOffsetX,
                                        (event.getY(pointer)-offsetY)/scale-dragOffsetY);
        } else if (action == MotionEvent.ACTION_CANCEL ||
                   ((action == MotionEvent.ACTION_UP || action == MotionEvent.ACTION_POINTER_UP) && id == dragPointer)) {
            dragPointer = dragControl = -1;
        }
        publish(); invalidate();
        return true;
    }

    private float toolbarY() { return Math.max(38, insetTop/scale+38); }
    private float optionsX() { return logicalWidth-Math.max(42, insetRight/scale+42); }

    private void editorButton(Canvas canvas) {
        circle(canvas, optionsX(), toolbarY(), 34, editing ? "Save" : "Options", false);
        if (editing) {
            circle(canvas, optionsX()-156, toolbarY(), 34, "Export", false);
            circle(canvas, optionsX()-78, toolbarY(), 34, "Import", false);
            paint.setTextSize(14);
            canvas.drawText("Drag controls, then Save and exit", logicalWidth/2, toolbarY()+58, paint);
            paint.setTextSize(9);
            canvas.drawText("and exit", optionsX(), toolbarY()+17, paint);
        }
    }

    private void showOptions() {
        optionsOpen = true; reset();
        AlertDialog dialog = new AlertDialog.Builder(getContext()).setTitle("Options")
            .setItems(new String[]{"General", "Edit buttons layout", "Look sensitivity"}, (d, which) -> {
                if (which == 0) post(this::showGeneral);
                else if (which == 1) { editing = true; visible = true; invalidate(); }
                else post(this::showSensitivity);
            }).setNegativeButton("Close", null).create();
        dialog.setOnDismissListener(d -> { optionsOpen = false; reset(); });
        dialog.show();
    }

    private boolean saveLayout() {
        boolean saved = preferences.edit().putString("configuration", exportLayout())
            .putFloat("look-sensitivity", sensitivity).putBoolean("swipe-layout", true).commit();
        if (!saved) Toast.makeText(getContext(), "Could not save layout. Try again.", Toast.LENGTH_LONG).show();
        return saved;
    }

    public String exportLayout() { return layout.exportConfiguration(sensitivity); }

    public void importLayout(String text) {
        // Parse and save the entire configuration before replacing the live controls.
        TouchLayout.Configuration imported = TouchLayout.importConfiguration(text);
        imported.layout.bounds(logicalWidth, logicalHeight);
        String normalized = imported.layout.exportConfiguration(imported.sensitivity);
        if (!preferences.edit().putString("configuration", normalized)
                .putFloat("look-sensitivity", imported.sensitivity).putBoolean("swipe-layout", true).commit())
            throw new IllegalArgumentException("Could not save imported layout");
        reset(); layout = imported.layout; sensitivity = imported.sensitivity;
        visible = true; updateSensors(); cancelRumble(); invalidate();
    }

    private String controlName(int type) { return type == LEFT ? "Move stick" : buttons[type].label; }

    public void startDeviceInput() {
        if (deviceInputActive) return;
        deviceInputActive = true;
        if (inputManager != null) inputManager.registerInputDeviceListener(this, null);
        refreshInputDevices();
        gyroAim.reset(); updateSensors(); post(poll);
    }

    public void stopDeviceInput() {
        deviceInputActive = false; removeCallbacks(poll);
        if (inputManager != null) inputManager.unregisterInputDeviceListener(this);
        updateSensors(); cancelRumble(); reset();
    }

    private void updateSensors() {
        boolean needed = deviceInputActive && shown() && layout.gyroscopeEnabled && gyroscope != null;
        if (needed && !gyroRegistered) {
            gyroAim.reset();
            gyroRegistered = sensors.registerListener(this, gyroscope, SensorManager.SENSOR_DELAY_GAME);
        } else if (!needed && gyroRegistered) {
            sensors.unregisterListener(this); gyroRegistered = false; gyroAim.reset();
        }
    }

    private void cancelRumble() {
        if (lastAmplitude != 0 && vibrator != null) {
            try { vibrator.cancel(); } catch (RuntimeException ignored) {}
        }
        lastAmplitude = 0;
    }

    @Override public void onSensorChanged(SensorEvent event) {
        if (!deviceInputActive || !shown() || !layout.gyroscopeEnabled || editing || optionsOpen) {
            gyroAim.reset(); return;
        }
        int rotation = ((WindowManager)getContext().getSystemService(Context.WINDOW_SERVICE))
            .getDefaultDisplay().getRotation();
        if (gyroAim.sample(event.timestamp, event.values[0], event.values[1], rotation, gyroDelta))
            // The existing direct-look path accepts logical pixels (0.0022 radians per pixel).
            nativeLook(-gyroDelta[0]/0.0022f*sensitivity, -gyroDelta[1]/0.0022f*sensitivity);
    }

    @Override public void onAccuracyChanged(Sensor sensor, int accuracy) {}

    private void showGeneral() {
        optionsOpen = true; reset(); cancelRumble();
        LinearLayout panel = new LinearLayout(getContext()); panel.setOrientation(LinearLayout.VERTICAL);
        panel.setPadding(24, 16, 24, 16);
        Switch rumble = new Switch(getContext());
        boolean hasRumble = vibrator != null && vibrator.hasVibrator();
        rumble.setText(hasRumble ? "Rumble" : "Rumble (unavailable on this phone)");
        rumble.setChecked(layout.rumbleEnabled); rumble.setEnabled(hasRumble);
        rumble.setOnCheckedChangeListener((button, enabled) -> {
            layout.rumbleEnabled = enabled; cancelRumble(); saveLayout();
        });
        Switch gyro = new Switch(getContext());
        gyro.setText("Gyroscope aim (Experimental)"+(gyroscope == null ? " - unavailable on this phone" : ""));
        gyro.setChecked(layout.gyroscopeEnabled); gyro.setEnabled(gyroscope != null);
        gyro.setOnCheckedChangeListener((button, enabled) -> {
            layout.gyroscopeEnabled = enabled; reset(); updateSensors(); saveLayout();
        });
        panel.addView(rumble); panel.addView(gyro);
        AlertDialog dialog = new AlertDialog.Builder(getContext()).setTitle("General")
            .setView(panel).setPositiveButton("Done", null).create();
        android.widget.Button manage = new android.widget.Button(getContext());
        manage.setText("Hide or add buttons");
        manage.setOnClickListener(v -> { dialog.dismiss(); post(this::showButtonManager); });
        panel.addView(manage);
        android.widget.Button sizes = new android.widget.Button(getContext());
        sizes.setText("Edit buttons size");
        sizes.setOnClickListener(v -> { dialog.dismiss(); post(this::showButtonSizes); });
        panel.addView(sizes);
        dialog.setOnDismissListener(d -> { optionsOpen = false; reset(); updateSensors(); }); dialog.show();
    }

    private void showButtonSizes() {
        optionsOpen = true; reset();
        LinearLayout list = new LinearLayout(getContext()); list.setOrientation(LinearLayout.VERTICAL);
        for (int i = 0; i < layout.size(); i++) {
            final int control = i;
            LinearLayout row = new LinearLayout(getContext()); row.setPadding(16, 4, 16, 4);
            TextView label = new TextView(getContext()); label.setTextColor(Color.WHITE);
            label.setText(controlName(layout.type(i))+(i >= TouchLayout.BASE_COUNT ? " (copy)" : "")
                +(!layout.shown(i) ? " (hidden)" : ""));
            row.addView(label, new LinearLayout.LayoutParams(0, -2, 1));
            android.widget.Button minus = new android.widget.Button(getContext()); minus.setText("-");
            TextView value = new TextView(getContext()); value.setTextColor(Color.WHITE);
            android.widget.Button plus = new android.widget.Button(getContext()); plus.setText("+");
            Runnable refresh = () -> {
                int percent = Math.round(layout.sizeScale(control)*100);
                value.setText(percent+"%"); minus.setEnabled(percent > 50); plus.setEnabled(percent < 200);
            };
            minus.setOnClickListener(v -> {
                layout.setSize(control, Math.max(50, Math.round(layout.sizeScale(control)*100)-10)/100f);
                saveLayout(); refresh.run(); invalidate();
            });
            plus.setOnClickListener(v -> {
                layout.setSize(control, Math.min(200, Math.round(layout.sizeScale(control)*100)+10)/100f);
                saveLayout(); refresh.run(); invalidate();
            });
            row.addView(minus); row.addView(value); row.addView(plus);
            list.addView(row); refresh.run();
        }
        ScrollView scroll = new ScrollView(getContext()); scroll.addView(list);
        AlertDialog dialog = new AlertDialog.Builder(getContext()).setTitle("Edit buttons size")
            .setView(scroll).setPositiveButton("Done", null).create();
        dialog.setOnDismissListener(d -> { optionsOpen = false; reset(); }); dialog.show();
    }

    private void showButtonManager() {
        optionsOpen = true; reset();
        LinearLayout list = new LinearLayout(getContext()); list.setOrientation(LinearLayout.VERTICAL);
        Runnable[] refresh = new Runnable[1];
        refresh[0] = () -> {
            list.removeAllViews();
            for (int i = 0; i < layout.size(); i++) {
                final int control = i;
                LinearLayout row = new LinearLayout(getContext()); row.setPadding(16, 4, 16, 4);
                TextView label = new TextView(getContext()); label.setTextColor(Color.WHITE);
                label.setText(controlName(layout.type(i))+(i >= TouchLayout.BASE_COUNT ? " (copy)" : ""));
                row.addView(label, new LinearLayout.LayoutParams(0, -2, 1));
                android.widget.Button toggle = new android.widget.Button(getContext());
                toggle.setText(layout.shown(i) ? "Hide" : "Show");
                toggle.setOnClickListener(v -> {
                    reset(); layout.setShown(control, !layout.shown(control));
                    saveLayout(); refresh[0].run(); invalidate();
                });
                row.addView(toggle);
                if (layout.type(i) != LEFT) {
                    android.widget.Button copy = new android.widget.Button(getContext()); copy.setText("Duplicate");
                    copy.setOnClickListener(v -> {
                        reset();
                        if (layout.duplicate(control) < 0)
                            Toast.makeText(getContext(), "Maximum 64 controls. Reset to remove copies.", Toast.LENGTH_LONG).show();
                        else saveLayout();
                        refresh[0].run(); invalidate();
                    });
                    row.addView(copy);
                }
                list.addView(row);
            }
        };
        refresh[0].run();
        ScrollView scroll = new ScrollView(getContext()); scroll.addView(list);
        AlertDialog dialog = new AlertDialog.Builder(getContext()).setTitle("Hide or add buttons")
            .setView(scroll).setPositiveButton("Done", null).setNegativeButton("Reset", null)
            .setNeutralButton("Add button", null).create();
        dialog.setOnDismissListener(d -> { optionsOpen = false; reset(); });
        dialog.show();
        dialog.getButton(AlertDialog.BUTTON_NEGATIVE).setOnClickListener(v -> {
            new AlertDialog.Builder(getContext()).setTitle("Reset buttons?")
                .setMessage("Restore original positions, show all default controls and remove duplicates?")
                .setNegativeButton("Cancel", null).setPositiveButton("Reset", (d, which) -> {
                    reset(); layout.resetDefaults(); visible = true; saveLayout();
                    refresh[0].run(); invalidate();
                }).show();
        });
        dialog.getButton(AlertDialog.BUTTON_NEUTRAL).setOnClickListener(v -> {
            String[] names = new String[TouchLayout.BASE_COUNT];
            for (int i = 0; i < names.length; i++) names[i] = controlName(i);
            new AlertDialog.Builder(getContext()).setTitle("Add button").setItems(names, (d, type) -> {
                reset();
                if (layout.add(type) < 0)
                    Toast.makeText(getContext(), type == LEFT ? "Move stick is already visible." : "Maximum 64 controls.", Toast.LENGTH_LONG).show();
                else saveLayout();
                refresh[0].run(); invalidate();
            }).setNegativeButton("Cancel", null).show();
        });
    }

    private void showSensitivity() {
        optionsOpen = true; reset();
        LinearLayout panel = new LinearLayout(getContext()); panel.setOrientation(LinearLayout.VERTICAL);
        panel.setPadding(32, 16, 32, 16);
        TextView value = new TextView(getContext());
        SeekBar slider = new SeekBar(getContext()); slider.setMax(150);
        slider.setProgress(Math.round((sensitivity-0.25f)/0.025f));
        value.setText(String.format(java.util.Locale.US, "Look sensitivity: %.2fx", sensitivity));
        slider.setOnSeekBarChangeListener(new SeekBar.OnSeekBarChangeListener() {
            public void onProgressChanged(SeekBar bar, int progress, boolean user) {
                sensitivity = 0.25f+progress*0.025f;
                value.setText(String.format(java.util.Locale.US, "Look sensitivity: %.2fx", sensitivity));
                saveLayout();
            }
            public void onStartTrackingTouch(SeekBar bar) {}
            public void onStopTrackingTouch(SeekBar bar) {}
        });
        panel.addView(value); panel.addView(slider);
        AlertDialog dialog = new AlertDialog.Builder(getContext()).setTitle("Look sensitivity")
            .setView(panel).setPositiveButton("Done", null).create();
        dialog.setOnDismissListener(d -> { optionsOpen = false; reset(); }); dialog.show();
    }

    private void circle(Canvas canvas, float x, float y, float radius, String label, boolean active) {
        circle(canvas, x, y, radius, label, active, 11);
    }

    private void circle(Canvas canvas, float x, float y, float radius, String label, boolean active, float textSize) {
        paint.setStyle(Paint.Style.FILL);
        paint.setColor(active ? 0x9983d9ff : 0x55304050);
        canvas.drawCircle(x, y, radius, paint);
        paint.setStyle(Paint.Style.STROKE); paint.setStrokeWidth(2);
        paint.setColor(active ? 0xffaee7ff : 0xffffffff);
        canvas.drawCircle(x, y, radius, paint);
        paint.setStyle(Paint.Style.FILL); paint.setColor(Color.WHITE);
        paint.setTextAlign(Paint.Align.CENTER); paint.setTextSize(textSize);
        canvas.drawText(label, x, y + textSize*0.36f, paint);
    }

    private void stick(Canvas canvas, int axis, float x, float y, String label) {
        circle(canvas, x, y, layout.radius(LEFT), label, false, 11*layout.sizeScale(LEFT));
        circle(canvas, x + axes[axis]/32767f*layout.radius(LEFT), y + axes[axis+1]/32767f*layout.radius(LEFT),
               24*layout.sizeScale(LEFT), "", held(LEFT));
    }

    @Override protected void onDraw(Canvas canvas) {
        if (!shown() && !editing) return;
        canvas.save(); canvas.translate(offsetX, offsetY); canvas.scale(scale, scale);
        if (!editing) circle(canvas, logicalWidth/2, toolbarY(), 28, visible ? "Hide" : "Touch", false);
        if (visible) {
            if (layout.shown(LEFT)) stick(canvas, 0, layout.x(LEFT), layout.y(LEFT), "Move");
            for (int i = 0; i < layout.size(); i++) {
                if (!layout.shown(i) || layout.type(i) == LEFT) continue;
                Button b = buttons[layout.type(i)];
                circle(canvas, layout.x(i), layout.y(i), layout.radius(i), b.label, held(i) || dragControl == i, 11*layout.sizeScale(i));
            }
        }
        editorButton(canvas);
        canvas.restore();
    }
}
