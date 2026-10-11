package com.halo.decomp;

import android.app.Activity;
import android.content.Intent;
import android.graphics.Color;
import android.net.Uri;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.os.ParcelFileDescriptor;
import android.provider.Settings;
import android.provider.DocumentsContract;
import android.database.Cursor;
import java.io.InputStream;
import java.io.IOException;
import java.util.ArrayList;
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
    private static final int PICK_MAPS = 2;

    private File dataRoot;
    private TextView status;
    private ProgressBar progress;
    private Button pick;
    private Button pickMaps;
    private final Handler handler = new Handler(Looper.getMainLooper());

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        dataRoot = getExternalFilesDir(null);
        // created by the app, so that files pushed into it with adb stay
        // readable (a directory adb creates there belongs to the shell user)
        if (dataRoot != null)
            new File(dataRoot, "maps").mkdirs();
        passOnHardwareId();
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
        // written whole under another name, then renamed: the game never
        // reads it half written
        File partial = new File(dataRoot, "join_link.txt.tmp");
        try (OutputStream out = new FileOutputStream(partial)) {
            out.write(intent.getData().toString().getBytes("UTF-8"));
        } catch (java.io.IOException e) {
            // the link is lost; the player can copy it instead
            partial.delete();
            return;
        }
        if (!partial.renameTo(new File(dataRoot, "join_link.txt")))
            partial.delete();
    }

    /**
     * This device's ANDROID_ID (the app's own: one per app signing key and
     * user, until a factory reset), which native code cannot read: the game
     * (port/linux/src/p2p.c) hashes it from hardware_id.txt into the
     * hardware id a host it joins is told.
     */
    private void passOnHardwareId() {
        String id;

        if (dataRoot == null)
            return;
        try {
            id = Settings.Secure.getString(getContentResolver(), Settings.Secure.ANDROID_ID);
        } catch (RuntimeException e) {
            return;
        }
        if (id == null || id.isEmpty())
            return;
        File partial = new File(dataRoot, "hardware_id.txt.tmp");
        try (OutputStream out = new FileOutputStream(partial)) {
            out.write(id.getBytes("UTF-8"));
        } catch (java.io.IOException e) {
            partial.delete();
            return;
        }
        if (!partial.renameTo(new File(dataRoot, "hardware_id.txt")))
            partial.delete();
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
            + "Or choose a folder containing extracted maps (ui.map and the other .map files), "
            + "or a folder with a maps subfolder.\n\n"
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

        pickMaps = new Button(this);
        pickMaps.setText("Choose extracted maps");
        pickMaps.setOnClickListener(v -> {
            Intent intent = new Intent(Intent.ACTION_OPEN_DOCUMENT_TREE);
            intent.addFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION);
            startActivityForResult(intent, PICK_MAPS);
        });
        layout.addView(pickMaps, new LinearLayout.LayoutParams(LinearLayout.LayoutParams.WRAP_CONTENT,
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
        if ((requestCode != PICK_IMAGE && requestCode != PICK_MAPS) || resultCode != RESULT_OK || data == null || data.getData() == null)
            return;
        Uri image = data.getData();
        pick.setEnabled(false);
        pickMaps.setEnabled(false);
        progress.setVisibility(View.VISIBLE);
        if (requestCode == PICK_MAPS) {
            progress.setIndeterminate(true);
            status.setText("Reading extracted maps...");
            new Thread(() -> importMaps(image), "Halo maps import").start();
        } else {
            progress.setIndeterminate(false);
            status.setText("Reading the disc image...");
            new Thread(() -> importImage(image), "Halo disc import").start();
        }
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
            pickMaps.setEnabled(true);
            pick.requestFocus();
        });
    }

    private static final class MapDocument {
        final Uri uri;
        final String name;
        final boolean directory;
        MapDocument(Uri uri, String name, boolean directory) {
            this.uri = uri; this.name = name; this.directory = directory;
        }
    }

    private ArrayList<MapDocument> children(Uri directory) throws IOException {
        Uri query = DocumentsContract.buildChildDocumentsUriUsingTree(directory,
            DocumentsContract.getDocumentId(directory));
        String[] columns = {DocumentsContract.Document.COLUMN_DOCUMENT_ID,
            DocumentsContract.Document.COLUMN_DISPLAY_NAME, DocumentsContract.Document.COLUMN_MIME_TYPE};
        ArrayList<MapDocument> result = new ArrayList<>();
        try (Cursor cursor = getContentResolver().query(query, columns, null, null, null)) {
            if (cursor == null) throw new IOException("The selected folder cannot be read");
            while (cursor.moveToNext()) {
                String name = cursor.getString(1);
                if (!validDocumentName(name))
                    throw new IOException("Invalid filename in the selected folder");
                result.add(new MapDocument(DocumentsContract.buildDocumentUriUsingTree(directory,
                    cursor.getString(0)), name, DocumentsContract.Document.MIME_TYPE_DIR.equals(cursor.getString(2))));
            }
        }
        return result;
    }

    private boolean validDocumentName(String name) {
        return name != null && !name.isEmpty() && !name.equals(".") && !name.equals("..") &&
            name.indexOf('/') < 0 && name.indexOf('\\') < 0;
    }

    private boolean hasUiMap(ArrayList<MapDocument> entries) {
        for (MapDocument entry : entries)
            if (!entry.directory && entry.name.equalsIgnoreCase("ui.map")) return true;
        return false;
    }

    private void copyMaps(Uri source, File destination, int depth) throws IOException {
        if (depth > 32) throw new IOException("Too many nested folders");
        if (!destination.mkdir()) throw new IOException("Cannot create the maps folder");
        byte[] buffer = new byte[256 * 1024];
        for (MapDocument entry : children(source)) {
            if (!validDocumentName(entry.name)) throw new IOException("Invalid filename in the selected folder");
            // Halo's map filenames are lowercase, even when a disc extraction used uppercase.
            String name = entry.name.toLowerCase(java.util.Locale.ROOT);
            File target = new File(destination, name);
            if (target.exists()) throw new IOException("Duplicate filename: " + entry.name);
            if (entry.directory) {
                copyMaps(entry.uri, target, depth + 1);
            } else {
                report("Copying " + entry.name + "...", -1);
                try (InputStream input = getContentResolver().openInputStream(entry.uri)) {
                    if (input == null) throw new IOException("Cannot read " + entry.name);
                    try (OutputStream output = new FileOutputStream(target)) {
                        int count;
                        while ((count = input.read(buffer)) != -1) output.write(buffer, 0, count);
                    }
                }
            }
        }
    }

    // Only called for app-owned temporary directories created by this import.
    private void removeImport(File file) {
        File[] contents = file.listFiles();
        if (contents != null) for (File child : contents) removeImport(child);
        file.delete();
    }

    private void importMaps(Uri tree) {
        File staging = null;
        try {
            if (dataRoot == null) throw new IOException("App storage is unavailable");
            Uri source = DocumentsContract.buildDocumentUriUsingTree(tree,
                DocumentsContract.getTreeDocumentId(tree));
            ArrayList<MapDocument> entries = children(source);
            if (!hasUiMap(entries)) {
                for (MapDocument entry : entries) {
                    if (entry.directory && entry.name.equalsIgnoreCase("maps")) {
                        source = entry.uri; entries = children(source); break;
                    }
                }
            }
            if (!hasUiMap(entries)) throw new IOException("Choose a maps folder containing ui.map, or its parent folder");
            staging = new File(dataRoot, "maps-import-" + System.nanoTime());
            copyMaps(source, staging, 0);
            if (new File(staging, "ui.map").length() == 0) throw new IOException("ui.map is empty");
            File destination = new File(dataRoot, "maps");
            File previous = new File(dataRoot, "maps-before-import-" + System.nanoTime());
            boolean backedUp = destination.exists();
            if (backedUp && !destination.renameTo(previous)) throw new IOException("Cannot prepare app storage");
            if (!staging.renameTo(destination)) {
                if (backedUp && !previous.renameTo(destination))
                    throw new IOException("Cannot install maps; previous files are preserved in " + previous.getName());
                throw new IOException("Cannot install copied maps");
            }
            staging = null;
            if (backedUp) removeImport(previous);
            handler.post(this::startGame);
        } catch (Exception exception) {
            fail("Import failed: " + exception.getMessage());
        } finally {
            if (staging != null) removeImport(staging);
        }
    }

    private void importImage(Uri image) {
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
            fail(exception.getMessage());
        } catch (Exception exception) {
            fail("Extracting failed: " + exception.getMessage());
        }
    }
}
