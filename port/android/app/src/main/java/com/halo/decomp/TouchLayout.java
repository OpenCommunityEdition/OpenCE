package com.halo.decomp;

import java.io.IOException;
import java.io.StringReader;
import java.io.StringWriter;
import java.util.ArrayList;
import java.util.Properties;

/** Display-relative positions, visibility and action types for touch controls. */
final class TouchLayout {
    static final int LEFT = 16, FIRE_LEFT = 17, CAMERA = 18;
    private static final float[][] DEFAULTS = {
        {811.36505f,411.75003f}, {877.21124f,366.6001f}, {883.5595f,281.63678f},
        {843.4512f,219.71782f}, {808.50323f,310.98508f}, {777.5762f,219.08284f},
        {893.86035f,484.31088f}, {558.1477f,504.46585f}, {420.64713f,502.42935f},
        {488.61844f,501.0651f}, {570,36}, {390,36}, {247.43086f,320.87274f},
        {234.66917f,405.15228f}, {207.84561f,351.7265f}, {272.89108f,368.7666f},
        {115.00001f,440}, {112.52527f,287.46466f}, {650,36}
    };
    private static final float[] DEFAULT_SIZES = {
        1.1f,1.2f,1f,1f,1.3f,1f,1f,0.9f,1f,1f,1f,1f,1f,1f,1f,1f,1f,1f,1f
    };
    private static final float[] RADII = {
        36,32,34,32,39,35,32,32,27,29,28,28,25,25,25,25,64,39,29
    };
    // One extra slot preserves every copy when migrating a full 64-control layout.
    static final int BASE_COUNT = 19, MAX_CONTROLS = 65;
    private static final class Control {
        final int type;
        float x, y;
        boolean shown = true;
        float size = 1f;
        Control(int type, float x, float y) { this.type = type; this.x = x; this.y = y; }
    }
    private final ArrayList<Control> controls = new ArrayList<>();

    private float width = 960, height = 540;
    boolean rumbleEnabled = true, gyroscopeEnabled = false, fpsCounter = false;
    float gyroscopeSensitivity = 1f;
    boolean overlayDisabled;
    float fieldOfView = 70f;
    static final float MIN_FOV = 55f, MAX_FOV = 90f;
    static final float MIN_SIZE = 0.5f, MAX_SIZE = 2f;

    void bounds(float width, float height) {
        if (!Float.isFinite(width) || !Float.isFinite(height) || width <= 0 || height <= 0)
            throw new IllegalArgumentException("Invalid display dimensions");
        for (Control control : controls) {
            control.x *= width / this.width;
            control.y *= height / this.height;
        }
        this.width = width; this.height = height;
        for (int i = 0; i < size(); i++) move(i, x(i), y(i));
    }
    float savedX(int i) { return x(i) * 960 / width; }
    float savedY(int i) { return y(i) * 540 / height; }

    TouchLayout() { resetDefaults(); }
    int size() { return controls.size(); }
    int type(int control) { return controls.get(control).type; }
    float x(int control) { return controls.get(control).x; }
    float y(int control) { return controls.get(control).y; }
    float radius(int control) { return RADII[type(control)] * controls.get(control).size; }
    float sizeScale(int control) { return controls.get(control).size; }
    void setSize(int control, float size) {
        if (!Float.isFinite(size) || size < MIN_SIZE || size > MAX_SIZE)
            throw new IllegalArgumentException("Button size must be between 50% and 200%");
        controls.get(control).size = size;
        move(control, x(control), y(control));
    }
    boolean shown(int control) { return controls.get(control).shown; }
    void setShown(int control, boolean shown) { controls.get(control).shown = shown; }

    void resetDefaults() {
        controls.clear();
        for (int i = 0; i < DEFAULTS.length; i++) {
            Control control = new Control(i, DEFAULTS[i][0]*width/960, DEFAULTS[i][1]*height/540);
            control.size = DEFAULT_SIZES[i];
            controls.add(control);
        }
        for (int i = 0; i < size(); i++) move(i, x(i), y(i));
    }

    int duplicate(int source) {
        if (size() >= MAX_CONTROLS || type(source) == LEFT) return -1;
        Control added = new Control(type(source), x(source)+radius(source)*2+12, y(source));
        added.size = sizeScale(source);
        controls.add(added);
        int index = size()-1;
        // If there is no space on the right, place the copy to the left instead.
        if (added.x > width-radius(source)) added.x = x(source)-radius(source)*2-12;
        move(index, added.x, added.y);
        return index;
    }

    int add(int type) {
        if (type < 0 || type >= BASE_COUNT) return -1;
        for (int i = 0; i < size(); i++) {
            if (type(i) == type && !shown(i)) { setShown(i, true); return i; }
        }
        return duplicate(type);
    }

