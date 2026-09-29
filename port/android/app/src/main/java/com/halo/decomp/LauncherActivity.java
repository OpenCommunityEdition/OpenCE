package com.halo.decomp;

import android.app.Activity;
import android.content.Intent;
import android.graphics.Color;
import android.net.Uri;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.os.ParcelFileDescriptor;
import android.util.TypedValue;
import android.view.Gravity;
import android.view.View;
import android.widget.Button;
import android.widget.LinearLayout;
import android.widget.ProgressBar;
import android.widget.TextView;

import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.OutputStream;
import java.nio.channels.FileChannel;

/**
 * Starts the game once its data is in place.
 *
 * The game reads the Xbox game data (the folder holding maps/) from the
 * app's external files directory, /sdcard/Android/data/com.halo.decomp/files.
 * If it is missing, this screen lets the player pick an Xbox disc image of
 * the game (.xiso or .iso, any version) with the system file picker, and
 * copies its maps folder there (XisoExtractor), as the desktop games do; or
 * they can push the maps folder with adb.
 */
public class LauncherActivity extends Activity {
    private static final int PICK_IMAGE = 1;

    private File dataRoot;
    private TextView status;
    private ProgressBar progress;
    private Button pick;
    private final Handler handler = new Handler(Looper.getMainLooper());

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        dataRoot = getExternalFilesDir(null);
        // created by the app, so that files pushed into it with adb stay
        // readable (a directory adb creates there belongs to the shell user)
        if (dataRoot != null)
            new File(dataRoot, "maps").mkdirs();
        passOnInvite(getIntent());
        if (haveData()) {
            startGame();
            return;
        }
        buildInterface();
    }

    /**
     * An internet play invite link the app was opened with: the game
     * (port/linux/src/p2p.c) picks it up from join_link.txt, whether it is
     * starting now or already running.
     */
    private void passOnInvite(Intent intent) {
        if (intent == null || !Intent.ACTION_VIEW.equals(intent.getAction()) || intent.getData() == null
            || dataRoot == null)
            return;
        try (OutputStream out = new FileOutputStream(new File(dataRoot, "join_link.txt"))) {
            out.write(intent.getData().toString().getBytes("UTF-8"));
        } catch (java.io.IOException e) {
            // the link is lost; the player can copy it instead
        }
    }

    private boolean haveData() {
        return dataRoot != null && new File(dataRoot, "maps/ui.map").isFile();
    }

    private void startGame() {
        startActivity(new Intent(this, HaloActivity.class));
        finish();
    }

    private int dp(float value) {
        return (int) TypedValue.applyDimension(TypedValue.COMPLEX_UNIT_DIP, value,
            getResources().getDisplayMetrics());
    }

    private void buildInterface() {
        LinearLayout layout = new LinearLayout(this);
        layout.setOrientation(LinearLayout.VERTICAL);
        layout.setGravity(Gravity.CENTER);
        layout.setPadding(dp(48), dp(24), dp(48), dp(24));
        layout.setBackgroundColor(Color.rgb(12, 16, 20));

        TextView title = new TextView(this);
        title.setText("Halo needs its game data");
        title.setTextColor(Color.WHITE);
        title.setTextSize(TypedValue.COMPLEX_UNIT_SP, 24);
        title.setGravity(Gravity.CENTER);
        layout.addView(title);

        TextView message = new TextView(this);
        message.setText("Choose an Xbox disc image of Halo: Combat Evolved (an .iso or .xiso file, any "
            + "version) on this device. Its maps folder is copied into the app's storage (about 1.8 GB), "
            + "and you can delete the image afterwards.\n\n"
            + "You can also copy a maps folder from a computer:\n"
            + "adb push <folder with maps>/. " + (dataRoot != null ? dataRoot.getAbsolutePath() : "") + "/");
        message.setTextColor(Color.rgb(200, 205, 210));
        message.setTextSize(TypedValue.COMPLEX_UNIT_SP, 15);
        message.setGravity(Gravity.CENTER);
        message.setPadding(0, dp(16), 0, dp(16));
        layout.addView(message);

        pick = new Button(this);
        pick.setText("Choose disc image");
        pick.setOnClickListener(v -> {
            // (disc images have no MIME type of their own: any file, checked
            // when it is read)
            Intent intent = new Intent(Intent.ACTION_OPEN_DOCUMENT);
            intent.addCategory(Intent.CATEGORY_OPENABLE);
            intent.setType("*/*");
            startActivityForResult(intent, PICK_IMAGE);
        });
        layout.addView(pick, new LinearLayout.LayoutParams(LinearLayout.LayoutParams.WRAP_CONTENT,
            LinearLayout.LayoutParams.WRAP_CONTENT));

        progress = new ProgressBar(this, null, android.R.attr.progressBarStyleHorizontal);
        progress.setMax(1000);
        progress.setVisibility(View.GONE);
        LinearLayout.LayoutParams progressLayout = new LinearLayout.LayoutParams(dp(480),
            LinearLayout.LayoutParams.WRAP_CONTENT);
        progressLayout.topMargin = dp(16);
        layout.addView(progress, progressLayout);

        status = new TextView(this);
        status.setTextColor(Color.rgb(160, 200, 160));
        status.setGravity(Gravity.CENTER);
        status.setPadding(0, dp(8), 0, 0);
        layout.addView(status);

        setContentView(layout);
        pick.requestFocus();
    }

    @Override
    protected void onResume() {
        super.onResume();
        // data pushed with adb while this screen was open
        if (pick != null && pick.isEnabled() && haveData())
            startGame();
    }

    @Override
    protected void onActivityResult(int requestCode, int resultCode, Intent data) {
        super.onActivityResult(requestCode, resultCode, data);
        if (requestCode != PICK_IMAGE || resultCode != RESULT_OK || data == null || data.getData() == null)
            return;
        Uri image = data.getData();
        pick.setEnabled(false);
        progress.setVisibility(View.VISIBLE);
        status.setText("Reading the disc image...");
        new Thread(() -> importImage(image)).start();
    }

    private void report(String text, int permille) {
        handler.post(() -> {
            status.setText(text);
            if (permille >= 0)
                progress.setProgress(permille);
        });
    }

    private void fail(String text) {
        handler.post(() -> {
            status.setText(text);
            progress.setVisibility(View.GONE);
            pick.setEnabled(true);
            pick.requestFocus();
        });
    }

    private void importImage(Uri image) {
        android.util.Log.i("halo", "importing " + image);
        try (ParcelFileDescriptor descriptor = getContentResolver().openFileDescriptor(image, "r")) {
            if (descriptor == null)
                throw new java.io.IOException("the file could not be opened");
            try (FileInputStream in = new FileInputStream(descriptor.getFileDescriptor())) {
                FileChannel channel = in.getChannel();

                XisoExtractor.extractMaps(channel, dataRoot, (file, done, total) ->
                    report("Extracting maps/" + file + " (" + (done >> 20) + " of " + (total >> 20) + " MB)",
                        total > 0 ? (int) (done * 1000 / total) : 0));
            }
            handler.post(() -> {
                if (haveData()) {
                    startGame();
                } else {
                    fail("The extraction finished but maps/ui.map is missing.");
                }
            });
        } catch (XisoExtractor.ExtractException exception) {
            /* the whole stack, so that a report is enough to work it out */
            android.util.Log.e("halo", "the extraction failed", exception);
            fail(exception.getMessage());
        } catch (Throwable failure) {
            /* an Error (a failed allocation, say) would otherwise take the
            app down and leave nothing behind to look at */
            android.util.Log.e("halo", "the extraction failed", failure);
            String message = failure.getMessage();

            fail("Extracting failed: " + (message != null ? message : failure.getClass().getName()));
        }
    }
}
