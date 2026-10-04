package com.halo.decomp;

public final class GyroscopeAimTest {
    private static void check(boolean ok, String message) {
        if (!ok) throw new AssertionError(message);
    }
    private static void near(float value, float expected, String message) {
        check(Math.abs(value-expected) < 0.000001f, message);
    }
    public static void main(String[] args) {
        GyroscopeAim aim = new GyroscopeAim(); float[] delta = new float[2];
        check(!aim.sample(1000000000L, 1, 0, 1, delta), "First event must only anchor time");
        check(aim.sample(1020000000L, 1, 0, 1, delta), "Landscape yaw must produce motion");
        near(delta[0], 0.02f, "Yaw integrates angular velocity in radians");
        near(delta[1], 0, "Pure yaw does not pitch");
        check(!aim.sample(1040000000L, 1, 0, 3, delta), "Changing orientation must reset the sample");
        aim.sample(1060000000L, 1, 0, 3, delta);
        near(delta[0], -0.02f, "Reverse landscape must reverse device-axis mapping");
        check(!aim.sample(2000000000L, 1, 0, 3, delta), "A resume gap must not jump the camera");
        aim.reset();
        check(!aim.sample(2100000000L, 0, 1, 1, delta), "Reset releases pending gyro motion");
        aim.sample(2120000000L, 0, 1, 1, delta);
        near(delta[1], -0.02f, "Landscape pitch must follow phone tilt rather than invert it");
        aim.reset(); aim.sample(2200000000L, 0, 1, 3, delta);
        aim.sample(2220000000L, 0, 1, 3, delta);
        near(delta[1], 0.02f, "Reverse landscape pitch must follow the reversed display axis");
        aim.reset(); aim.sample(3000000000L, 0.005f, -0.005f, 0, delta);
        check(!aim.sample(3020000000L, 0.005f, -0.005f, 0, delta), "Small stationary noise must not drift");
        check(!aim.sample(3040000000L, Float.NaN, 1, 0, delta), "Invalid sensor values are rejected");
        aim.sample(4000000000L, 1, 1, 0, delta);
        check(!aim.sample(3990000000L, 1, 1, 0, delta), "Out-of-order timestamps produce no motion");
        System.out.println("Gyroscope integration, orientation and lifecycle checks passed");
    }
}
