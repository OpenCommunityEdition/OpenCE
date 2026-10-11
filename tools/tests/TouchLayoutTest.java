package com.halo.decomp;

/** Standalone geometry/persistence regression checks; no phone required. */
public final class TouchLayoutTest {
    private static void check(boolean condition, String message) {
        if (!condition) throw new AssertionError(message);
    }
    public static void main(String[] args) {
        TouchLayout layout = new TouchLayout();
        check(layout.size() == 19, "All 18 buttons and the movement stick must be editable");
        try {
            java.util.Properties defaults = new java.util.Properties();
            try (java.io.Reader reader = java.nio.file.Files.newBufferedReader(java.nio.file.Path.of(args[0]))) { defaults.load(reader); }
            for (int i = 0; i < 18; i++) {
                String key = "control."+i+".";
                check(Math.abs(layout.x(i)-Float.parseFloat(defaults.getProperty(key+"x"))) < 0.001f &&
                      Math.abs(layout.y(i)-Float.parseFloat(defaults.getProperty(key+"y"))) < 0.001f &&
                      layout.sizeScale(i) == Float.parseFloat(defaults.getProperty(key+"size")) &&
                      layout.shown(i) == Boolean.parseBoolean(defaults.getProperty(key+"visible")),
                      "Default control must match supplied layout: "+i);
            }
        } catch (java.io.IOException e) { throw new AssertionError(e); }
        layout.move(TouchLayout.LEFT, -500, -500);
        check(layout.x(TouchLayout.LEFT) >= 64 && layout.y(TouchLayout.LEFT) == 64,
              "Dragging must keep the full stick inside the screen without a toolbar exclusion");
        layout.move(TouchLayout.FIRE_LEFT, 10000, 10000);
        check(layout.x(TouchLayout.FIRE_LEFT)+39 <= 960 && layout.y(TouchLayout.FIRE_LEFT)+39 <= 540,
              "Dragging beyond the bottom/right edge must keep the stick reachable");
        layout.move(0, 230, 320);
        layout.move(5, 440, 390);
        TouchLayout reopened = new TouchLayout();
        for (int i = 0; i < layout.size(); i++) reopened.restore(i, layout.x(i), layout.y(i));
        for (int i = 0; i < layout.size(); i++) {
            check(layout.x(i) == reopened.x(i) && layout.y(i) == reopened.y(i),
                  "Saved coordinates must round-trip for control "+i);
        }
        float before = reopened.x(0);
        reopened.restore(0, Float.NaN, 200);
        reopened.restore(0, Float.POSITIVE_INFINITY, 200);
        reopened.restore(0, -20, 200);
        check(reopened.x(0) == before, "Invalid saved coordinates must not corrupt the layout");
        reopened.move(0, Float.NaN, 200);
        check(reopened.x(0) == before, "Invalid drag coordinates must not corrupt the layout");
        // A saved top-row Pause button is valid even before it has been moved.
        check(reopened.y(10) == 36, "Reopening must preserve unmoved top-row controls");
        reopened.bounds(1200, 540);
        reopened.move(0, 9999, 1);
        check(reopened.x(0) == 1200-reopened.radius(0) && reopened.y(0) == reopened.radius(0),
              "Controls must reach the full widescreen edge and top edge");
        TouchLayout wide = new TouchLayout();
        wide.restore(0, reopened.savedX(0), reopened.savedY(0));
        wide.bounds(1200, 540);
        check(wide.x(0) == reopened.x(0), "Widescreen positions must survive reload");
        // Duplicates retain their action and are independently placed and hidden.
        int copy = wide.duplicate(4);
        check(copy == 19 && wide.type(copy) == 4, "Fire copies must retain the fire action");
        wide.setShown(4, false);
        wide.move(copy, 300, 100);
        check(!wide.shown(4) && wide.shown(copy), "Hiding an original must not hide its copy");
        check(wide.duplicate(TouchLayout.LEFT) == -1, "There can only be one movement stick");
        String exported = wide.exportConfiguration(2.25f);
        TouchLayout.Configuration imported = TouchLayout.importConfiguration(exported);
        imported.layout.bounds(1200, 540);
        check(imported.sensitivity == 2.25f && imported.layout.size() == 20,
              "Configuration must preserve sensitivity and duplicates");
        for (int i = 0; i < wide.size(); i++) {
            check(imported.layout.type(i) == wide.type(i) && imported.layout.shown(i) == wide.shown(i)
                && Math.abs(imported.layout.x(i)-wide.x(i)) < 0.001f
                && Math.abs(imported.layout.y(i)-wide.y(i)) < 0.001f,
                "Export/import must preserve each control on widescreen");
        }
        check(wide.add(4) == 4 && wide.shown(4), "Add must restore a hidden button first");
        wide.setShown(TouchLayout.LEFT, false);
        check(wide.add(TouchLayout.LEFT) == TouchLayout.LEFT, "Hidden move stick must be restorable");
        reject(exported.replace("version=4", "version=9"));
        reject(exported.replace("control.19.type=4", "control.19.type=16"));
        reject(exported.replaceAll("(?m)^control\\.19\\.x=.*$", "control.19.x=NaN"));
        reject(exported.replace("count=20", "count=10000"));
        reject(exported.replace("sensitivity=2.25", "sensitivity=Infinity"));
        reject(exported.replace("control.0.visible=false", "control.0.visible=maybe")
            .replace("control.0.visible=true", "control.0.visible=maybe"));
        reject("not a layout");
        wide.setSize(copy, 1.6f);
        wide.rumbleEnabled = false; wide.gyroscopeEnabled = true;
        TouchLayout.Configuration sized = TouchLayout.importConfiguration(wide.exportConfiguration(2.25f));
        check(sized.layout.sizeScale(copy) == 1.6f && !sized.layout.rumbleEnabled && sized.layout.gyroscopeEnabled,
              "Sizes, rumble and gyro settings must survive export/import");
        int sizedCopy = wide.duplicate(copy);
        check(wide.sizeScale(sizedCopy) == 1.6f, "Duplicates inherit their source size");
        wide.move(copy, 99999, 99999);
        wide.setSize(copy, 2f);
        check(wide.x(copy)+wide.radius(copy) <= 1200 && wide.y(copy)+wide.radius(copy) <= 540,
              "Growing a button at the edge must keep it reachable");
        String sizedText = sized.layout.exportConfiguration(2.25f);
        reject(sizedText.replace("control.19.size=1.6", "control.19.size=NaN"));
        reject(sizedText.replace("control.19.size=1.6", "control.19.size=3.0"));
        reject(sizedText.replace("rumble=false", "rumble=invalid"));
        reject(sizedText.replace("gyroscope=true", "gyroscope=invalid"));
        wide.gyroscopeSensitivity = 3.75f; wide.fpsCounter = true;
        String version3 = wide.exportConfiguration(2.25f);
        TouchLayout.Configuration hardware = TouchLayout.importConfiguration(version3);
        check(hardware.layout.gyroscopeSensitivity == 3.75f && hardware.layout.fpsCounter,
              "Version 3 must preserve independent gyro sensitivity and FPS preference");
        reject(version3.replace("gyroscope-sensitivity=3.75", "gyroscope-sensitivity=NaN"));
        reject(version3.replace("gyroscope-sensitivity=3.75", "gyroscope-sensitivity=4.01"));
        reject(version3.replace("fps-counter=true", "fps-counter=invalid"));
        TouchLayout.Configuration version2 = TouchLayout.importConfiguration(legacy(version3, 2)
            .replaceAll("(?m)^(gyroscope-sensitivity|fps-counter)=.*\\R", ""));
        check(version2.layout.gyroscopeEnabled && !version2.layout.rumbleEnabled &&
              version2.layout.gyroscopeSensitivity == 1 && !version2.layout.fpsCounter,
              "Version 2 must retain hardware toggles and default the new settings");
        String legacy = legacy(sizedText, 1).replaceAll("(?m)^.*\\.size=.*\\R", "")
            .replaceAll("(?m)^(rumble|gyroscope)=.*\\R", "");
        TouchLayout.Configuration old = TouchLayout.importConfiguration(legacy);
        check(old.layout.sizeScale(copy) == 1 && old.layout.rumbleEnabled && !old.layout.gyroscopeEnabled,
              "Legacy layouts must load with default sizes and gyro disabled");
        wide.resetDefaults();
        check(wide.size() == 19 && wide.shown(4) && Math.abs(wide.x(4)-808.50323f*1200f/960) < 0.001f,
              "Reset restores defaults on the current display and removes all copies");
        check(wide.sizeScale(4) == 1.3f && !wide.rumbleEnabled && wide.gyroscopeEnabled,
              "Button reset must restore sizes without changing General settings");
        while (wide.duplicate(4) >= 0) {}
        check(wide.size() == TouchLayout.MAX_CONTROLS, "Duplicate count must be bounded");
        wide.overlayDisabled = true; wide.fieldOfView = 85f;
        String general = wide.exportConfiguration(1f);
        TouchLayout.Configuration settings = TouchLayout.importConfiguration(general);
        check(settings.layout.overlayDisabled && settings.layout.fieldOfView == 85f, "Overlay and FOV must persist");
        reject(general.replace("field-of-view=85.0", "field-of-view=NaN"));
        reject(general.replace("field-of-view=85.0", "field-of-view=91.0"));
        reject(general.replace("overlay-disabled=true", "overlay-disabled=invalid"));
        settings = TouchLayout.importConfiguration(general.replaceAll("(?m)^(field-of-view|overlay-disabled)=.*\\R", ""));
        check(!settings.layout.overlayDisabled && settings.layout.fieldOfView == 70f, "Older exports must get safe defaults");
        TouchLayout full = new TouchLayout();
        while (full.size() < 65) check(full.duplicate(4) >= 0, "Full layout copies");
        TouchLayout migrated = TouchLayout.importConfiguration(legacy(full.exportConfiguration(1), 3)).layout;
        check(migrated.size() == 65 && migrated.type(18) == TouchLayout.CAMERA && migrated.type(64) == 4,
              "Full legacy layouts must keep every copy and gain Camera mode");
        check(TouchLayout.importConfiguration(migrated.exportConfiguration(1)).layout.size() == 65,
              "Full migrated layout must remain reloadable");
        check(migrated.duplicate(4) == -1, "Migration capacity must stay bounded");
        System.out.println("Touch layout, visibility, duplication and import/export checks passed");
    }

    private static String legacy(String text, int version) {
        try {
            java.util.Properties p = new java.util.Properties();
            p.load(new java.io.StringReader(text));
            int count = Integer.parseInt(p.getProperty("count"));
            for (int i = TouchLayout.CAMERA; i < count; i++) {
                for (String field : new String[]{"type", "x", "y", "visible", "size"}) {
                    String key = "control."+i+"."+field;
                    String next = p.getProperty("control."+(i+1)+"."+field);
                    if (next == null) p.remove(key); else p.setProperty(key, next);
                }
            }
            p.setProperty("count", Integer.toString(count-1));
            p.setProperty("version", Integer.toString(version));
            java.io.StringWriter out = new java.io.StringWriter(); p.store(out, "Legacy test");
            return out.toString();
        } catch(java.io.IOException e) { throw new AssertionError(e); }
    }

    private static void reject(String text) {
        try { TouchLayout.importConfiguration(text); }
        catch (IllegalArgumentException expected) { return; }
        throw new AssertionError("Invalid configuration was accepted");
    }
}
