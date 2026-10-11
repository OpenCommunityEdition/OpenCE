package com.halo.decomp;

/** Integrates angular velocity in display coordinates; output is in radians. */
final class GyroscopeAim {
    private long previousTime;
    private int previousRotation = -1;
    private float previousX, previousY;

    void reset() { previousTime = 0; previousRotation = -1; }

    boolean sample(long time, float x, float y, int rotation, float[] delta) {
        delta[0] = delta[1] = 0;
        if (!Float.isFinite(x) || !Float.isFinite(y) || rotation < 0 || rotation > 3) {
            reset(); return false;
        }
        float screenX = x, screenY = y;
        switch (rotation) {
            case 1: screenX = y; screenY = -x; break;
            case 2: screenX = -x; screenY = -y; break;
            case 3: screenX = -y; screenY = x; break;
        }
        // Small stationary sensor noise must not gradually drift the camera.
        if (Math.abs(screenX) < 0.01f) screenX = 0;
        if (Math.abs(screenY) < 0.01f) screenY = 0;
        float dt = (time-previousTime)*1e-9f;
        boolean valid = previousTime != 0 && rotation == previousRotation && dt > 0 && dt <= 0.1f;
        if (valid) {
            delta[0] = -(screenY+previousY)*0.5f*dt;
            delta[1] = -(screenX+previousX)*0.5f*dt;
        }
        previousTime = time; previousRotation = rotation;
        previousX = screenX; previousY = screenY;
        return valid && (delta[0] != 0 || delta[1] != 0);
    }
}
