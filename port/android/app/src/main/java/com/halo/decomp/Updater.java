package com.halo.decomp;

import android.app.Activity;
import android.app.AlertDialog;
import android.content.Intent;
import android.net.Uri;
import android.opengl.EGL14;
import android.opengl.EGLConfig;
import android.opengl.EGLContext;
import android.opengl.EGLDisplay;
import android.opengl.EGLSurface;
import android.opengl.GLES20;
import android.util.TypedValue;
import android.widget.LinearLayout;
import android.widget.ProgressBar;
import android.widget.TextView;

import org.json.JSONObject;

import java.io.ByteArrayOutputStream;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.net.HttpURLConnection;
import java.net.URL;
import java.nio.charset.StandardCharsets;
import java.security.MessageDigest;
import java.util.ArrayList;
import java.util.List;
import java.util.Locale;
import java.util.regex.Matcher;
import java.util.regex.Pattern;
import java.util.zip.ZipEntry;
import java.util.zip.ZipInputStream;

/**
 * The app's self-updater, as the desktop games' (port/linux/src/updater.c).
 *
 * A build of the main branch made by GitHub Actions knows its build number
 * (BuildConfig.HALO_BUILD_NUMBER, the workflow's run number, which names its
 * release: build-number); other builds have none and never look. When
 * update.auto in config.toml is true (the default), the game asks GitHub for
 * the latest release when it starts, on a thread of its own, and if it is
 * newer asks the player whether to update:
 *
 * - Yes: the release's app (halo-android-release.zip or -debug.zip) is
 *   downloaded and handed to Android's package installer, which replaces the
 *   game (closing it) and offers to open the new version.
 * - No: nothing, until the next start.
 * - Do not ask again: after the player confirms it, update.auto = false is
 *   written to config.toml.
 *
 * Every build of main is signed with the same key (the workflow's), which an
 * app must keep for Android to install a new version over it.
 *
 * It also downloads the open-source Vulkan driver for Adreno GPUs, for
 * LauncherActivity (driverDownload): see there.
 */
final class Updater {
    private static final String REPOSITORY = "OpenCommunityEdition/OpenCE";
    private static final String USER_AGENT = "halo-ce-universal-updater";
    private static final int TIMEOUT_MILLISECONDS = 20000;

    private Updater() {
    }

    /** At the game's start: looks for a new version, in the background. */
    static void start(Activity activity) {
        File config = configFile(activity);

        if (BuildConfig.HALO_BUILD_NUMBER <= 0 || config == null || !autoUpdate(config))
            return;
        new Thread(() -> {
            int latest = latestRelease();

            if (latest > BuildConfig.HALO_BUILD_NUMBER)
                activity.runOnUiThread(() -> ask(activity, latest));
        }, "update check").start();
    }

    /* ---------- the open-source Vulkan driver for Adreno GPUs

    When the player has chosen Turnip (display.vk_driver = "auto", Video Setup's Vulkan driver), the Turnip build (Mesa's
    Vulkan driver for Adreno) for an Adreno's series is downloaded into the data folder when it is not there, from the project
    that releases it (port/android/README.md, "Graphics: OpenGL ES and Vulkan"). The download must have the size and SHA-256 of
    the build that was chosen, or it is dropped. It is looked for whatever update.auto says: the driver is not an update of
    the game. LauncherActivity does it before the game starts, with the download's progress on screen, so that the game finds
    the driver as it starts.

    Which archive is this phone's is decided once, from the GPU's name, and written to vk_driver_auto.txt in the data folder
    (the archive's name, or an empty line for a GPU that has none): display.vk_driver = "auto" is that archive
    (port/android/host/host_vk_driver.c), so that a player turns Turnip on with one setting. The default driver is the
    phone's own (display.vk_driver = ""), for which nothing is downloaded. */

    static final class Driver {
        final String name, url, sha256, series;
        final long size;
        final int firstModel, lastModel;

        Driver(String name, String url, long size, String sha256, int firstModel, int lastModel, String series) {
            this.name = name;
            this.url = url;
            this.size = size;
            this.sha256 = sha256;
            this.firstModel = firstModel;
            this.lastModel = lastModel;
            this.series = series;
        }
    }

