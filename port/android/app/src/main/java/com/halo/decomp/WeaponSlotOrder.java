package com.halo.decomp;

/**
 * Display-only ordering of Halo's four physical weapon inventory slots.
 * No weapon is created, removed, or moved inside the game engine.
 * This class has no Android dependencies, so its rules can be tested directly.
 */
final class WeaponSlotOrder {
    private WeaponSlotOrder() {}

    private static boolean occupied(int mask, int physicalSlot) {
        return (mask & (1 << physicalSlot)) != 0;
    }

    /**
     * On pickup/drop/respawn, promote existing weapons into the quick slots.
     * Preserve the relative display order of remaining weapons and put empty
     * physical slots at the end. A quick slot will never be left empty when
     * two or more actual weapons exist.
     */
    static void compact(int[] order, int mask) {
        int[] next = new int[4];
        int count = 0;
        for (int i = 0; i < 4; i++)
            if (occupied(mask, order[i])) next[count++] = order[i];
        for (int i = 0; i < 4; i++)
            if (!occupied(mask, order[i])) next[count++] = order[i];
        System.arraycopy(next, 0, order, 0, 4);
    }

    /**
     * Move an existing gun's visual card to another frame. Return the moved
     * physical slot, or -1 when there is no possible move.
     *
     * Swapping two occupied cards is straightforward. If a quick-slot gun is
     * dragged into an empty drawer cell, pull another *real* drawer weapon into
     * the vacated quick slot. If there is no third weapon, reject the move:
     * leaving a quick slot empty or creating a fake copy would be misleading.
     */
    static boolean canMove(int[] order, int mask, int from, int to) {
        if (from < 0 || from >= 4 || to < 0 || to >= 4 || from == to
                || !occupied(mask, order[from])) return false;
        if (occupied(mask, order[to]) || from >= 2) return true;
        if (to < 2) return false;
        for (int i = 2; i < 4; i++)
            if (i != to && occupied(mask, order[i])) return true;
        return false;
    }

    static int move(int[] order, int mask, int from, int to) {
        if (!canMove(order, mask, from, to)) return -1;
        int moved = order[from];
        int displaced = order[to];
        if (!occupied(mask, displaced) && from < 2) {
            if (to < 2) return -1; // Keep the first quick slot(s) populated.
            int replacement = -1;
            for (int i = 2; i < 4; i++) {
                if (i != to && occupied(mask, order[i])) {
                    replacement = i;
                    break;
                }
            }
            if (replacement < 0) return -1; // Only 1-2 real weapons.
            order[from] = order[replacement];
            order[replacement] = displaced;
            order[to] = moved;
        } else {
            order[from] = displaced;
            order[to] = moved;
        }
        return moved;
    }
}
