package com.halo.decomp;

import android.app.Activity;
import android.app.ActivityManager;
import android.app.AlertDialog;
import android.content.Intent;
import android.database.Cursor;
import android.net.Uri;
import android.os.Bundle;
import android.provider.DocumentsContract;
import android.widget.ArrayAdapter;
import android.widget.Button;
import android.widget.EditText;
import android.widget.LinearLayout;
import android.widget.Spinner;
import android.widget.TextView;

import java.io.File;
import java.io.IOException;
import java.io.InputStream;
import java.util.HashSet;
import java.util.List;
import java.util.Set;

/* A separate launcher shortcut keeps import I/O out of the running game. */
public final class MccImportActivity extends Activity {
    private static final int TREE = 71, ZIP = 72;
    private File root;
    private TextView status;
    private EditText name;
    private Spinner mode;
    private Button folder, archive;
    private Thread worker;
    private MccPackageImporter.Prepared pending;
    private volatile boolean destroyed;

    @Override protected void onCreate(Bundle state) {
        super.onCreate(state);
        root = getExternalFilesDir(null);
        LinearLayout panel = new LinearLayout(this);
        panel.setOrientation(LinearLayout.VERTICAL);
        int padding = (int)(24 * getResources().getDisplayMetrics().density);
        panel.setPadding(padding, padding, padding, padding);
        TextView heading = new TextView(this);
        heading.setText("Import MCC maps"); heading.setTextSize(24); panel.addView(heading);
        TextView help = new TextView(this);
        help.setText("Copy your complete Halo 1 Workshop package to this device, then choose its folder or ZIP. "
            + "Keep ModInfo.json, CampaignInfo.json and mission metadata. Steam is not needed.\n\n"
            + "For shared resources, choose a folder containing the singleplayer bitmaps.map, or the multiplayer "
            + "bitmaps.map and sounds.map from your MCC installation. Quit the game before importing.");
        panel.addView(help);
        mode = new Spinner(this);
        mode.setAdapter(new ArrayAdapter<>(this, android.R.layout.simple_spinner_dropdown_item,
            new String[]{"Workshop package", "Singleplayer shared resources", "Multiplayer shared resources"}));
        panel.addView(mode);
        name = new EditText(this); name.setSingleLine(true);
        name.setHint("Optional package folder name (default: Workshop ID)"); panel.addView(name);
        folder = new Button(this); folder.setText("Choose folder"); folder.setOnClickListener(v -> choose(TREE)); panel.addView(folder);
        archive = new Button(this); archive.setText("Choose ZIP"); archive.setOnClickListener(v -> choose(ZIP)); panel.addView(archive);
        status = new TextView(this); panel.addView(status);
        Button close = new Button(this); close.setText("Close"); close.setOnClickListener(v -> finish()); panel.addView(close);
        android.widget.ScrollView scroll = new android.widget.ScrollView(this); scroll.addView(panel); setContentView(scroll);
        if (root == null) { status.setText("App storage is unavailable."); busy(true); }
    }

