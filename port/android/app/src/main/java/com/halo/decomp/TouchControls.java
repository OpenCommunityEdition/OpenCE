package com.halo.decomp;

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
import android.content.Context;
import android.content.SharedPreferences;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Paint;
import android.util.SparseIntArray;
import android.util.SparseArray;
import android.view.MotionEvent;
import android.view.WindowManager;
import android.view.View;
import android.widget.Toast;

/** Multitouch overlay with direct swipe aiming and full-display layout editing. */
public final class TouchControls extends View implements SensorEventListener {
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
        new Button("Fire", 39, -1, 5),
        new Button("Camera mode", 29, -1, -1)
    };
    private final Paint paint = new Paint(Paint.ANTI_ALIAS_FLAG);
    private final SparseIntArray owners = new SparseIntArray();
    private final SparseArray<float[]> buttonTouches = new SparseArray<>();
    private final int[] axes = new int[6];
    private TouchLayout layout = new TouchLayout();
    private final SharedPreferences preferences;
    private boolean editing;
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
    private final Runnable rumblePoll = new Runnable() {
        public void run() {
            if (!deviceInputActive) return;
            int amplitude = layout.rumbleEnabled && !editing && !menusActive ? nativeRumble() : 0;
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
    private static native void nativeCameraMode();
    private static native boolean nativeCheatRequest(int id, boolean enabled);
    private static native int nativeCheatStatus(int id);
    private static native void nativeFieldOfView(float degrees);
    private StartupCheats startupCheats;
    // Edit in-game UI: the game's own HUD panels, saved apart from the touch layout.
    private static native void nativeHudLayout(float[] offsets);
    // The main menu's build label: the installed app's own version, whatever build this is.
    private static native void nativeAppVersion(String version);
    private HudLayout hud = new HudLayout();
    private boolean hudEditing;
    private int hudPointer = -1, hudPanel = -1;
    private float hudLastX, hudLastY;
    private int dragPointer = -1, dragControl = -1;
    private float dragOffsetX, dragOffsetY;
    private float scale = 1, offsetX, offsetY;
    private int insetLeft, insetRight, insetTop, insetBottom;
    private boolean visible = true, menuOverlayVisible;
    private int menuPointer = -1, menuGestureRevision, menuGesturePage;
    private float menuStartX, menuStartY, menuLastY;
    private boolean menuScrolled;

    private static native void nativeState(int lx, int ly, int rx, int ry,
                                          int lt, int rt, int buttons);

    public TouchControls(Context context) {
        super(context);
        sensors = (SensorManager) context.getSystemService(Context.SENSOR_SERVICE);
        gyroscope = sensors == null ? null : sensors.getDefaultSensor(Sensor.TYPE_GYROSCOPE);
        if (Build.VERSION.SDK_INT >= 31) {
            VibratorManager manager = (VibratorManager) context.getSystemService(Context.VIBRATOR_MANAGER_SERVICE);
            vibrator = manager == null ? null : manager.getDefaultVibrator();
        } else vibrator = (Vibrator) context.getSystemService(Context.VIBRATOR_SERVICE);
        preferences = context.getSharedPreferences("touch-layout-v1", Context.MODE_PRIVATE);
        hud = HudLayout.load(preferences.getString("hud-layout", null));
        nativeAppVersion(appVersion(context));
        try {
            java.io.File folder = context.getExternalFilesDir(null);
            if (folder == null) throw new java.io.IOException("Game storage is unavailable");
            startupCheats = new StartupCheats(new java.io.File(folder, "init.txt").toPath());
        } catch (java.io.IOException e) {
            Toast.makeText(context, "Cannot read init.txt: "+e.getMessage(), Toast.LENGTH_LONG).show();
        }
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
        sensitivity = preferences.getFloat("look-sensitivity", sensitivity > 0 ? sensitivity : 2f);
        if (!Float.isFinite(sensitivity) || sensitivity < 0.25f || sensitivity > 4f) sensitivity = 2f;
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
        dragPointer = dragControl = menuPointer = -1;
        hudPointer = hudPanel = -1;
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
        if (layout.overlayDisabled || editing || (menusActive && (!menuOverlayVisible || menuPage != 0))) {
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
        if (editing && inside(x, y, optionsX(), toolbarY(), 34)) return EDIT;
        if (editing && inside(x, y, optionsX()-156, toolbarY(), 34)) return EXPORT;
        if (editing && inside(x, y, optionsX()-78, toolbarY(), 34)) return IMPORT;
        if (!layout.overlayDisabled && inside(x, y, logicalWidth/2, toolbarY(), 28)) return TOGGLE;
        if (!overlayVisible()) return Integer.MIN_VALUE;
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

    @Override public boolean onTouchEvent(MotionEvent event) {
        int action = event.getActionMasked(), index = event.getActionIndex();
        int id = event.getPointerId(index);
        float x = (event.getX(index)-offsetX)/scale, y = (event.getY(index)-offsetY)/scale;
        if (editing) return editTouch(event, action, index, id, x, y);
        if (action == MotionEvent.ACTION_DOWN || action == MotionEvent.ACTION_POINTER_DOWN) {
            if (hit(x, y) == TOGGLE) {
                reset();
                if (menusActive) menuOverlayVisible = !menuOverlayVisible;
                else visible = !visible;
                owners.put(id, TOGGLE);
                performClick(); invalidate(); return true;
            }
        }
        if (owners.get(id, Integer.MIN_VALUE) == TOGGLE) {
            if (action == MotionEvent.ACTION_UP || action == MotionEvent.ACTION_POINTER_UP)
                owners.delete(id);
            return true;
        }
        if (menusActive && (menuPointer >= 0 || owners.size() == 0 && hit(x, y) == Integer.MIN_VALUE))
            return menuTouch(event, action, index, id);
        if (action == MotionEvent.ACTION_DOWN || action == MotionEvent.ACTION_POINTER_DOWN) {
            int control = hit(x, y);
            if (control == EDIT) {
                return true;
            } else if (control == TOGGLE) {
                reset(); visible = !visible; performClick();
            } else if (control != Integer.MIN_VALUE) {
                if (!menusActive && control >= 0 && layout.type(control) == TouchLayout.CAMERA) nativeCameraMode();
                owners.put(id, control);
                if (control >= 0 && layout.type(control) == LEFT) moveStick(control, x, y);
                else buttonTouches.put(id, new float[]{event.getX(index), event.getY(index)});
            } else if (!layout.overlayDisabled && !menusActive && visible && lookPointer < 0) {
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
                if (!menusActive && lookPointer < 0 && origin != null &&
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

    private boolean menuTouch(MotionEvent event, int action, int index, int id) {
        if (action == MotionEvent.ACTION_DOWN) {
            menuPointer = id; menuGestureRevision = revision; menuGesturePage = menuPage;
            menuStartX = event.getX(index); menuStartY = menuLastY = event.getY(index);
            menuScrolled = false;
            if (menuPage == 4 || menuPage == 5 || menuPage == 12 || menuPage == 13)
                nativeMenuPointer(menuStartX/getWidth(), menuStartY/getHeight(), true, false, 0, false);
        } else if (action == MotionEvent.ACTION_MOVE && menuPointer >= 0) {
            int p = event.findPointerIndex(menuPointer);
            if (p < 0 || menuGesturePage != menuPage) return true;
            float px = event.getX(p), py = event.getY(p);
            if (menuPage == 4 || menuPage == 5 || menuPage == 12 || menuPage == 13) {
                nativeMenuPointer(px/getWidth(), py/getHeight(), true, false, 0, true);
                menuScrolled = true;
            } else if (menuPage != 0) {
                boolean scrollbar = menuStartX/getWidth() > 0.72f;
                if (scrollbar || Math.abs(py-menuStartY) > 12*scale) {
                    menuScrolled = true;
                    nativeMenuPointer(px/getWidth(), py/getHeight(), false, false,
                        (menuLastY-py)/getHeight(), scrollbar);
                }
            }
            menuLastY = py;
        } else if ((action == MotionEvent.ACTION_UP || action == MotionEvent.ACTION_POINTER_UP) && id == menuPointer) {
            if (menuScrolled && (menuPage == 4 || menuPage == 5 || menuPage == 12 || menuPage == 13))
                nativeMenuPointer(event.getX(index)/getWidth(), event.getY(index)/getHeight(), true, false, 0, true);
            if (!menuScrolled && menuGestureRevision == revision) {
                nativeMenuPointer(event.getX(index)/getWidth(), event.getY(index)/getHeight(), true, false, 0, false);
                performClick();
            }
            menuPointer = -1;
        } else if (action == MotionEvent.ACTION_CANCEL) { reset(); }
        return true;
    }

    @Override public boolean performClick() { super.performClick(); return true; }

    private boolean editTouch(MotionEvent event, int action, int index, int id, float x, float y) {
        if (hudEditing) return hudTouch(event, action, id, x, y);
        if (action == MotionEvent.ACTION_DOWN || action == MotionEvent.ACTION_POINTER_DOWN) {
            if (dragPointer < 0) {
                int control = hit(x, y);
                if (control == EDIT) {
                    if (saveLayout()) {
                        reset(); editing = false; menuPage = 2; publishMenu(); updateSensors(); performClick();
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

    // Edit in-game UI: drag the three panels; Save and Reset sit at the bottom right,
    // clear of where the game draws its HUD.
    private float hudButtonY() { return logicalHeight-Math.max(46, insetBottom/scale+46); }
    private float hudSaveX() { return logicalWidth-Math.max(50, insetRight/scale+50); }
    private float hudResetX() { return hudSaveX()-84; }

    private boolean hudTouch(MotionEvent event, int action, int id, float x, float y) {
        if (action == MotionEvent.ACTION_DOWN || action == MotionEvent.ACTION_POINTER_DOWN) {
            if (hudPointer < 0) {
                if (inside(x, y, hudSaveX(), hudButtonY(), 36)) {
                    exitHudEditing(); performClick();
                } else if (inside(x, y, hudResetX(), hudButtonY(), 36)) {
                    hud.reset(); nativeHudLayout(hud.offsets()); performClick();
                } else {
                    int panel = hud.hit(x, y, logicalWidth, logicalHeight);
                    if (panel >= 0) { hudPointer = id; hudPanel = panel; hudLastX = x; hudLastY = y; }
                }
            }
        } else if (action == MotionEvent.ACTION_MOVE && hudPointer >= 0) {
            int pointer = event.findPointerIndex(hudPointer);
            if (pointer >= 0) {
                float nx = (event.getX(pointer)-offsetX)/scale, ny = (event.getY(pointer)-offsetY)/scale;
                hud.move(hudPanel, nx-hudLastX, ny-hudLastY, logicalWidth, logicalHeight);
                hudLastX = nx; hudLastY = ny;
                // The game's HUD follows the finger where it is drawn behind the editor.
                nativeHudLayout(hud.offsets());
            }
        } else if (action == MotionEvent.ACTION_CANCEL ||
                   ((action == MotionEvent.ACTION_UP || action == MotionEvent.ACTION_POINTER_UP) && id == hudPointer)) {
            hudPointer = hudPanel = -1;
        }
        invalidate();
        return true;
    }

    /** Saves the panels and returns to General; a failed save keeps the editor open. */
    private void exitHudEditing() {
        if (!preferences.edit().putString("hud-layout", hud.save()).commit()) {
            Toast.makeText(getContext(), "Could not save the in-game UI. Try again.", Toast.LENGTH_LONG).show();
            return;
        }
        reset(); hudEditing = false; editing = false; menuPage = 3;
        nativeHudLayout(hud.offsets()); revision++; publishMenu(); updateSensors(); invalidate();
    }

    private void drawHudEditor(Canvas canvas) {
        canvas.save(); canvas.translate(offsetX, offsetY); canvas.scale(scale, scale);
        for (int panel = 0; panel < HudLayout.COUNT; panel++) {
            float[] b = hud.bounds(panel, logicalWidth, logicalHeight);
            boolean active = panel == hudPanel;
            paint.setStyle(Paint.Style.FILL); paint.setColor(active ? 0x7783d9ff : 0x55304050);
            canvas.drawRoundRect(b[0], b[1], b[2], b[3], 10, 10, paint);
            paint.setStyle(Paint.Style.STROKE); paint.setStrokeWidth(2);
            paint.setColor(active ? 0xffaee7ff : 0xffffffff);
            canvas.drawRoundRect(b[0], b[1], b[2], b[3], 10, 10, paint);
            paint.setStyle(Paint.Style.FILL); paint.setColor(Color.WHITE);
            paint.setTextAlign(Paint.Align.CENTER); paint.setTextSize(16);
            canvas.drawText(HudLayout.name(panel), (b[0]+b[2])/2, (b[1]+b[3])/2+6, paint);
        }
        circle(canvas, hudSaveX(), hudButtonY(), 36, "SAVE", false, 13);
        circle(canvas, hudResetX(), hudButtonY(), 36, "RESET", false, 13);
        paint.setTextAlign(Paint.Align.CENTER); paint.setTextSize(14);
        canvas.drawText("DRAG THE RADAR, WEAPONS AND HEALTH PANELS, THEN SAVE",
            logicalWidth/2, hudButtonY()+5, paint);
        canvas.restore();
    }

    private float toolbarY() { return Math.max(38, insetTop/scale+38); }
    private float optionsX() { return logicalWidth-Math.max(42, insetRight/scale+42); }

    private void editorButton(Canvas canvas) {
        if (editing) circle(canvas, optionsX(), toolbarY(), 34, "SAVE", false);
        if (editing) {
            circle(canvas, optionsX()-156, toolbarY(), 34, "EXPORT", false);
            circle(canvas, optionsX()-78, toolbarY(), 34, "IMPORT", false);
            paint.setTextSize(14);
            canvas.drawText("DRAG CONTROLS, THEN SAVE AND EXIT", logicalWidth/2, toolbarY()+58, paint);
            paint.setTextSize(9);
            canvas.drawText("AND EXIT", optionsX(), toolbarY()+17, paint);
        }
    }


    private boolean menusActive = true;
    private int menuContext, menuPage, selectedControl, revision, currentFps;
    private static native int[] nativeMenuPoll();
    private static native void nativeMenuPointer(float x, float y, boolean click, boolean back, float scroll, boolean scrollbar);
    private static native void nativeMenuPublish(int revision, int page, boolean editing,
        String title, String[] labels, int[] actions, int[] values);
    private static final String[] CHEATS = {"Invincibility", "Jetpack", "Infinite ammo", "Bump possession",
        "Super jump", "Reflexive damage", "Medusa", "Omnipotent", "Controller cheats", "Bottomless clip",
        "Active camouflage (local player)", "Active camouflage", "All powerups", "All vehicles",
        "All weapons", "Teleport to camera"};
    private final Runnable menuPoll = new Runnable() {
        public void run() {
            if (!deviceInputActive) return;
            int[] state = nativeMenuPoll();
            if (state != null) {
                boolean active = state[0] != 0;
                if (active != menusActive || menuContext != state[1]) {
                    menusActive = active; menuContext = state[1];
                    if (hudEditing) {
                        // The menu went away under the editor: back to what was saved.
                        hudEditing = false;
                        hud = HudLayout.load(preferences.getString("hud-layout", null));
                        nativeHudLayout(hud.offsets());
                    }
                    menuPage = 0; editing = false; menuOverlayVisible = false;
                    reset(); cancelRumble(); updateSensors(); revision++; publishMenu();
                }
                if (state[2] != 0 && state[4] == revision) applyMenuAction(state[2], state[3]);
                currentFps = state[5];
                if (menuPage == 6) publishMenu();
                invalidate();
            }
            postDelayed(this, 16);
        }
    };

    public boolean menuBack() {
        if (!menusActive) return false;
        if (hudEditing) exitHudEditing();
        else if (editing) {
            if (saveLayout()) {
                editing = false; menuPage = 2; revision++; publishMenu(); invalidate();
            }
        }
        else nativeMenuPointer(0, 0, false, true, 0, false);
        return true;
    }

    private void applyMenuAction(int action, int value) {
        // Release overlay buttons before switching to a custom page.
        if (owners.size() != 0) reset();
        // A controller's B under the in-game UI editor leaves it as Save does.
        if (hudEditing) {
            if (action == 90) exitHudEditing();
            return;
        }
        if (action >= 3000) {
            int id = action-3000;
            if (id < 0 || id >= CHEATS.length) return;
            if (menuContext == 1) {
                try {
                    if (startupCheats == null) throw new java.io.IOException("Game storage is unavailable");
                    startupCheats.toggle(id);
                } catch (java.io.IOException e) {
                    Toast.makeText(getContext(), "Cannot save init.txt: "+e.getMessage(), Toast.LENGTH_LONG).show();
                }
            } else {
                int status = nativeCheatStatus(id);
                if (status != -2) nativeCheatRequest(id, id >= 10 || status != 1);
            }
        } else if (action >= 2000) {
            if (layout.add(action-2000) < 0)
                Toast.makeText(getContext(), "Control limit reached or movement stick already visible.", Toast.LENGTH_SHORT).show();
            else saveLayout();
            menuPage = 7;
        } else if (action >= 1000) {
            selectedControl = action-1000; menuPage = menuPage == 8 ? 12 : 9;
        } else if (action == 91) {
            openCreator();
            return;
        } else if (action == 90) {
            if (menuPage == 1) menuPage = 0;
            else if (menuPage == 2 || menuPage == 3 || menuPage == 6) menuPage = 1;
            else if (menuPage == 5 || menuPage == 13) menuPage = 3;
            else if (menuPage == 9 || menuPage == 10 || menuPage == 11) menuPage = 7;
            else if (menuPage == 12) menuPage = 8;
            else menuPage = 2;
        } else if (action == 12) { layout.overlayDisabled = false; saveLayout(); editing = true; visible = true; reset(); updateSensors(); }
        else if (action == 14) { hudEditing = true; editing = true; reset(); updateSensors(); }
        else if (action == 33) { layout.overlayDisabled = !layout.overlayDisabled; reset(); saveLayout(); }
        else if (action == 43) {
            layout.fieldOfView = TouchLayout.MIN_FOV+Math.max(0, Math.min(1000, value))*(TouchLayout.MAX_FOV-TouchLayout.MIN_FOV)/1000;
            nativeFieldOfView(layout.fieldOfView); saveLayout();
        }
        else if (action == 20) { layout.setShown(selectedControl, !layout.shown(selectedControl)); saveLayout(); }
        else if (action == 21) {
            if (layout.duplicate(selectedControl) < 0)
                Toast.makeText(getContext(), "Maximum "+TouchLayout.MAX_CONTROLS+" controls.", Toast.LENGTH_SHORT).show();
            else saveLayout();
        } else if (action == 22) { layout.resetDefaults(); saveLayout(); menuPage = 7; }
        else if (action == 30) { layout.rumbleEnabled = !layout.rumbleEnabled; cancelRumble(); saveLayout(); }
        else if (action == 31) { layout.gyroscopeEnabled = !layout.gyroscopeEnabled; updateSensors(); saveLayout(); }
        else if (action == 32) { layout.fpsCounter = !layout.fpsCounter; saveLayout(); }
        else if (action == 40 || action == 41) {
            float amount = 0.25f+Math.max(0, Math.min(1000, value))*3.75f/1000;
            if (action == 40) sensitivity = amount; else layout.gyroscopeSensitivity = amount;
            saveLayout();
        } else if (action == 42) {
            layout.setSize(selectedControl, 0.5f+Math.max(0, Math.min(1000, value))*1.5f/1000); saveLayout();
        } else { menuPage = action; }
        revision++; publishMenu(); invalidate();
    }

    /** the installed package's version name and code, as Android has them now */
    private static String appVersion(Context context) {
        try {
            android.content.pm.PackageInfo info = context.getPackageManager().getPackageInfo(context.getPackageName(), 0);
            long code = Build.VERSION.SDK_INT >= 28 ? info.getLongVersionCode() : info.versionCode;
            return versionLabel(info.versionName, code);
        } catch (android.content.pm.PackageManager.NameNotFoundException | RuntimeException e) {
            return versionLabel(BuildConfig.VERSION_NAME, BuildConfig.VERSION_CODE);
        }
    }

    /** a release's name already has its number ("build 186"); another build shows both */
    private static String versionLabel(String name, long code) {
        return name != null && name.startsWith("build") ? name : "v"+name+" ("+code+")";
    }

    private void openCreator() {
            try {
                getContext().startActivity(new android.content.Intent(android.content.Intent.ACTION_VIEW,
                    android.net.Uri.parse("https://github.com/cybersecurity/halo-ce-universal")));
            } catch (android.content.ActivityNotFoundException e) {
                Toast.makeText(getContext(), "No browser available", Toast.LENGTH_SHORT).show();
            }
    }

    private void publishMenu() {
        java.util.ArrayList<String> labels = new java.util.ArrayList<>();
        java.util.ArrayList<Integer> actions = new java.util.ArrayList<>(), values = new java.util.ArrayList<>();
        java.util.function.BiConsumer<String, Integer> row = (text, id) -> {
            labels.add(text); actions.add(id); values.add(-1);
        };
        String title = "Porting options";
        if (menuPage == 1) {
            row.accept("Overlay settings", 2); row.accept("General", 3); row.accept("Cheats", 6);
        } else if (menuPage == 2) {
            title = "Overlay settings";
            row.accept("Edit buttons layout", 12); row.accept("Hide or add buttons", 7);
            row.accept("Edit buttons size", 8); row.accept("Look sensitivity", 4);
            row.accept("Disable all overlay: "+(layout.overlayDisabled ? "ON" : "OFF"), 33);
        } else if (menuPage == 3) {
            title = "General";
            row.accept("Rumble: "+(vibrator == null || !vibrator.hasVibrator() ? "unavailable" : layout.rumbleEnabled ? "ON" : "OFF"),
                vibrator == null || !vibrator.hasVibrator() ? 0 : 30);
            row.accept("Gyroscope: "+(gyroscope == null ? "unavailable" : layout.gyroscopeEnabled ? "ON" : "OFF"), gyroscope == null ? 0 : 31);
            row.accept("Gyroscope sensitivity", 5); row.accept("FPS counter: "+(layout.fpsCounter ? "ON" : "OFF"), 32);
            row.accept("Field of view (FOV)", 13); row.accept("Edit in-game UI", 14);
        } else if (menuPage == 13) {
            title = "Field of view (FOV)";
            row.accept(String.format(java.util.Locale.US, "FOV: %.0f degrees", layout.fieldOfView), 43);
            values.set(0, Math.round((layout.fieldOfView-TouchLayout.MIN_FOV)/(TouchLayout.MAX_FOV-TouchLayout.MIN_FOV)*1000));
        } else if (menuPage == 4 || menuPage == 5 || menuPage == 12) {
            boolean size = menuPage == 12;
            title = size ? "Edit buttons size" : menuPage == 4 ? "Look sensitivity" : "Gyroscope sensitivity";
            float amount = size ? layout.sizeScale(selectedControl) : menuPage == 4 ? sensitivity : layout.gyroscopeSensitivity;
            row.accept(String.format(java.util.Locale.US, size ? "%s: %.0f%%" : "%s: %.2fx",
                size ? controlName(layout.type(selectedControl)) : title, size ? amount*100 : amount), size ? 42 : menuPage == 4 ? 40 : 41);
            values.set(0, Math.round((amount-(size ? 0.5f : 0.25f))/(size ? 1.5f : 3.75f)*1000));
        } else if (menuPage == 6) {
            title = menuContext == 1 ? "Startup cheats" : "Cheats";
            for (int i = 0; i < CHEATS.length; i++) {
                if (menuContext == 1) {
                    row.accept(CHEATS[i]+(startupCheats != null && startupCheats.enabled(i) ? " [ON]" : " [OFF]"), startupCheats == null ? 0 : 3000+i);
                    continue;
                }
                int status = nativeCheatStatus(i);
                row.accept(CHEATS[i]+(status == -2 ? " ..." : status == -1 ? " (unavailable)" : i < 10 ? status == 1 ? " [ON]" : " [OFF]" : " (instant)"), 3000+i);
            }
        } else if (menuPage == 7 || menuPage == 8) {
            title = menuPage == 7 ? "Hide or add buttons" : "Edit buttons size";
            for (int i = 0; i < layout.size(); i++)
                row.accept(controlName(layout.type(i))+(i >= TouchLayout.BASE_COUNT ? " #"+(i+1) : "")+
                    (menuPage == 7 ? layout.shown(i) ? " [visible]" : " [hidden]" : " "+Math.round(layout.sizeScale(i)*100)+"%"), 1000+i);
        } else if (menuPage == 9) {
            title = controlName(layout.type(selectedControl));
            row.accept(layout.shown(selectedControl) ? "Hide button" : "Show button", 20);
            if (layout.type(selectedControl) != LEFT) row.accept("Duplicate button", 21);
            row.accept("Add button", 10); row.accept("Reset all buttons", 11);
        } else if (menuPage == 10) {
            title = "Add button";
            for (int i = 0; i < TouchLayout.BASE_COUNT; i++) row.accept(controlName(i), 2000+i);
        } else if (menuPage == 11) {
            title = "Restore defaults and remove copies?"; row.accept("Reset all buttons", 22);
        }
        if (menuPage != 0) row.accept("Back", 90);
        int[] ids = new int[actions.size()], progress = new int[values.size()];
        for (int i = 0; i < ids.length; i++) { ids[i] = actions.get(i); progress[i] = values.get(i); }
        // The game's menus are set in capitals; the Porting screens match them.
        String[] rows = new String[labels.size()];
        for (int i = 0; i < rows.length; i++) rows[i] = labels.get(i).toUpperCase(java.util.Locale.ROOT);
        // Keep the revision stable for status refreshes so pending taps remain valid.
        nativeMenuPublish(revision, menuPage, editing, title.toUpperCase(java.util.Locale.ROOT), rows, ids, progress);
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
        visible = true; nativeFieldOfView(layout.fieldOfView); updateSensors(); cancelRumble(); invalidate();
    }

    private String controlName(int type) { return type == LEFT ? "Move stick" : buttons[type].label; }

    public void startDeviceInput() {
        if (deviceInputActive) return;
        deviceInputActive = true;
        nativeFieldOfView(layout.fieldOfView); nativeHudLayout(hud.offsets());
        gyroAim.reset(); updateSensors(); post(rumblePoll); post(menuPoll);
    }

    public void stopDeviceInput() {
        deviceInputActive = false; removeCallbacks(rumblePoll); removeCallbacks(menuPoll);
        updateSensors(); cancelRumble(); reset();
    }

    private void updateSensors() {
        boolean needed = deviceInputActive && !menusActive && !editing && layout.gyroscopeEnabled && gyroscope != null;
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
        if (!deviceInputActive || !layout.gyroscopeEnabled || editing || menusActive) {
            gyroAim.reset(); return;
        }
        int rotation = ((WindowManager)getContext().getSystemService(Context.WINDOW_SERVICE))
            .getDefaultDisplay().getRotation();
        if (gyroAim.sample(event.timestamp, event.values[0], event.values[1], rotation, gyroDelta))
            // The existing direct-look path accepts logical pixels (0.0022 radians per pixel).
            nativeLook(-gyroDelta[0]/0.0022f*layout.gyroscopeSensitivity, -gyroDelta[1]/0.0022f*layout.gyroscopeSensitivity);
    }

    @Override public void onAccuracyChanged(Sensor sensor, int accuracy) {}

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
        circle(canvas, x, y, layout.radius(LEFT), "", false, 11*layout.sizeScale(LEFT));
        circle(canvas, x + axes[axis]/32767f*layout.radius(LEFT), y + axes[axis+1]/32767f*layout.radius(LEFT),
               24*layout.sizeScale(LEFT), "", held(LEFT));
        TouchIcons.draw(canvas, paint, LEFT, x + axes[axis]/32767f*layout.radius(LEFT),
            y + axes[axis+1]/32767f*layout.radius(LEFT), 18*layout.sizeScale(LEFT), held(LEFT));
    }

    private boolean overlayVisible() {
        return !layout.overlayDisabled && (editing || (menusActive ? menuOverlayVisible : visible));
    }

    @Override protected void onDraw(Canvas canvas) {
        if (hudEditing) { drawHudEditor(canvas); return; }
        if (layout.overlayDisabled) return;
        if (layout.fpsCounter && !menusActive && !editing) {
            paint.setStyle(Paint.Style.FILL); paint.setColor(Color.WHITE);
            paint.setTextAlign(Paint.Align.LEFT); paint.setTextSize(18*scale);
            canvas.drawText("FPS: "+currentFps, insetLeft+12*scale, insetTop+24*scale, paint);
        }
        canvas.save(); canvas.translate(offsetX, offsetY); canvas.scale(scale, scale);
        if (!editing) {
            circle(canvas, logicalWidth/2, toolbarY(), 28, "", false);
            TouchIcons.draw(canvas, paint, 19, logicalWidth/2, toolbarY(), 20, menusActive ? menuOverlayVisible : visible);
        }
        if (overlayVisible()) {
            if (layout.shown(LEFT)) stick(canvas, 0, layout.x(LEFT), layout.y(LEFT), "Move");
            for (int i = 0; i < layout.size(); i++) {
                if (!layout.shown(i) || layout.type(i) == LEFT) continue;
                Button b = buttons[layout.type(i)];
                boolean active = held(i) || dragControl == i;
                circle(canvas, layout.x(i), layout.y(i), layout.radius(i), "", active, 11*layout.sizeScale(i));
                TouchIcons.draw(canvas, paint, layout.type(i), layout.x(i), layout.y(i), layout.radius(i)*0.72f, active);
            }
        }
        editorButton(canvas);
        canvas.restore();
    }
}