    private static final String RELEASE = "https://github.com/K11MCH1/AdrenoToolsDrivers/releases/download/v26.0.0-rc08/";
    private static final Driver[] DRIVERS = {
        // the build the Vulkan renderer was tested with (Adreno 750)
        new Driver("Turnip_v26.0.0_R8.zip", RELEASE + "Turnip_v26.0.0_R8.zip", 3478359,
            "e634db0f929e2205e95511c769071817d0390180ec72c8e690bc76375e813715", 600, 799, "6xx or 7xx"),
        // the same release's build for the A8xx series ("Turnip A8XX Draft"), not tried here
        new Driver("Turnip_v26.0.0_R8_A8xx.zip", RELEASE + "turnip_a8xx.zip", 2477437,
            "27e400a39cae06ac22695d74b837449868f712c679662c5793776bd575eca3bd", 800, 899, "8xx"),
    };
    private static final String AUTO_FILE = "vk_driver_auto.txt";

    private static Driver driverNamed(String name) {
        for (Driver driver : DRIVERS) {
            if (driver.name.equals(name))
                return driver;
        }
        return null;
    }

    /** whether this phone's archive was decided and is in the data folder at root (or it has none) */
    static boolean driverReady(File root) {
        if (root == null)
            return true;
        List<String> lines = readLines(new File(root, AUTO_FILE));

        if (lines.isEmpty())
            return false;
        Driver driver = driverNamed(lines.get(0).trim());

        return driver == null || new File(root, driver.name).length() == driver.size;
    }

    /** the GPU's name, and this phone's archive (null for a GPU that has none) */
    static final class Choice {
        final String gpu;
        final Driver driver;

        Choice(String gpu, Driver driver) {
            this.gpu = gpu;
            this.driver = driver;
        }
    }

    /** decides this phone's archive from its GPU and records it in vk_driver_auto.txt; null if the GPU cannot be named */
    static Choice driverChoose(File root) {
        String renderer = glRenderer();
        int model = adrenoModel(renderer);
        Driver chosen = null;

        if (renderer.isEmpty()) {
            android.util.Log.i("halo", "driver: the GPU could not be named; nothing is downloaded this time");
            return null;
        }
        for (Driver driver : DRIVERS) {
            if (model >= driver.firstModel && model <= driver.lastModel)
                chosen = driver;
        }
        android.util.Log.i("halo", "driver: the GPU is \"" + renderer + "\": " + (chosen != null ? chosen.name
            : "no Turnip build for it; Vulkan runs on the phone's own driver"));
        File partial = new File(root, AUTO_FILE + ".tmp");
        try (OutputStream out = new FileOutputStream(partial)) {
            out.write(((chosen != null ? chosen.name : "") + "\n").getBytes(StandardCharsets.UTF_8));
        } catch (IOException e) {
            partial.delete();
            return new Choice(renderer, chosen);
        }
        if (!partial.renameTo(new File(root, AUTO_FILE)))
            partial.delete();
        return new Choice(renderer, chosen);
    }

    /** whether an archive is in the data folder at root */
    static boolean driverPresent(File root, Driver driver) {
        return new File(root, driver.name).length() == driver.size;
    }

    /** the archive downloaded into root (as a .partial file, renamed once it is checked); throws why it could not be */
    static void driverDownload(File root, Driver driver, String gpu, Progress progress) throws Exception {
        File archive = new File(root, driver.name);
        File partial = new File(root, driver.name + ".partial");

        try {
            android.util.Log.i("halo", "driver: downloading " + driver.url + " for the " + gpu);
            download(driver.url, partial, progress);
            if (partial.length() != driver.size || !driver.sha256.equals(sha256(partial)))
                throw new IOException("the download is not the build that was chosen (" + partial.length() + " bytes)");
            archive.delete();
            if (!partial.renameTo(archive))
                throw new IOException("it could not be renamed to " + archive);
            android.util.Log.i("halo", "driver: " + driver.name + " is in the data folder");
        } catch (Exception e) {
            partial.delete();
            android.util.Log.i("halo", "driver: could not download " + driver.name + ": " + e);
            throw e;
        }
    }

    /** the number of an Adreno GPU from its OpenGL ES renderer string ("Adreno (TM) 750"), 0 for any other GPU */
    private static int adrenoModel(String renderer) {
        Matcher matcher = Pattern.compile("adreno\\D*(\\d{3})").matcher(renderer.toLowerCase(Locale.ROOT));

        return matcher.find() ? Integer.parseInt(matcher.group(1)) : 0;
    }