    String exportConfiguration(float sensitivity) {
        if (!validSensitivity(sensitivity) || !validSensitivity(gyroscopeSensitivity)) throw new IllegalArgumentException("Invalid sensitivity");
        Properties values = new Properties();
        values.setProperty("format", "halo-touch-layout");
        values.setProperty("version", "4");
        values.setProperty("gyroscope-sensitivity", Float.toString(gyroscopeSensitivity));
        values.setProperty("fps-counter", Boolean.toString(fpsCounter));
        values.setProperty("rumble", Boolean.toString(rumbleEnabled));
        values.setProperty("gyroscope", Boolean.toString(gyroscopeEnabled));
        values.setProperty("overlay-disabled", Boolean.toString(overlayDisabled));
        values.setProperty("field-of-view", Float.toString(fieldOfView));
        values.setProperty("count", Integer.toString(size()));
        values.setProperty("sensitivity", Float.toString(sensitivity));
        for (int i = 0; i < size(); i++) {
            String key = "control."+i+".";
            values.setProperty(key+"type", Integer.toString(type(i)));
            values.setProperty(key+"x", Float.toString(savedX(i)));
            values.setProperty(key+"y", Float.toString(savedY(i)));
            values.setProperty(key+"visible", Boolean.toString(shown(i)));
            values.setProperty(key+"size", Float.toString(sizeScale(i)));
        }
        StringWriter text = new StringWriter();
        try { values.store(text, "Halo Android touch layout"); }
        catch (IOException e) { throw new IllegalStateException(e); }
        return text.toString();
    }

    static final class Configuration {
        final TouchLayout layout;
        final float sensitivity;
        Configuration(TouchLayout layout, float sensitivity) {
            this.layout = layout; this.sensitivity = sensitivity;
        }
    }

    private static boolean validSensitivity(float value) {
        return Float.isFinite(value) && value >= 0.25f && value <= 4f;
    }

    static Configuration importConfiguration(String text) {
        if (text == null || text.length() > 65536) throw new IllegalArgumentException("Invalid layout file");
        Properties values = new Properties();
        try {
            values.load(new StringReader(text));
            String version = values.getProperty("version");
            if (!"halo-touch-layout".equals(values.getProperty("format")) ||
                    !("1".equals(version) || "2".equals(version) || "3".equals(version) || "4".equals(version)))
                throw new IllegalArgumentException("Unsupported layout file");
            int count = Integer.parseInt(values.getProperty("count"));
            float sensitivity = Float.parseFloat(values.getProperty("sensitivity"));
            if (count < 18 || count > MAX_CONTROLS || !validSensitivity(sensitivity))
                throw new IllegalArgumentException("Invalid layout settings");
            TouchLayout layout = new TouchLayout();
            layout.controls.clear();
            if (values.containsKey("overlay-disabled")) layout.overlayDisabled = readBoolean(values, "overlay-disabled");
            if (values.containsKey("field-of-view")) {
                layout.fieldOfView = Float.parseFloat(values.getProperty("field-of-view"));
                if (!Float.isFinite(layout.fieldOfView) || layout.fieldOfView < MIN_FOV || layout.fieldOfView > MAX_FOV)
                    throw new IllegalArgumentException("Invalid field of view");
            }
            if (!"1".equals(version)) {
                layout.rumbleEnabled = readBoolean(values, "rumble");
                layout.gyroscopeEnabled = readBoolean(values, "gyroscope");
                if ("3".equals(version) || "4".equals(version)) {
                    layout.fpsCounter = readBoolean(values, "fps-counter");
                    layout.gyroscopeSensitivity = Float.parseFloat(values.getProperty("gyroscope-sensitivity"));
                    if (!validSensitivity(layout.gyroscopeSensitivity))
                        throw new IllegalArgumentException("Invalid gyroscope sensitivity");
                }
            }
            int base = "4".equals(version) ? BASE_COUNT : 18;
            if (count < base) throw new IllegalArgumentException("Incomplete base controls");
            for (int i = 0; i < count; i++) {
                String key = "control."+i+".";
                int type = Integer.parseInt(values.getProperty(key+"type"));
                float x = Float.parseFloat(values.getProperty(key+"x"));
                float y = Float.parseFloat(values.getProperty(key+"y"));
                String shown = values.getProperty(key+"visible");
                float size = !"1".equals(version) ? Float.parseFloat(values.getProperty(key+"size")) : 1f;
                if (type < 0 || type >= base || (i < base && type != i) ||
                        (i >= base && type == LEFT) || !Float.isFinite(x) || !Float.isFinite(y) ||
                        x < 0 || x > 960 || y < 0 || y > 540 ||
                        !("true".equals(shown) || "false".equals(shown)) ||
                        !Float.isFinite(size) || size < MIN_SIZE || size > MAX_SIZE)
                    throw new IllegalArgumentException("Invalid control in layout file");
                Control control = new Control(type, x, y);
                control.shown = Boolean.parseBoolean(shown);
                control.size = size;
                layout.controls.add(control);
            }
            if (base == 18) {
                Control camera = new Control(CAMERA, DEFAULTS[CAMERA][0], DEFAULTS[CAMERA][1]);
                layout.controls.add(CAMERA, camera);
            }
            return new Configuration(layout, sensitivity);
        } catch (IOException | NullPointerException e) {
            throw new IllegalArgumentException("Incomplete layout file", e);
        }
    }

    private static boolean readBoolean(Properties values, String key) {
        String value = values.getProperty(key);
        if (!("true".equals(value) || "false".equals(value)))
            throw new IllegalArgumentException("Invalid "+key+" option");
        return Boolean.parseBoolean(value);
    }

    void move(int control, float x, float y) {
        if (!Float.isFinite(x) || !Float.isFinite(y)) return;
        float radius = radius(control);
        controls.get(control).x = Math.max(radius, Math.min(width-radius, x));
        controls.get(control).y = Math.max(radius, Math.min(height-radius, y));
    }

    void restore(int control, float x, float y) {
        if (Float.isFinite(x) && Float.isFinite(y) && x >= 0
                && x <= 960 && y >= 0 && y <= 540) {
            controls.get(control).x = x;
            controls.get(control).y = y;
        }
    }
}
