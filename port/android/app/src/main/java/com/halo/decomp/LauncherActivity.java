package com.halo.decomp;

import android.Manifest;
import android.app.Activity;
import android.content.ContentResolver;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.database.Cursor;
import android.net.Uri;
import android.os.Build;
import android.os.Bundle;
import android.os.Environment;
import android.os.Handler;
import android.os.Looper;
import android.os.ParcelFileDescriptor;
import android.provider.DocumentsContract;
import android.provider.OpenableColumns;
import android.provider.Settings;
import android.util.TypedValue;
import android.view.Gravity;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Button;
import android.widget.FrameLayout;
import android.widget.LinearLayout;
import android.widget.ScrollView;
import android.widget.TextView;

import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.nio.channels.FileChannel;
import java.util.ArrayList;
import java.util.LinkedHashSet;
import java.util.List;
import java.util.Locale;
import java.util.Set;

/**
 * Starts the game once its data is in place.
 *
 * The game reads the Xbox game data (the folder holding maps/) from the
 * app's external files directory, /sdcard/Android/data/com.halo.decomp/files.
 * If it is missing, this screen finds or accepts an Xbox disc image and
 * extracts maps/ out of it on the device, or copies a maps/ folder the
 * player picks.
 *
 * The screen is an ONI archive terminal: a Halo ring in the dark, holo-cyan
 * readouts and a data-recovery progress bar.
 */
public class LauncherActivity extends Activity {
    private static final int PICK_FOLDER = 1;
    private static final int PICK_IMAGE = 2;
    private static final int REQUEST_STORAGE = 3;

    private static final int COLOR_ACTIVE = HaloUi.CYAN;

    private File dataRoot;
    private TextView status;
    private TextView readout;
    private TextView percent;
    private TextView seal;
    private HaloProgressView progress;
    private LinearLayout buttons;