    /**
     * The GPU's OpenGL ES renderer string, from a context of its own on a 1x1 pbuffer ("" if none can be made). The display
     * is not terminated: the game's renderer shares it.
     */
    private static String glRenderer() {
        EGLDisplay display = EGL14.eglGetDisplay(EGL14.EGL_DEFAULT_DISPLAY);
        EGLContext context = EGL14.EGL_NO_CONTEXT;
        EGLSurface surface = EGL14.EGL_NO_SURFACE;
        int[] version = new int[2];

        if (display == EGL14.EGL_NO_DISPLAY || !EGL14.eglInitialize(display, version, 0, version, 1))
            return "";
        try {
            int[] attributes = { EGL14.EGL_RENDERABLE_TYPE, EGL14.EGL_OPENGL_ES2_BIT, EGL14.EGL_SURFACE_TYPE,
                EGL14.EGL_PBUFFER_BIT, EGL14.EGL_NONE };
            EGLConfig[] configs = new EGLConfig[1];
            int[] count = new int[1];

            if (!EGL14.eglChooseConfig(display, attributes, 0, configs, 0, 1, count, 0) || count[0] < 1)
                return "";
            context = EGL14.eglCreateContext(display, configs[0], EGL14.EGL_NO_CONTEXT,
                new int[] { EGL14.EGL_CONTEXT_CLIENT_VERSION, 2, EGL14.EGL_NONE }, 0);
            surface = EGL14.eglCreatePbufferSurface(display, configs[0],
                new int[] { EGL14.EGL_WIDTH, 1, EGL14.EGL_HEIGHT, 1, EGL14.EGL_NONE }, 0);
            if (context == EGL14.EGL_NO_CONTEXT || surface == EGL14.EGL_NO_SURFACE
                || !EGL14.eglMakeCurrent(display, surface, surface, context))
                return "";
            String renderer = GLES20.glGetString(GLES20.GL_RENDERER);

            return renderer != null ? renderer : "";
        } finally {
            EGL14.eglMakeCurrent(display, EGL14.EGL_NO_SURFACE, EGL14.EGL_NO_SURFACE, EGL14.EGL_NO_CONTEXT);
            if (surface != EGL14.EGL_NO_SURFACE)
                EGL14.eglDestroySurface(display, surface);
            if (context != EGL14.EGL_NO_CONTEXT)
                EGL14.eglDestroyContext(display, context);
        }
    }

    private static String sha256(File file) throws Exception {
        MessageDigest digest = MessageDigest.getInstance("SHA-256");
        StringBuilder text = new StringBuilder();

        try (InputStream stream = new FileInputStream(file)) {
            byte[] buffer = new byte[65536];
            int count;

            while ((count = stream.read(buffer)) > 0)
                digest.update(buffer, 0, count);
        }
        for (byte b : digest.digest())
            text.append(String.format(Locale.ROOT, "%02x", b));
        return text.toString();
    }

    static File configFile(Activity activity) {
        File root = activity.getExternalFilesDir(null);

        return root != null ? new File(root, "config.toml") : null;
    }

    /* ---------- config.toml's update.auto */

    private static boolean autoUpdate(File config) {
        String value = readSetting(config, "update", "auto");

        return value == null || !value.startsWith("false");
    }

    /** update.auto = false written into config.toml (only its line changed) */
    static boolean writeAutoUpdateOff(File config) {
        return writeSetting(config, "update", "auto", "false");
    }

    /** a setting's value as config.toml has it (a string with its quotes), null when it is not there */
    static String readSetting(File config, String wantedSection, String key) {
        String section = "";

        for (String line : readLines(config)) {
            String trimmed = line.trim();

            if (trimmed.startsWith("[") && trimmed.contains("]")) {
                section = trimmed.substring(1, trimmed.indexOf(']')).trim();
            } else if (section.equals(wantedSection) && isKey(trimmed, key)) {
                return trimmed.substring(trimmed.indexOf('=') + 1).trim();
            }
        }
        return null;
    }

