package com.halo.decomp;

import android.app.Application;

/**
 * The game (HaloActivity) runs in a process of its own, ":game", so that
 * its Java runtime starts fresh for it: nothing the launcher, the disc
 * image import or an earlier game did is left in its heap.
 *
 * There the native library is loaded first thing, and reserves the fixed
 * addresses the game needs at 0x80000000 (port/android/host/host_main.c,
 * JNI_OnLoad) before the Java side has allocated anything large: on some
 * devices, handhelds with a large Java heap especially, ART's large object
 * space covers those addresses, and only an idle part of it can be taken
 * back (port/android/host/host_memory.c).
 */
public class HaloApplication extends Application {
    @Override
    public void onCreate() {
        super.onCreate();
        String process = Application.getProcessName();
        if (process == null || !process.endsWith(":game"))
            return;
        try {
            // in SDLActivity's order (HaloActivity.getLibraries); it
            // finds them loaded
            System.loadLibrary("SDL3");
            System.loadLibrary("main");
        } catch (UnsatisfiedLinkError e) {
            // SDLActivity reports it
        }
    }
}