    private boolean gameRunning() {
        ActivityManager manager = (ActivityManager)getSystemService(ACTIVITY_SERVICE);
        List<ActivityManager.RunningAppProcessInfo> processes = manager.getRunningAppProcesses();
        if (processes != null) for (ActivityManager.RunningAppProcessInfo process : processes)
            if (process.processName.equals(getPackageName() + ":game")) return true;
        return false;
    }
    private void busy(boolean value) {
        folder.setEnabled(!value); archive.setEnabled(!value); mode.setEnabled(!value); name.setEnabled(!value);
    }
    private void choose(int request) {
        if (gameRunning()) { status.setText("Quit the game before importing or replacing its maps."); return; }
        Intent intent = new Intent(request == TREE ? Intent.ACTION_OPEN_DOCUMENT_TREE : Intent.ACTION_OPEN_DOCUMENT);
        if (request == ZIP) { intent.addCategory(Intent.CATEGORY_OPENABLE); intent.setType("application/zip"); }
        intent.addFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION);
        try { startActivityForResult(intent, request); }
        catch (RuntimeException error) { status.setText("No document picker is available: " + error.getMessage()); }
    }

    private void copyTree(Uri tree, String document, String prefix, int depth, Set<String> visited,
                          MccPackageImporter.Receiver receiver) throws IOException {
        if (depth > 8 || visited.size() >= MccPackageImporter.MAX_FILES || !visited.add(document))
            throw new IOException("Document tree is too large, too deep or contains a cycle");
        Uri children = DocumentsContract.buildChildDocumentsUriUsingTree(tree, document);
        String[] columns = {DocumentsContract.Document.COLUMN_DOCUMENT_ID, DocumentsContract.Document.COLUMN_DISPLAY_NAME,
            DocumentsContract.Document.COLUMN_MIME_TYPE};
        try (Cursor cursor = getContentResolver().query(children, columns, null, null, null)) {
            if (cursor == null) throw new IOException("Cannot read the selected folder");
            while (cursor.moveToNext()) {
                if (Thread.currentThread().isInterrupted()) throw new IOException("Import cancelled");
                String id = cursor.getString(0), component = cursor.getString(1), mime = cursor.getString(2);
                if (component == null || component.contains("/") || component.contains("\\")) throw new IOException("Invalid document name");
                String relative = MccPackageImporter.path(prefix + component);
                if (DocumentsContract.Document.MIME_TYPE_DIR.equals(mime)) copyTree(tree, id, relative + "/", depth + 1, visited, receiver);
                else {
                    if (visited.size() >= MccPackageImporter.MAX_FILES || !visited.add(id)) throw new IOException("Duplicate document or too many files");
                    Uri file = DocumentsContract.buildDocumentUriUsingTree(tree, id);
                    try (InputStream input = getContentResolver().openInputStream(file)) {
                        if (input == null) throw new IOException("Cannot read " + component);
                        receiver.file(relative, input);
                    }
                }
            }
        } catch (SecurityException error) { throw new IOException("Folder permission was revoked", error); }
    }

    @Override protected void onActivityResult(int request, int result, Intent data) {
        super.onActivityResult(request, result, data);
        if ((request != TREE && request != ZIP) || result != RESULT_OK || data == null || data.getData() == null) return;
        if (worker != null || gameRunning()) { status.setText("Finish the current operation and quit the game before importing."); return; }
        Uri uri = data.getData();
        String packageName = name.getText().toString();
        int selectedMode = mode.getSelectedItemPosition();
        busy(true); status.setText("Checking and staging files…");
        worker = new Thread(() -> {
            try {
                MccPackageImporter.Source source;
                if (request == TREE) source = receiver -> copyTree(uri, DocumentsContract.getTreeDocumentId(uri), "", 0, new HashSet<>(), receiver);
                else source = receiver -> {
                    try (InputStream input = getContentResolver().openInputStream(uri)) {
                        if (input == null) throw new IOException("Cannot read the selected archive");
                        MccPackageImporter.zip(input).copy(receiver);
                    }
                };
                MccPackageImporter.Prepared prepared = MccPackageImporter.prepare(root, source, packageName, selectedMode);
                runOnUiThread(() -> {
                    worker = null;
                    if (destroyed || isFinishing()) { discard(prepared); return; }
                    pending = prepared;
                    boolean replace;
                    try { replace = prepared.exists(root); }
                    catch (IOException error) { pending = null; discard(prepared); failed(error); return; }
                    new AlertDialog.Builder(this).setTitle(replace ? "Replace installed package?" : "Install MCC content?")
                        .setMessage(prepared.title + "\n" + prepared.maps + " maps, " + (prepared.bytes / (1024 * 1024)) + " MiB\n"
                            + "Destination: mcc_maps/" + prepared.destination
                            + (replace ? "\n\nThis replaces the existing package contents. Game saves are preserved." : ""))
                        .setPositiveButton(replace ? "Replace" : "Install", (dialog, which) -> install(prepared, replace))
                        .setNegativeButton("Cancel", (dialog, which) -> { pending = null; discard(prepared); busy(false); })
                        .setOnCancelListener(dialog -> { pending = null; discard(prepared); busy(false); }).show();
                });
            } catch (IOException | RuntimeException error) { failed(error); }
        }, "MCC import");
        worker.start();
    }
    private void install(MccPackageImporter.Prepared prepared, boolean replace) {
        if (gameRunning()) { status.setText("Quit the game before installing maps."); pending = null; discard(prepared); busy(false); return; }
        pending = null;
        status.setText("Installing…");
        worker = new Thread(() -> {
            try (MccPackageImporter.Prepared cleanup = prepared) {
                prepared.install(root, replace);
                runOnUiThread(() -> {
                    worker = null;
                    if (destroyed) return;
                    busy(false);
                    status.setText("Installed in mcc_maps/" + prepared.destination + ". Start the game to refresh the map list. "
                        + "For mission ordering and progression, enable Campaign Missions in Settings > Online > Map Sharing > MCC Packages "
                        + "(Network Setup in other menu styles). "
                        + "Maps that use shared resources also need the matching resource set.");
                });
            } catch (IOException | RuntimeException error) { failed(error); }
        }, "MCC install");
        worker.start();
    }
    private void discard(MccPackageImporter.Prepared prepared) {
        new Thread(() -> { try { prepared.close(); } catch (IOException error) { android.util.Log.e("MCC import", "Staging cleanup failed", error); } }, "MCC cleanup").start();
    }
    private void failed(Exception error) {
        runOnUiThread(() -> { worker = null; if (!destroyed) { busy(false); status.setText("Import failed: " + error.getMessage()); } });
    }
    @Override protected void onDestroy() {
        destroyed = true;
        if (worker != null) worker.interrupt();
        if (pending != null) { discard(pending); pending = null; }
        super.onDestroy();
    }
}