    /** key = value written into config.toml's section (only its line changed; added when it is not there) */
    static boolean writeSetting(File config, String wantedSection, String key, String value) {
        List<String> lines = readLines(config);
        List<String> out = new ArrayList<>();
        String section = "";
        boolean inSection = false, written = false;

        for (String line : lines) {
            String trimmed = line.trim();

            if (trimmed.startsWith("[") && trimmed.contains("]")) {
                if (inSection && !written) {
                    out.add(key + " = " + value);
                    written = true;
                }
                section = trimmed.substring(1, trimmed.indexOf(']')).trim();
                inSection = section.equals(wantedSection);
            } else if (inSection && !written && isKey(trimmed, key)) {
                out.add(key + " = " + value);
                written = true;
                continue;
            }
            out.add(line);
        }
        if (!written) {
            if (!inSection) {
                out.add("");
                out.add("[" + wantedSection + "]");
            }
            out.add(key + " = " + value);
        }
        StringBuilder text = new StringBuilder();
        for (String line : out)
            text.append(line).append('\n');
        try (OutputStream stream = new FileOutputStream(config)) {
            stream.write(text.toString().getBytes(StandardCharsets.UTF_8));
            return true;
        } catch (IOException e) {
            return false;
        }
    }

    private static boolean isKey(String trimmed, String key) {
        return trimmed.startsWith(key) && trimmed.substring(key.length()).trim().startsWith("=");
    }

    private static List<String> readLines(File file) {
        List<String> lines = new ArrayList<>();

        try (InputStream stream = new FileInputStream(file)) {
            String text = new String(readAll(stream), StandardCharsets.UTF_8);

            for (String line : text.split("\n", -1))
                lines.add(line.endsWith("\r") ? line.substring(0, line.length() - 1) : line);
            if (!lines.isEmpty() && lines.get(lines.size() - 1).isEmpty())
                lines.remove(lines.size() - 1);
        } catch (IOException e) {
            // no file yet: every setting at its default
        }
        return lines;
    }

    private static byte[] readAll(InputStream stream) throws IOException {
        ByteArrayOutputStream out = new ByteArrayOutputStream();
        byte[] buffer = new byte[16384];
        int count;

        while ((count = stream.read(buffer)) > 0)
            out.write(buffer, 0, count);
        return out.toByteArray();
    }

    /* ---------- GitHub */

    private static HttpURLConnection open(String address) throws IOException {
        HttpURLConnection connection = (HttpURLConnection) new URL(address).openConnection();

        connection.setConnectTimeout(TIMEOUT_MILLISECONDS);
        connection.setReadTimeout(TIMEOUT_MILLISECONDS);
        connection.setRequestProperty("User-Agent", USER_AGENT);
        connection.setInstanceFollowRedirects(true);
        return connection;
    }

    /** the build number of GitHub's latest release, 0 if there is none */
    private static int latestRelease() {
        try {
            HttpURLConnection connection = open("https://api.github.com/repos/" + REPOSITORY + "/releases/latest");

            connection.setRequestProperty("Accept", "application/vnd.github+json");
            try (InputStream stream = connection.getInputStream()) {
                String tag = new JSONObject(new String(readAll(stream), StandardCharsets.UTF_8)).optString("tag_name");

                return tag.startsWith("build-") ? Integer.parseInt(tag.substring(6)) : 0;
            } finally {
                connection.disconnect();
            }
        } catch (Exception e) {
            android.util.Log.i("halo", "update: could not check for a new version: " + e);
            return 0;
        }
    }

    /* ---------- the player's answer */

    private static void ask(Activity activity, int latest) {
        if (activity.isFinishing())
            return;
        new AlertDialog.Builder(activity)
            .setTitle("Halo: new version")
            .setMessage("A new version of Halo was detected (build " + latest + "; this is build "
                + BuildConfig.HALO_BUILD_NUMBER + ").\n\nDo you want to update? The game will close and start "
                + "the new version.")
            .setCancelable(false)
            .setPositiveButton("Yes", (dialog, which) -> update(activity, latest))
            .setNegativeButton("No", null)
            .setNeutralButton("Do not ask again", (dialog, which) -> confirmNever(activity))
            .show();
    }

    private static void confirmNever(Activity activity) {
        new AlertDialog.Builder(activity)
            .setTitle("Halo: new version")
            .setMessage("Stop asking about new versions?\n\nTo ask again, set auto = true in the [update] section "
                + "of config.toml.")
            .setCancelable(false)
            .setPositiveButton("Yes", (dialog, which) -> {
                File config = configFile(activity);

                if (config != null)
                    writeAutoUpdateOff(config);
            })
            .setNegativeButton("No", null)
            .show();
    }

