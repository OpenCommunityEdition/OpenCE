package com.halo.decomp;

import android.app.Activity;
import android.content.ContentResolver;
import android.content.Intent;
import android.database.Cursor;
import android.graphics.Color;
import android.net.Uri;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.provider.DocumentsContract;
import android.util.TypedValue;
import android.view.Gravity;
import android.view.View;
import android.widget.Button;
import android.widget.LinearLayout;
import android.widget.ProgressBar;
import android.widget.TextView;

import java.io.File;
import java.io.FileOutputStream;
import java.io.InputStream;
import java.io.OutputStream;
import java.util.ArrayList;
import java.util.List;

/**
 * Starts the game once its data is in place.
 *
 * The game reads the PAL Xbox data (the folder holding maps/) from the
 * app's external files directory, /sdcard/Android/data/com.halo.decomp/files.
 * If it is missing, this screen lets the player pick the folder with the
 * system file picker and copies it there (or they can push it with adb).
 */
public class LauncherActivity extends Activity {
    private static final int PICK_FOLDER = 1;

    private File dataRoot;
    private TextView status;
    private ProgressBar progress;
    private Button pick;
    private final Handler handler = new Handler(Looper.getMainLooper());

    private static final class Entry {
        final Uri uri;
        final String path;
        final long size;

        Entry(Uri uri, String path, long size) {
            this.uri = uri;
            this.path = path;
            this.size = size;
        }
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        dataRoot = getExternalFilesDir(null);
        // created by the app, so that files pushed into it with adb stay
        // readable (a directory adb creates there belongs to the shell user)
        if (dataRoot != null)
            new File(dataRoot, "maps").mkdirs();
        if (haveData()) {
            startGame();
            return;
        }
        buildInterface();
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
        message.setText("Choose the folder that contains the \"maps\" folder of the PAL game data "
            + "(build 01.01.14.2342). It is copied into the app's storage (about 1.8 GB).\n\n"
            + "You can also copy it from a computer:\n"
            + "adb push <folder>/. " + (dataRoot != null ? dataRoot.getAbsolutePath() : "") + "/");
        message.setTextColor(Color.rgb(200, 205, 210));
        message.setTextSize(TypedValue.COMPLEX_UNIT_SP, 15);
        message.setGravity(Gravity.CENTER);
        message.setPadding(0, dp(16), 0, dp(16));
        layout.addView(message);

        pick = new Button(this);
        pick.setText("Choose game data folder");
        pick.setOnClickListener(v -> {
            Intent intent = new Intent(Intent.ACTION_OPEN_DOCUMENT_TREE);
            startActivityForResult(intent, PICK_FOLDER);
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
        if (requestCode != PICK_FOLDER || resultCode != RESULT_OK || data == null || data.getData() == null)
            return;
        Uri tree = data.getData();
        pick.setEnabled(false);
        progress.setVisibility(View.VISIBLE);
        status.setText("Looking for the game data...");
        new Thread(() -> importData(tree)).start();
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

    /** the children of a document in the picked tree */
    private List<String[]> children(ContentResolver resolver, Uri tree, String documentId) {
        List<String[]> result = new ArrayList<>();
        Uri uri = DocumentsContract.buildChildDocumentsUriUsingTree(tree, documentId);
        String[] columns = {
            DocumentsContract.Document.COLUMN_DOCUMENT_ID,
            DocumentsContract.Document.COLUMN_DISPLAY_NAME,
            DocumentsContract.Document.COLUMN_MIME_TYPE,
            DocumentsContract.Document.COLUMN_SIZE,
        };
        try (Cursor cursor = resolver.query(uri, columns, null, null, null)) {
            while (cursor != null && cursor.moveToNext()) {
                result.add(new String[] {
                    cursor.getString(0), cursor.getString(1), cursor.getString(2),
                    cursor.isNull(3) ? "0" : cursor.getString(3),
                });
            }
        }
        return result;
    }

    private void collect(ContentResolver resolver, Uri tree, String documentId, String path, List<Entry> out) {
        for (String[] child : children(resolver, tree, documentId)) {
            String childPath = path.isEmpty() ? child[1] : path + "/" + child[1];
            if (DocumentsContract.Document.MIME_TYPE_DIR.equals(child[2]))
                collect(resolver, tree, child[0], childPath, out);
            else
                out.add(new Entry(DocumentsContract.buildDocumentUriUsingTree(tree, child[0]), childPath,
                    Long.parseLong(child[3])));
        }
    }

    private void importData(Uri tree) {
        try {
            ContentResolver resolver = getContentResolver();
            String rootId = DocumentsContract.getTreeDocumentId(tree);
            List<Entry> entries = new ArrayList<>();
            collect(resolver, tree, rootId, "", entries);

            // the picked folder holds maps/, or is maps/ itself
            boolean hasMapsFolder = false, isMapsFolder = false;
            for (Entry entry : entries) {
                if (entry.path.equalsIgnoreCase("maps/ui.map"))
                    hasMapsFolder = true;
                if (entry.path.equalsIgnoreCase("ui.map"))
                    isMapsFolder = true;
            }
            if (!hasMapsFolder && !isMapsFolder) {
                fail("That folder does not contain maps/ui.map. Pick the folder that holds \"maps\".");
                return;
            }
            long total = 0, done = 0;
            for (Entry entry : entries)
                total += entry.size;
            byte[] buffer = new byte[1 << 20];
            for (Entry entry : entries) {
                String path = isMapsFolder ? "maps/" + entry.path : entry.path;
                File destination = new File(dataRoot, path);
                File parent = destination.getParentFile();
                if (parent != null)
                    parent.mkdirs();
                File partial = new File(destination.getPath() + ".partial");
                try (InputStream in = resolver.openInputStream(entry.uri);
                     OutputStream out = new FileOutputStream(partial)) {
                    int count;
                    while ((count = in.read(buffer)) > 0) {
                        out.write(buffer, 0, count);
                        done += count;
                        report("Copying " + path + " (" + (done >> 20) + " of " + (total >> 20) + " MB)",
                            total > 0 ? (int) (done * 1000 / total) : 0);
                    }
                }
                if (!partial.renameTo(destination))
                    throw new java.io.IOException("cannot write " + destination);
            }
            handler.post(() -> {
                if (haveData()) {
                    startGame();
                } else {
                    fail("The copy finished but maps/ui.map is missing.");
                }
            });
        } catch (Exception exception) {
            fail("Copying failed: " + exception.getMessage());
        }
    }
}
