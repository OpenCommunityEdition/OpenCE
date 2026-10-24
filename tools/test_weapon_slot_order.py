#!/usr/bin/env python3
"""Test OpenCE HUD slot order without an Android device or Gradle."""
from pathlib import Path
import subprocess
import tempfile

root = Path(__file__).resolve().parents[1]
java = root / 'port/android/app/src/main/java/com/halo/decomp/WeaponSlotOrder.java'

harness = '''package com.halo.decomp;
import java.util.Arrays;
public final class WeaponSlotOrderTest {
    static void equal(String caseName, int[] actual, int... expected) {
        if (!Arrays.equals(actual, expected))
            throw new AssertionError(caseName + ": " + Arrays.toString(actual)
                + " expected " + Arrays.toString(expected));
    }
    static void expect(String caseName, int actual, int expected) {
        if (actual != expected)
            throw new AssertionError(caseName + ": " + actual + " expected " + expected);
    }
    public static void main(String[] args) {
        int[] order = {0,1,2,3};
        expect("two weapons cannot vacate primary", WeaponSlotOrder.move(order,3,0,2),-1);
        equal("no duplicate",order,0,1,2,3);

        order = new int[]{0,1,2,3};
        expect("3 weapons can fill vacated primary",WeaponSlotOrder.move(order,7,0,3),0);
        equal("drawer replaces primary with real gun",order,2,1,3,0);

        order = new int[]{0,1,2,3};
        expect("swap to occupied main",WeaponSlotOrder.move(order,15,2,0),2);
        equal("quick slot receives gun",order,2,1,0,3);

        order = new int[]{0,1,2,3};
        expect("swap main cards",WeaponSlotOrder.move(order,3,0,1),0);
        equal("main cards swap",order,1,0,2,3);

        order = new int[]{0,1,2,3};
        expect("move drawer card into empty drawer",WeaponSlotOrder.move(order,7,2,3),2);
        equal("drawer positions work",order,0,1,3,2);

        order = new int[]{0,1,2,3};
        WeaponSlotOrder.compact(order,5);
        equal("non-contiguous physical inventory fills mains",order,0,2,1,3);

        order = new int[]{2,1,3,0};
        WeaponSlotOrder.compact(order,3);
        equal("drop gun promotes remaining",order,1,0,2,3);

        order = new int[]{0,1,2,3};
        WeaponSlotOrder.compact(order,8);
        equal("one weapon stays on main",order,3,0,1,2);
        expect("cannot empty sole main",WeaponSlotOrder.move(order,8,0,1),-1);
        equal("one weapon stays visible",order,3,0,1,2);
        // Exhaustively verify all 16 inventories, for all drags. The UI
        // mapping must remain a permutation, never manufacture a new gun,
        // and retain filled quick slots when two actual guns exist.
        for (int mask = 0; mask < 16; mask++) {
            for (int source = 0; source < 4; source++) {
                for (int dest = 0; dest < 4; dest++) {
                    order = new int[]{0,1,2,3};
                    WeaponSlotOrder.compact(order, mask);
                    WeaponSlotOrder.move(order, mask, source, dest);
                    int seen = 0;
                    for (int slot : order) {
                        if (slot < 0 || slot >= 4 || (seen & (1 << slot)) != 0)
                            throw new AssertionError("duplicate/invalid physical slot");
                        seen |= (1 << slot);
                    }
                    if (seen != 15) throw new AssertionError("slot missing");
                    if (Integer.bitCount(mask) >= 2) {
                        if ((mask & (1 << order[0])) == 0 ||
                            (mask & (1 << order[1])) == 0)
                            throw new AssertionError("quick slot emptied by a drag");
                    }
                }
            }
        }
        System.out.println("Weapon slot ordering: 8 cases + 256 invariant checks passed");
    }
}
'''
with tempfile.TemporaryDirectory() as tmp:
    tmp = Path(tmp)
    (tmp / 'WeaponSlotOrderTest.java').write_text(harness)
    subprocess.run(['javac','-d',str(tmp),str(java),str(tmp/'WeaponSlotOrderTest.java')],check=True)
    subprocess.run(['java','-cp',str(tmp),'com.halo.decomp.WeaponSlotOrderTest'],check=True)
