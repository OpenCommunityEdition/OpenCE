package com.halo.decomp;

import android.app.AlertDialog;
import android.content.Context;
import android.widget.Button;
import android.widget.LinearLayout;
import android.widget.ScrollView;
import android.widget.Switch;
import android.widget.Toast;
import java.util.function.Consumer;

/* Optional offline/host controls, inspired by thelinkin3000's Android overlay
 * and its credited antecedents, hardran3's OpenCE-Ultrahand and HaloDrones.
 * The engine rechecks permission for every request; UI state is advisory. */
final class AndroidCheatMenu {
    private static native int nativeState();
    private static native boolean nativeRequest(int request);

    static boolean enabled() {
        return (nativeState() & 1) != 0;
    }

    static boolean available() {
        return (nativeState() & 3) == 3;
    }

    private static boolean request(Context context, int command) {
        if (nativeRequest(command)) return true;
        Toast.makeText(context, "Cheats unavailable or queue busy", Toast.LENGTH_SHORT).show();
        return false;
    }

    static void show(Context context, Consumer<AlertDialog> openDialog) {
        int state = nativeState();
        if ((state & 2) == 0) {
            openDialog.accept(new AlertDialog.Builder(context).setTitle("Cheats")
                .setMessage("Available during gameplay when playing offline or hosting the game.")
                .setPositiveButton("Close", null).create());
            return;
        }
        LinearLayout panel = new LinearLayout(context);
        panel.setOrientation(LinearLayout.VERTICAL);
        int padding = (int)(16 * context.getResources().getDisplayMetrics().density);
        panel.setPadding(padding, padding, padding, padding);
        String[] switches = {"Invincibility", "Unlimited ammunition", "Bottomless clip", "Super jump",
            "Omnipotent", "No fall damage", "Medusa mode", "Bump possession", "Easy Rider"};
        for (int i = 0; i < switches.length; i++) {
            final int command = i;
            Switch toggle = new Switch(context);
            toggle.setText(switches[i]);
            toggle.setChecked((state & (1 << (8 + i))) != 0);
            toggle.setOnClickListener(view -> {
                if (!request(context, command | (toggle.isChecked() ? 0x100 : 0)))
                    toggle.setChecked(!toggle.isChecked());
            });
            panel.addView(toggle);
        }
        String[] actions = {"Spawn weapons", "Spawn powerups", "Active camouflage", "Spawn vehicles"};
        for (int i = 0; i < actions.length; i++) {
            final int command = switches.length + i;
            Button button = new Button(context);
            button.setText(actions[i]);
            button.setOnClickListener(view -> request(context, command));
            panel.addView(button);
        }
        ScrollView scroll = new ScrollView(context);
        scroll.addView(panel);
        openDialog.accept(new AlertDialog.Builder(context).setTitle("Cheats")
            .setView(scroll).setNegativeButton("Close", null).create());
    }
}
