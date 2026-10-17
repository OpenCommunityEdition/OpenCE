package com.halo.decomp;

import java.io.IOException;
import java.io.StringReader;
import java.io.StringWriter;
import java.util.ArrayList;
import java.util.Properties;

final class TouchLayout {
    static final int LEFT = 16;
    static final int FIRE_LEFT = 17;
    static final int BASE_COUNT = 18;

    // Toggle button on-screen (index 18)
    static final int TB_HIDE = 18;
    static final int TOTAL_TYPES = 19;
    static final int TYPE_CUSTOM = 99;

    static final int MAX_CONTROLS = 64;
    static final float MIN_SIZE = 0.4f;
    static final float MAX_SIZE = 2.5f;
    static final float MIN_OPACITY = 0.1f;
    static final float MAX_OPACITY = 1f;
    static final float MIN_SENSITIVITY = 0.25f;
    static final float MAX_SENSITIVITY = 4f;
    static final float MAX_MINIMUM_RADIUS = 34f;

    // Uniform standard button radius: all buttons are identical in size at 100%
    static final float BUTTON_BASE_RADIUS = 32f;
    static final float STICK_BASE_RADIUS = 64f;

    static final int MODE_STANDARD = 0;
    static final int MODE_TAP_AND_HOLD = 1;
    static final int MODE_TOGGLE = 2;

    private static final float GRID_WIDTH = 960;
    private static final float GRID_HEIGHT = 540;
    private static final int VERSION = 6;
    private static final int MAX_FILE_LENGTH = 65536;

    private static final float[][] DEFAULTS = {
        // Gameplay Controls (0..17)
        {856, 447}, {913, 377}, {794, 377}, {850, 312}, {915, 239}, {802, 239},
        {236, 449}, {691, 449}, {360, 490}, {438, 490}, {570, 36}, {390, 36},
        {100, 237}, {100, 335}, {51, 286}, {149, 286}, {115, 440}, {255, 239},
        // Toggle Controls button (18)
        {480, 38}
    };

    private static final float[] RADII = {
        36, 32, 34, 32, 39, 35, 32, 32, 27, 29, 28, 28, 25, 25, 25, 25, 64, 39,
        30
    };

    static final class Control {
        int type;
        float x;
        float y;
        boolean shown = true;
        float size = 1f;
        float opacity = -1f;
        int mode = MODE_STANDARD;
        int longType = -1;
        int customKeycode = -1;
        int longKeycode = -1;
        String customLabel = "";
        String iconFile = "";
        String knobFile = "";

        Control(int type, float x, float y) {
            this.type = type;
            this.x = x;
            this.y = y;
        }
    }

    static final class Configuration {
        final TouchLayout layout;
        final float sensitivity;

        Configuration(TouchLayout layout, float sensitivity) {
            this.layout = layout;
            this.sensitivity = sensitivity;
        }
    }

    private final ArrayList<Control> controls = new ArrayList<>();
    private float width = GRID_WIDTH;
    private float height = GRID_HEIGHT;
    private float minimumRadius;

    boolean rumbleEnabled = true;
    boolean gyroscopeEnabled = false;
    float opacity = 1f;
    boolean floatingStick = false;

    TouchLayout() {
        resetDefaults();
    }

    void bounds(float width, float height) {
        if (!Float.isFinite(width) || !Float.isFinite(height) || width <= 0 || height <= 0)
            throw new IllegalArgumentException("Invalid display dimensions");
        for (Control control : controls) {
            control.x *= width / this.width;
            control.y *= height / this.height;
        }
        this.width = width;
        this.height = height;
        keepAllInside();
    }

    void setMinimumRadius(float radius) {
        minimumRadius = Float.isFinite(radius) && radius > 0 ? Math.min(radius, MAX_MINIMUM_RADIUS) : 0;
        keepAllInside();
    }

    int size() { return controls.size(); }
    int type(int control) { return controls.get(control).type; }
    void setType(int control, int type) { controls.get(control).type = type; }
    float x(int control) { return controls.get(control).x; }
    float y(int control) { return controls.get(control).y; }
    float savedX(int control) { return x(control) * GRID_WIDTH / width; }
    float savedY(int control) { return y(control) * GRID_HEIGHT / height; }

