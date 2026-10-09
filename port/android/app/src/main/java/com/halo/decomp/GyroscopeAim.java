package com.halo.decomp;

/**
 * Gyroscope aiming (TouchControls): integrates the sensor's angular velocity,
 * turned into the display's coordinates for its rotation, into radians of
 * view turn between samples.
 */
final class GyroscopeAim {
    /** below this (radians a second) a reading is the sensor's noise at rest, which must not drift the view */
    private static final float NOISE = 0.01f;
    /** a gap longer than this (seconds) starts over rather than turn at once */
    private static final float LONGEST_GAP = 0.1f;

    private long previousTime;
    private int previousRotation = -1;
    private float previousX;
    private float previousY;

    void reset() {
        previousTime = 0;
        previousRotation = -1;
    }

    /**
     * takes one sample (its time in nanoseconds, the angular velocity about
     * the device's x and y axes, the display's rotation 0..3) and puts the turn
     * since the last into delta {yaw, pitch}; false while there is none
     */
    boolean sample(long time, float x, float y, int rotation, float[] delta) {
        delta[0] = 0;
        delta[1] = 0;
        if (!Float.isFinite(x) || !Float.isFinite(y) || rotation < 0 || rotation > 3) {
            reset();
            return false;
        }
        float screenX = x;
        float screenY = y;
        switch (rotation) {
            case 1:
                screenX = y;
                screenY = -x;
                break;
            case 2:
                screenX = -x;
                screenY = -y;
                break;
            case 3:
                screenX = -y;
                screenY = x;
                break;
            default:
                break;
        }
        if (Math.abs(screenX) < NOISE)
            screenX = 0;
        if (Math.abs(screenY) < NOISE)
            screenY = 0;
        float seconds = (time - previousTime) * 1e-9f;
        boolean valid = previousTime != 0 && rotation == previousRotation && seconds > 0 && seconds <= LONGEST_GAP;
        if (valid) {
            // the trapezoid rule over the two samples
            delta[0] = -(screenY + previousY) * 0.5f * seconds;
            delta[1] = -(screenX + previousX) * 0.5f * seconds;
        }
        previousTime = time;
        previousRotation = rotation;
        previousX = screenX;
        previousY = screenY;
        return valid && (delta[0] != 0 || delta[1] != 0);
    }
}
