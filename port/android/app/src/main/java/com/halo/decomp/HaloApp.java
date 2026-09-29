package com.halo.decomp;

/**
 * Claims the memory the game needs, as early as the app can.
 *
 * The game runs in a fixed 128 MB of address space, and the game data is
 * linked to the addresses inside it, so the range cannot move. Android's
 * Java runtime maps its own heaps in the same 4 GB, and it places them
 * where it finds room: the app that claims the range first gets it, and
 * the runtime works around it. This class is the app's first code to run
 * (the system creates the Application object before anything else in the
 * process), and loading the game's native library runs a constructor that
 * makes the claim. If the runtime got there first, the library tries again
 * when the game starts, and the log says what happened.
 */
public class HaloApp extends android.app.Application {
    static {
        System.loadLibrary("main");
    }
}