    // Uniform sizing: all buttons scale down smoothly without being clamped by a rigid minimum
    float radius(int control) {
        int t = type(control);
        float baseR = (t == LEFT) ? STICK_BASE_RADIUS : BUTTON_BASE_RADIUS;
        float scaled = baseR * sizeScale(control);
        return Math.max(scaled, (t == LEFT) ? 24f : 10f);
    }

    float sizeScale(int control) { return controls.get(control).size; }

    void setSize(int control, float size) {
        if (!Float.isFinite(size) || size < MIN_SIZE || size > MAX_SIZE)
            throw new IllegalArgumentException("Button size must be between 40% and 250%");
        controls.get(control).size = size;
        move(control, x(control), y(control));
    }

    float controlOpacity(int control) {
        float op = controls.get(control).opacity;
        return op < 0 ? opacity : op;
    }

    float rawControlOpacity(int control) { return controls.get(control).opacity; }

    void setControlOpacity(int control, float op) {
        if (op < 0) {
            controls.get(control).opacity = -1f;
        } else {
            controls.get(control).opacity = Math.max(MIN_OPACITY, Math.min(MAX_OPACITY, op));
        }
    }

    int mode(int control) { return controls.get(control).mode; }
    void setMode(int control, int mode) { controls.get(control).mode = mode; }
    int longType(int control) { return controls.get(control).longType; }
    void setLongType(int control, int type) { controls.get(control).longType = type; }

    int customKeycode(int control) { return controls.get(control).customKeycode; }
    void setCustomKeycode(int control, int code) { controls.get(control).customKeycode = code; }
    int longKeycode(int control) { return controls.get(control).longKeycode; }
    void setLongKeycode(int control, int code) { controls.get(control).longKeycode = code; }

    String customLabel(int control) { return controls.get(control).customLabel; }
    void setCustomLabel(int control, String lbl) { controls.get(control).customLabel = lbl != null ? lbl : ""; }

    String iconFile(int control) { return controls.get(control).iconFile; }
    void setIconFile(int control, String file) { controls.get(control).iconFile = file != null ? file : ""; }
    String knobFile(int control) { return controls.get(control).knobFile; }
    void setKnobFile(int control, String file) { controls.get(control).knobFile = file != null ? file : ""; }

    boolean shown(int control) { return controls.get(control).shown; }
    void setShown(int control, boolean shown) { controls.get(control).shown = shown; }

    void setOpacity(float opacity) {
        if (!validOpacity(opacity))
            throw new IllegalArgumentException("Opacity must be between 10% and 100%");
        this.opacity = opacity;
    }

    void resetDefaults() {
        resetAllDefaults(true);
    }

    void resetAllDefaults(boolean resetIcons) {
        ArrayList<String> icons = new ArrayList<>();
        ArrayList<String> knobs = new ArrayList<>();
        if (!resetIcons) {
            for (Control c : controls) {
                icons.add(c.iconFile);
                knobs.add(c.knobFile);
            }
        }

        controls.clear();
        for (int i = 0; i < DEFAULTS.length; i++) {
            Control c = new Control(i, DEFAULTS[i][0] * width / GRID_WIDTH, DEFAULTS[i][1] * height / GRID_HEIGHT);
            if (!resetIcons && i < icons.size()) {
                c.iconFile = icons.get(i);
                c.knobFile = knobs.get(i);
            }
            controls.add(c);
        }
        keepAllInside();
    }

    void resetSingleControl(int control, boolean resetIcon) {
        if (control < 0 || control >= controls.size()) return;
        Control c = controls.get(control);
        int t = c.type;
        if (t >= 0 && t < DEFAULTS.length) {
            c.x = DEFAULTS[t][0] * width / GRID_WIDTH;
            c.y = DEFAULTS[t][1] * height / GRID_HEIGHT;
        }
        c.size = 1f;
        c.opacity = -1f;
        c.shown = true;
        c.mode = MODE_STANDARD;
        c.longType = -1;
        c.customKeycode = -1;
        c.longKeycode = -1;
        c.customLabel = "";
        if (resetIcon) {
            c.iconFile = "";
            c.knobFile = "";
        }
        move(control, c.x, c.y);
    }

