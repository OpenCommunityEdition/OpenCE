package com.halo.decomp;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;

import java.io.File;

/**
 * The app replaced by a new version (Updater): its download goes, and the
 * game is started again. (Android lets an app start itself from here only
 * in some cases; otherwise the package installer's Open button does.)
 */
public class UpdateReceiver extends BroadcastReceiver {
    @Override
    public void onReceive(Context context, Intent intent) {
        if (!Intent.ACTION_MY_PACKAGE_REPLACED.equals(intent.getAction()))
            return;
        new File(new File(context.getCacheDir(), UpdateProvider.DIRECTORY), UpdateProvider.APK).delete();
        try {
            Intent start = new Intent(context, LauncherActivity.class);

            start.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
            context.startActivity(start);
        } catch (RuntimeException e) {
            // not allowed from the background here: the installer's Open button starts it
        }
    }
}