    /* ---------- updating */

    private static void update(Activity activity, int latest) {
        String asset = "halo-android-" + (BuildConfig.DEBUG ? "debug" : "release") + ".zip";
        File directory = new File(activity.getCacheDir(), UpdateProvider.DIRECTORY);
        LinearLayout layout = new LinearLayout(activity);
        TextView status = new TextView(activity);
        ProgressBar bar = new ProgressBar(activity, null, android.R.attr.progressBarStyleHorizontal);
        int padding = (int) TypedValue.applyDimension(TypedValue.COMPLEX_UNIT_DIP, 24,
            activity.getResources().getDisplayMetrics());

        layout.setOrientation(LinearLayout.VERTICAL);
        layout.setPadding(padding, padding / 2, padding, 0);
        status.setText("Downloading build " + latest + "...");
        bar.setMax(1000);
        layout.addView(status);
        layout.addView(bar);
        AlertDialog progress = new AlertDialog.Builder(activity)
            .setTitle("Halo: new version")
            .setView(layout)
            .setCancelable(false)
            .show();

        new Thread(() -> {
            try {
                directory.mkdirs();
                File zip = new File(directory, "update.zip");
                File apk = new File(directory, UpdateProvider.APK);

                download("https://github.com/" + REPOSITORY + "/releases/download/build-" + latest + "/" + asset, zip,
                    (received, total) -> activity.runOnUiThread(() -> {
                        bar.setProgress(total > 0 ? (int) (received * 1000 / total) : 0);
                        status.setText("Downloading build " + latest + "... (" + (received >> 20) + " of "
                            + (total >> 20) + " MB)");
                    }));
                extractApk(zip, apk);
                zip.delete();
                activity.runOnUiThread(() -> {
                    progress.dismiss();
                    install(activity);
                });
            } catch (Exception e) {
                android.util.Log.i("halo", "update: failed: " + e);
                activity.runOnUiThread(() -> {
                    progress.dismiss();
                    new AlertDialog.Builder(activity)
                        .setTitle("Halo: new version")
                        .setMessage("The update failed:\n\n" + e.getMessage())
                        .setPositiveButton("OK", null)
                        .show();
                });
            }
        }, "update download").start();
    }

    interface Progress {
        void report(long received, long total);
    }

    private static void download(String address, File file, Progress progress) throws IOException {
        HttpURLConnection connection = open(address);

        try {
            int status = connection.getResponseCode();

            if (status != 200)
                throw new IOException("the server answered " + status);
            long total = connection.getContentLengthLong();
            long received = 0, reported = 0;
            byte[] buffer = new byte[65536];

            try (InputStream in = connection.getInputStream(); OutputStream out = new FileOutputStream(file)) {
                int count;

                while ((count = in.read(buffer)) > 0) {
                    out.write(buffer, 0, count);
                    received += count;
                    if (received - reported >= 256 * 1024 || received == total) {
                        progress.report(received, total);
                        reported = received;
                    }
                }
            }
            if (total > 0 && received != total)
                throw new IOException("the download broke off");
        } finally {
            connection.disconnect();
        }
    }

    /** the zip's app (its one .apk) to apk */
    private static void extractApk(File zip, File apk) throws IOException {
        try (ZipInputStream in = new ZipInputStream(new FileInputStream(zip))) {
            ZipEntry entry;

            while ((entry = in.getNextEntry()) != null) {
                if (entry.isDirectory() || !entry.getName().endsWith(".apk"))
                    continue;
                try (OutputStream out = new FileOutputStream(apk)) {
                    byte[] buffer = new byte[65536];
                    int count;

                    while ((count = in.read(buffer)) > 0)
                        out.write(buffer, 0, count);
                }
                return;
            }
        }
        throw new IOException("the download has no app in it");
    }

    /**
     * Android's package installer, with the new app: it asks the player to
     * allow this app to install apps the first time, replaces the game
     * (closing it) and offers to open the new version.
     */
    private static void install(Activity activity) {
        Intent intent = new Intent(Intent.ACTION_VIEW);

        intent.setDataAndType(Uri.parse("content://" + UpdateProvider.AUTHORITY + "/" + UpdateProvider.APK),
            "application/vnd.android.package-archive");
        intent.addFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION | Intent.FLAG_ACTIVITY_NEW_TASK);
        activity.startActivity(intent);
    }
}