    int duplicate(int source) {
        if (size() >= MAX_CONTROLS || type(source) == LEFT || type(source) == TB_HIDE)
            return -1;
        float offset = radius(source) * 2 + 12;
        Control src = controls.get(source);
        Control added = new Control(src.type, x(source) + offset, y(source));
        added.size = src.size;
        added.opacity = src.opacity;
        added.mode = src.mode;
        added.longType = src.longType;
        added.customKeycode = src.customKeycode;
        added.longKeycode = src.longKeycode;
        added.customLabel = src.customLabel;
        added.iconFile = src.iconFile;
        added.knobFile = src.knobFile;
        controls.add(added);
        int index = size() - 1;
        if (added.x > width - radius(index))
            added.x = x(source) - offset;
        move(index, added.x, added.y);
        return index;
    }

    int add(int type) {
        if (type == LEFT || type == TB_HIDE) {
            for (int i = 0; i < size(); i++) {
                if (type(i) == type) {
                    setShown(i, true);
                    return i;
                }
            }
            return -1;
        }

        for (int i = 0; i < size(); i++) {
            if (type(i) == type && !shown(i)) {
                setShown(i, true);
                return i;
            }
        }
        return duplicate(type < BASE_COUNT ? type : 0);
    }

    int addCustomButton(String label, int keycode) {
        if (size() >= MAX_CONTROLS) return -1;
        Control added = new Control(TYPE_CUSTOM, width / 2, height / 2);
        added.customLabel = label;
        added.customKeycode = keycode;
        controls.add(added);
        int index = size() - 1;
        move(index, added.x, added.y);
        return index;
    }

    boolean removeControl(int control) {
        if (control < 0 || control >= controls.size()) return false;
        if (type(control) == TB_HIDE) return false;

        if (control < BASE_COUNT || type(control) == LEFT) {
            setShown(control, false);
            return true;
        }

        controls.remove(control);
        return true;
    }

    void move(int control, float x, float y) {
        if (!Float.isFinite(x) || !Float.isFinite(y))
            return;
        float r = radius(control);
        controls.get(control).x = Math.max(r, Math.min(width - r, x));
        controls.get(control).y = Math.max(r, Math.min(height - r, y));
    }

    private void keepAllInside() {
        for (int i = 0; i < size(); i++)
            move(i, x(i), y(i));
    }

    String exportConfiguration(float sensitivity) {
        if (!validSensitivity(sensitivity))
            throw new IllegalArgumentException("Invalid sensitivity");
        Properties values = new Properties();
        values.setProperty("format", "halo-touch-layout");
        values.setProperty("version", Integer.toString(VERSION));
        values.setProperty("rumble", Boolean.toString(rumbleEnabled));
        values.setProperty("gyroscope", Boolean.toString(gyroscopeEnabled));
        values.setProperty("opacity", Float.toString(opacity));
        values.setProperty("floating-stick", Boolean.toString(floatingStick));
        values.setProperty("count", Integer.toString(size()));
        values.setProperty("sensitivity", Float.toString(sensitivity));
        for (int i = 0; i < size(); i++) {
            Control c = controls.get(i);
            String key = "control." + i + ".";
            values.setProperty(key + "type", Integer.toString(c.type));
            values.setProperty(key + "x", Float.toString(savedX(i)));
            values.setProperty(key + "y", Float.toString(savedY(i)));
            values.setProperty(key + "visible", Boolean.toString(c.shown));
            values.setProperty(key + "size", Float.toString(c.size));
            values.setProperty(key + "opacity", Float.toString(c.opacity));
            values.setProperty(key + "mode", Integer.toString(c.mode));
            values.setProperty(key + "longType", Integer.toString(c.longType));
            values.setProperty(key + "customKeycode", Integer.toString(c.customKeycode));
            values.setProperty(key + "longKeycode", Integer.toString(c.longKeycode));
            values.setProperty(key + "customLabel", c.customLabel != null ? c.customLabel : "");
            values.setProperty(key + "iconFile", c.iconFile != null ? c.iconFile : "");
            values.setProperty(key + "knobFile", c.knobFile != null ? c.knobFile : "");
        }
        StringWriter text = new StringWriter();
        try {
            values.store(text, "Halo Android touch layout");
        } catch (IOException e) {
            throw new IllegalStateException(e);
        }
        return text.toString();
    }