    private final Handler handler = new Handler(Looper.getMainLooper());
    private final List<String> lines = new ArrayList<>();
    private final List<File> foundImages = new ArrayList<>();
    private boolean archiveAccess;
    private volatile boolean extracting;
    private volatile boolean scanning;
    private volatile boolean cancelled;
    private boolean autoExtractPending = true;
    private int lastLoggedPercent = -1;

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
        passOnInvite(getIntent());
        buildInterface();
        if (haveData()) {
            startGame();
            return;
        }
        Uri target = imageFromIntent(getIntent());
        if (target != null) {
            log("> archive image handed to the terminal");
            beginExtract(target, displayName(target));
        } else {
            scanForImages();
        }
    }

    @Override
    protected void onResume() {
        super.onResume();
        // data pushed with adb, or permission granted in Settings
        if (haveData()) {
            startGame();
            return;
        }
        if (!extracting && buttons != null)
            scanForImages();
    }

    @Override
    protected void onNewIntent(Intent intent) {
        super.onNewIntent(intent);
        setIntent(intent);
        passOnInvite(intent);
        Uri target = imageFromIntent(intent);
        if (target != null && !extracting)
            beginExtract(target, displayName(target));
    }

    /** an Xbox disc image opened or shared with the app */
    private Uri imageFromIntent(Intent intent) {
        if (intent == null)
            return null;
        String action = intent.getAction();
        Uri data = intent.getData();
        if (Intent.ACTION_SEND.equals(action)) {
            android.os.Parcelable shared = intent.getParcelableExtra(Intent.EXTRA_STREAM);
            if (shared instanceof Uri)
                data = (Uri) shared;
        } else if (!Intent.ACTION_VIEW.equals(action)) {
            return null;
        }
        if (data == null || "halo".equals(data.getScheme()))
            return null;
        return data;
    }

    /**
     * An internet play invite link the app was opened with: the game
     * (port/linux/src/p2p.c) picks it up from join_link.txt, whether it is
     * starting now or already running.
     */
    private void passOnInvite(Intent intent) {
        if (intent == null || !Intent.ACTION_VIEW.equals(intent.getAction()) || intent.getData() == null
            || dataRoot == null || !"halo".equals(intent.getData().getScheme()))
            return;
        try (OutputStream out = new FileOutputStream(new File(dataRoot, "join_link.txt"))) {
            out.write(intent.getData().toString().getBytes("UTF-8"));
        } catch (IOException e) {
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

    // ---------------------------------------------------------------- the screen

    private void buildInterface() {
        FrameLayout root = new FrameLayout(this);
        root.addView(new HaloBackgroundView(this),
            new FrameLayout.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.MATCH_PARENT));

        ScrollView scroll = new ScrollView(this);
        scroll.setFillViewport(true);
        scroll.setClipToPadding(false);
        root.addView(scroll, new FrameLayout.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT,
            ViewGroup.LayoutParams.MATCH_PARENT));

        LinearLayout column = new LinearLayout(this);
        column.setOrientation(LinearLayout.VERTICAL);
        column.setGravity(Gravity.CENTER_HORIZONTAL);
        int pad = HaloUi.dp(this, 40);
        column.setPadding(pad, HaloUi.dp(this, 30), pad, HaloUi.dp(this, 30));
        scroll.addView(column, new ScrollView.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT,
            ViewGroup.LayoutParams.WRAP_CONTENT));

        TextView classification = HaloUi.text(this, "UNSC // ONI SECTION III   ·   EYES ONLY", HaloUi.AMBER, 11,
            HaloUi.MONO);
        classification.setGravity(Gravity.CENTER);
        column.addView(classification);
        TextView title = HaloUi.heading(this, "Halo: Combat Evolved", HaloUi.TEXT, 30);
        title.setLetterSpacing(0.3f);
        title.setGravity(Gravity.CENTER);
        column.addView(title, margins(0, 6, 0, 0));
        TextView subtitle = HaloUi.text(this, "INSTALLATION 04   ·   ARCHIVE RECOVERY PROTOCOL", HaloUi.CYAN_DIM,
            12, HaloUi.MONO);
        subtitle.setGravity(Gravity.CENTER);
        column.addView(subtitle, margins(0, 2, 0, 0));

        View panel = buildPanel();
        panel.setBackground(HaloUi.panel(this));
        int panelPad = HaloUi.dp(this, 22);
        panel.setPadding(panelPad, HaloUi.dp(this, 18), panelPad, HaloUi.dp(this, 18));
        column.addView(panel, margins(0, 22, 0, 0));

        buttons = new LinearLayout(this);
        buttons.setOrientation(LinearLayout.VERTICAL);
        column.addView(buttons, margins(0, 20, 0, 0));

        TextView footer = HaloUi.text(this, "CORTANA   //   DATA RECOVERY   //   UNSC INFINITY", HaloUi.CYAN_DIM, 11,
            HaloUi.MONO);
        footer.setGravity(Gravity.CENTER);
        column.addView(footer, margins(0, 24, 0, 0));

        setContentView(root);
    }

    private View buildPanel() {
        LinearLayout content = new LinearLayout(this);
        content.setOrientation(LinearLayout.VERTICAL);

        LinearLayout header = new LinearLayout(this);
        header.setOrientation(LinearLayout.HORIZONTAL);
        header.setGravity(Gravity.CENTER_VERTICAL);
        HaloGlyphView glyph = new HaloGlyphView(this);
        header.addView(glyph, new LinearLayout.LayoutParams(HaloUi.dp(this, 40), HaloUi.dp(this, 40)));
        status = HaloUi.heading(this, "Initializing", HaloUi.CYAN, 16);
        LinearLayout.LayoutParams statusParams = new LinearLayout.LayoutParams(0,
            ViewGroup.LayoutParams.WRAP_CONTENT, 1f);
        statusParams.leftMargin = HaloUi.dp(this, 14);
        header.addView(status, statusParams);
        seal = HaloUi.text(this, "", HaloUi.AMBER, 11, HaloUi.MONO);
        header.addView(seal);
        content.addView(header);

        readout = HaloUi.readout(this, "", HaloUi.TEXT_DIM, 12.5f);
        readout.setMaxLines(6);
        content.addView(readout, margins(0, 12, 0, 0));

        progress = new HaloProgressView(this);
        content.addView(progress, margins(0, 16, 0, 0));
        progress.setVisibility(View.GONE);

        percent = HaloUi.text(this, "", COLOR_ACTIVE, 12, HaloUi.MONO);
        percent.setGravity(Gravity.END);
        content.addView(percent, margins(0, 6, 0, 0));
        percent.setVisibility(View.GONE);

        return content;
    }

    private LinearLayout.LayoutParams margins(int left, int top, int right, int bottom) {
        LinearLayout.LayoutParams params = new LinearLayout.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT,
            ViewGroup.LayoutParams.WRAP_CONTENT);
        params.setMargins(HaloUi.dp(this, left), HaloUi.dp(this, top), HaloUi.dp(this, right),
            HaloUi.dp(this, bottom));
        return params;
    }

    private void addButton(String label, Runnable action) {
        Button button = new Button(this);
        button.setText(label);
        HaloUi.styleButton(this, button);
        button.setOnClickListener(view -> action.run());
        LinearLayout.LayoutParams params = new LinearLayout.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT,
            ViewGroup.LayoutParams.WRAP_CONTENT);
        params.bottomMargin = HaloUi.dp(this, 10);
        buttons.addView(button, params);
    }

    private void setStatus(String value) {
        status.setText(value);
    }

    private void log(String line) {
        lines.add(line);
        while (lines.size() > 6)
            lines.remove(0);
        StringBuilder builder = new StringBuilder();
        for (int i = 0; i < lines.size(); i++) {
            if (i > 0)
                builder.append('\n');
            builder.append(lines.get(i));
        }
        readout.setText(builder);
    }

    private void showProgress(boolean visible) {
        progress.setVisibility(visible ? View.VISIBLE : View.GONE);
        percent.setVisibility(visible ? View.VISIBLE : View.GONE);
    }

    // ---------------------------------------------------------------- the search

    /**
     * Looks for an Xbox disc image where the player would keep one: the
     * app's own folder (where it can be pushed), then the shared storage
     * (Downloads, Documents and the top level).
     */
    private void scanForImages() {
        if (extracting || scanning || buttons == null)
            return;
        scanning = true;
        setStatus("Scanning for archive image");
        new Thread(() -> {
            boolean access = hasArchiveAccess();
            Set<String> seen = new LinkedHashSet<>();
            List<File> found = new ArrayList<>();
            scan(new File(dataRoot.getAbsolutePath()), 3, seen, found);
            scan(Environment.getExternalStoragePublicDirectory(Environment.DIRECTORY_DOWNLOADS), 2, seen, found);
            scan(Environment.getExternalStoragePublicDirectory(Environment.DIRECTORY_DOCUMENTS), 2, seen, found);
            scan(Environment.getExternalStorageDirectory(), 2, seen, found);
            handler.post(() -> {
                scanning = false;
                presentScan(found, access);
            });
        }, "archive-scan").start();
    }

    private void scan(File directory, int depth, Set<String> seen, List<File> out) {
        if (directory == null || depth < 0)
            return;
        File[] children = directory.listFiles();
        if (children == null)
            return;
        for (File child : children) {
            if (child.isDirectory()) {
                if (depth > 0 && !child.getName().equals("Android"))
                    scan(child, depth - 1, seen, out);
                continue;
            }
            String name = child.getName().toLowerCase(Locale.US);
            if ((name.endsWith(".iso") || name.endsWith(".xiso")) && seen.add(child.getAbsolutePath()))
                out.add(child);
        }
    }

    private void presentScan(List<File> found, boolean access) {
        if (extracting)
            return;
        foundImages.clear();
        foundImages.addAll(found);
        archiveAccess = access;
        buttons.removeAllViews();

        if (!access)
            log("> deep scan locked: grant archive access to search shared storage");

        if (!found.isEmpty()) {
            setStatus("Archive image detected");
            log("> " + found.size() + " candidate(s) located");
            if (autoExtractPending && found.size() == 1) {
                autoExtractPending = false;
                log("> opening " + found.get(0).getName());
                beginExtract(found.get(0), found.get(0).getName());
                return;
            }
            autoExtractPending = false;
            for (File image : found) {
                String label = image.getName();
                addButton("Restore from " + trim(label, 30), () -> beginExtract(image, label));
            }
        } else {
            autoExtractPending = false;
            setStatus("No archive image found");
            log("> no .iso or .xiso in reach");
        }

        if (!access)
            addButton("Grant archive access", this::requestArchiveAccess);
        addButton("Select disc image", this::pickImage);
        addButton("Select game data folder", this::pickFolder);
        addButton("Scan again", this::scanForImages);
    }

    private static String trim(String value, int length) {
        return value.length() <= length ? value : value.substring(0, length - 1) + "…";
    }

    private boolean hasArchiveAccess() {
        if (Build.VERSION.SDK_INT >= 30)
            return Environment.isExternalStorageManager();
        return checkSelfPermission(Manifest.permission.READ_EXTERNAL_STORAGE) == PackageManager.PERMISSION_GRANTED;
    }

    private void requestArchiveAccess() {
        log("> requesting archive access");
        if (Build.VERSION.SDK_INT >= 30) {
            try {
                startActivity(new Intent(Settings.ACTION_MANAGE_APP_ALL_FILES_ACCESS_PERMISSION,
                    Uri.parse("package:" + getPackageName())));
            } catch (Exception exception) {
                startActivity(new Intent(Settings.ACTION_MANAGE_ALL_FILES_ACCESS_PERMISSION));
            }
        } else {
            requestPermissions(new String[] { Manifest.permission.READ_EXTERNAL_STORAGE }, REQUEST_STORAGE);
        }
    }

    @Override
    public void onRequestPermissionsResult(int requestCode, String[] permissions, int[] results) {
        super.onRequestPermissionsResult(requestCode, permissions, results);
        if (requestCode == REQUEST_STORAGE)
            scanForImages();
    }

    private void pickImage() {
        Intent intent = new Intent(Intent.ACTION_OPEN_DOCUMENT);
        intent.addCategory(Intent.CATEGORY_OPENABLE);
        intent.setType("*/*");
        intent.putExtra(Intent.EXTRA_MIME_TYPES,
            new String[] { "application/x-iso9660-image", "application/octet-stream" });
        startActivityForResult(intent, PICK_IMAGE);
    }

    private void pickFolder() {
        startActivityForResult(new Intent(Intent.ACTION_OPEN_DOCUMENT_TREE), PICK_FOLDER);
    }

    private String displayName(Uri uri) {
        try (Cursor cursor = getContentResolver().query(uri, new String[] { OpenableColumns.DISPLAY_NAME },
            null, null, null)) {
            if (cursor != null && cursor.moveToFirst())
                return cursor.getString(0);
        } catch (Exception exception) {
            // fall through to the last path segment
        }
        String path = uri.getLastPathSegment();
        return path != null ? path : "disc image";
    }

    // ---------------------------------------------------------------- the recovery

    private void beginExtract(File image, String label) {
        beginExtract(image, null, label);
    }

    private void beginExtract(Uri image, String label) {
        beginExtract(null, image, label);
    }

    private void beginExtract(File file, Uri uri, String label) {
        if (extracting)
            return;
        extracting = true;
        cancelled = false;
        lastLoggedPercent = -1;
        buttons.removeAllViews();
        seal.setText("LIVE");
        seal.setTextColor(HaloUi.AMBER);
        setStatus("Reading archive");
        log("> " + trim(label, 40));
        showProgress(true);
        progress.setProgress(0f);
        percent.setText("0%");

        new Thread(() -> {
            FileChannel channel = null;
            ParcelFileDescriptor descriptor = null;
            try {
                if (file != null) {
                    channel = DiscImage.open(file);
                } else {
                    descriptor = getContentResolver().openFileDescriptor(uri, "r");
                    if (descriptor == null)
                        throw new IOException("Could not open the disc image.");
                    channel = new FileInputStream(descriptor.getFileDescriptor()).getChannel();
                }
                DiscImage.extract(channel, dataRoot, (name, done, total) -> {
                    if (cancelled)
                        return false;
                    handler.post(() -> report(name, done, total));
                    return true;
                });
                handler.post(this::finishExtract);
            } catch (DiscImage.Cancelled stop) {
                handler.post(() -> reset("Recovery halted by operator."));
            } catch (Exception exception) {
                handler.post(() -> reset(exception.getMessage() != null ? exception.getMessage()
                    : exception.toString()));
            } finally {
                close(channel);
                close(descriptor);
            }
        }, "archive-recovery").start();
    }

    private void report(String name, long done, long total) {
        float fraction = total > 0 ? (float) done / total : 0f;
        progress.setProgress(fraction);
        int current = (int) (fraction * 100);
        percent.setText(current + "%");
        setStatus("Reading " + trim(name, 26).toUpperCase(Locale.US));
        if (current / 10 != lastLoggedPercent / 10) {
            lastLoggedPercent = current;
            log("> maps/" + name + "  " + current + "%");
        }
    }

    private void finishExtract() {
        extracting = false;
        seal.setText("SEALED");
        seal.setTextColor(HaloUi.CYAN);
        if (haveData()) {
            setStatus("Archive restored");
            log("> maps/ui.map verified — starting game");
            handler.postDelayed(this::startGame, 650);
        } else {
            reset("The copy finished but maps/ui.map is missing.");
        }
    }

    private void reset(String message) {
        extracting = false;
        seal.setText("HALTED");
        seal.setTextColor(HaloUi.AMBER);
        showProgress(false);
        setStatus("Recovery halted");
        log("! " + message);
        presentScan(foundImages, archiveAccess);
    }

    private static void close(java.io.Closeable closeable) {
        if (closeable != null) {
            try {
                closeable.close();
            } catch (IOException ignored) {
                // best effort
            }
        }
    }

    @Override
    public void onBackPressed() {
        if (extracting) {
            cancelled = true;
            log("> halt requested…");
            return;
        }
        super.onBackPressed();
    }

    @Override
    protected void onActivityResult(int requestCode, int resultCode, Intent data) {
        super.onActivityResult(requestCode, resultCode, data);
        if (resultCode != RESULT_OK || data == null || data.getData() == null)
            return;
        Uri uri = data.getData();
        if (requestCode == PICK_IMAGE) {
            try {
                getContentResolver().takePersistableUriPermission(uri, Intent.FLAG_GRANT_READ_URI_PERMISSION);
            } catch (Exception exception) {
                // some providers do not offer a persistable grant
            }
            beginExtract(uri, displayName(uri));
        } else if (requestCode == PICK_FOLDER) {
            extracting = true;
            cancelled = false;
            buttons.removeAllViews();
            seal.setText("LIVE");
            seal.setTextColor(HaloUi.AMBER);
            setStatus("Copying game data");
            showProgress(true);
            progress.setProgress(0f);
            new Thread(() -> importData(uri), "game-data-import").start();
        }
    }

    // ---------------------------------------------------------------- the maps folder

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
                handler.post(() -> reset("That folder does not contain maps/ui.map."));
                return;
            }
            long total = 0, done = 0;
            for (Entry entry : entries)
                total += entry.size;
            byte[] buffer = new byte[1 << 20];
            for (Entry entry : entries) {
                if (cancelled)
                    throw new DiscImage.Cancelled();
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
                        final long copied = done;
                        final String copying = path;
                        final long all = total;
                        handler.post(() -> {
                            float fraction = all > 0 ? (float) copied / all : 0f;
                            progress.setProgress(fraction);
                            percent.setText((int) (fraction * 100) + "%");
                            setStatus("Copying " + trim(copying, 24).toUpperCase(Locale.US));
                        });
                    }
                }
                if (!partial.renameTo(destination))
                    throw new IOException("cannot write " + destination);
            }
            handler.post(() -> {
                extracting = false;
                if (haveData()) {
                    setStatus("Archive restored");
                    log("> maps/ui.map verified — starting game");
                    handler.postDelayed(this::startGame, 650);
                } else {
                    reset("The copy finished but maps/ui.map is missing.");
                }
            });
        } catch (DiscImage.Cancelled stop) {
            handler.post(() -> reset("Copy halted by operator."));
        } catch (Exception exception) {
            handler.post(() -> reset("Copying failed: " + exception.getMessage()));
        }
    }
}