    static Configuration importConfiguration(String text) {
        if (text == null || text.length() > MAX_FILE_LENGTH)
            throw new IllegalArgumentException("Invalid layout file");
        Properties values = new Properties();
        try {
            values.load(new StringReader(text));
            int version = readVersion(values);
            int count = Integer.parseInt(values.getProperty("count"));
            float sensitivity = Float.parseFloat(values.getProperty("sensitivity"));
            if (count < BASE_COUNT || count > MAX_CONTROLS || !validSensitivity(sensitivity))
                throw new IllegalArgumentException("Invalid layout settings");
            TouchLayout layout = new TouchLayout();
            layout.controls.clear();
            if (version >= 2) {
                layout.rumbleEnabled = readBoolean(values, "rumble");
                layout.gyroscopeEnabled = readBoolean(values, "gyroscope");
            }
            if (version >= 3) {
                float opacity = Float.parseFloat(values.getProperty("opacity"));
                if (!validOpacity(opacity))
                    throw new IllegalArgumentException("Invalid opacity");
                layout.opacity = opacity;
                layout.floatingStick = readBoolean(values, "floating-stick");
            }
            for (int i = 0; i < count; i++)
                layout.controls.add(readControl(values, i, version));
            if (count < TOTAL_TYPES) {
                for (int i = count; i < TOTAL_TYPES; i++)
                    layout.controls.add(new Control(i, DEFAULTS[i][0] * layout.width / GRID_WIDTH, DEFAULTS[i][1] * layout.height / GRID_HEIGHT));
            }
            return new Configuration(layout, sensitivity);
        } catch (IOException | NullPointerException e) {
            throw new IllegalArgumentException("Incomplete layout file", e);
        }
    }

    private static int readVersion(Properties values) {
        if (!"halo-touch-layout".equals(values.getProperty("format")))
            throw new IllegalArgumentException("Unsupported layout file");
        String version = values.getProperty("version");
        for (int known = 1; known <= VERSION; known++) {
            if (Integer.toString(known).equals(version))
                return known;
        }
        throw new IllegalArgumentException("Unsupported layout file");
    }

    private static Control readControl(Properties values, int index, int version) {
        String key = "control." + index + ".";
        int type = Integer.parseInt(values.getProperty(key + "type"));
        float x = Float.parseFloat(values.getProperty(key + "x"));
        float y = Float.parseFloat(values.getProperty(key + "y"));
        String shown = values.getProperty(key + "visible");
        float size = version >= 2 ? Float.parseFloat(values.getProperty(key + "size")) : 1f;
        boolean validType = type >= 0 && (type < 32 || type == TYPE_CUSTOM);
        boolean validPlace = Float.isFinite(x) && Float.isFinite(y) && x >= 0 && x <= GRID_WIDTH
            && y >= 0 && y <= GRID_HEIGHT;
        boolean validSize = Float.isFinite(size) && size >= MIN_SIZE && size <= MAX_SIZE;
        if (!validType || !validPlace || !validSize || !("true".equals(shown) || "false".equals(shown)))
            throw new IllegalArgumentException("Invalid control in layout file");
        Control control = new Control(type, x, y);
        control.shown = Boolean.parseBoolean(shown);
        control.size = size;
        if (version >= 4) {
            String op = values.getProperty(key + "opacity");
            if (op != null) control.opacity = Float.parseFloat(op);
            String m = values.getProperty(key + "mode");
            if (m != null) control.mode = Integer.parseInt(m);
            String lt = values.getProperty(key + "longType");
            if (lt != null) control.longType = Integer.parseInt(lt);
            String ck = values.getProperty(key + "customKeycode");
            if (ck != null) control.customKeycode = Integer.parseInt(ck);
            control.customLabel = values.getProperty(key + "customLabel", "");
            control.iconFile = values.getProperty(key + "iconFile", "");
            control.knobFile = values.getProperty(key + "knobFile", "");
        }
        if (version >= 5) {
            String lk = values.getProperty(key + "longKeycode");
            if (lk != null) control.longKeycode = Integer.parseInt(lk);
        }
        return control;
    }

    private static boolean readBoolean(Properties values, String key) {
        String value = values.getProperty(key);
        if (!("true".equals(value) || "false".equals(value)))
            throw new IllegalArgumentException("Invalid " + key + " option");
        return Boolean.parseBoolean(value);
    }

    static boolean validSensitivity(float value) {
        return Float.isFinite(value) && value >= MIN_SENSITIVITY && value <= MAX_SENSITIVITY;
    }

    static boolean validOpacity(float value) {
        return Float.isFinite(value) && value >= MIN_OPACITY && value <= MAX_OPACITY;
    }
}
