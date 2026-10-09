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
 * The launcher: the game's menu once its data is in place (play, the mods,
 * resetting the settings), and its import before then.
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
    private static final int PICK_RESOURCES = 2;

    private File dataRoot;
    private View launcherRoot;
    private GameDataManager dataManager;
    private TextView status, updateStatus;
    private ProgressBar progress;
    private Button pick;
    private final Handler handler = new Handler(Looper.getMainLooper());

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        GamepadNavigation.install(getWindow());
        RunLog.start(this);
        RunLog.line("Launcher opened");
        dataRoot = getExternalFilesDir(null);
        // created by the app, so that files pushed into it with adb stay
        // readable (a directory adb creates there belongs to the shell user)
        if (dataRoot != null)
            new File(dataRoot, "maps").mkdirs();
        passOnHardwareId();
        passOnInvite(getIntent());
        if (haveData()) {
            // an invite link goes straight to the game; otherwise the menu
            if (isInvite(getIntent()))
                startGame();
            else
                buildMenu();
            return;
        }
        buildInterface();
    }

    private static boolean isInvite(Intent intent) {
        return intent != null && Intent.ACTION_VIEW.equals(intent.getAction()) && intent.getData() != null;
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
        writeInvite(intent.getData().toString());
    }

    private boolean writeInvite(String text) {
        String invite = ServerInvite.normalize(text);
        File root = gameRoot();
        if (invite == null || root == null) return false;
        File pvpHost = new File(root, "pvp_host.txt");
        if (pvpHost.exists() && !pvpHost.delete()) return false;
        File pendingHost = new File(root, "coop_host.txt");
        if (pendingHost.exists() && !pendingHost.delete()) return false;
        File partial = new File(root, "join_link.txt.tmp");
        try (OutputStream out = new FileOutputStream(partial)) {
            out.write(invite.getBytes("UTF-8"));
        } catch (java.io.IOException e) {
            partial.delete();
            return false;
        }
        if (partial.renameTo(new File(root, "join_link.txt"))) return true;
        partial.delete();
        return false;
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
        File identityRoot=gameRoot();if(identityRoot==null)return;
        File partial = new File(identityRoot, "hardware_id.txt.tmp");
        try (OutputStream out = new FileOutputStream(partial)) {
            out.write(id.getBytes("UTF-8"));
        } catch (java.io.IOException e) {
            partial.delete();
            return;
        }
        if (!partial.renameTo(new File(identityRoot, "hardware_id.txt")))
            partial.delete();
    }

    private boolean haveData() {
        File root=gameRoot();return root!=null&&new File(root,"maps/ui.map").isFile();
    }

    private boolean readyToPlay() {
        if (busy || (dataManager!=null&&dataManager.busy())) return false;
        if(!haveData()){dataManager().show();return false;}
        if (new ModInstaller(gameRoot()).recoveryNeeded(ModInstaller.SPV1)) {
            if (mods == null) buildMenu();
            selectMod(ModInstaller.SPV1);
            status.setText("An interrupted mod change needs recovery. Restore the original campaign or uninstall before playing.");
            return false;
        }
        return true;
    }

    private boolean startGame() {
        if (!readyToPlay()) return false;
        try { NetworkProfile.prepare(this, gameRoot()); }
        catch (java.io.IOException e) {
            LauncherHelp.page(this, "Network selection could not be saved", e.getMessage());
            return false;
        }
        RunLog.line("Starting game; data root=" + gameRoot());
        File[] mapFiles = new File(gameRoot(), "maps").listFiles((dir, name) -> name.endsWith(".map"));
        if (mapFiles != null) for (File mapFile : mapFiles) RunLog.line("Data header: " + mapFile.getName() + " " + MapInfo.read(mapFile).summary() + " bytes=" + mapFile.length());
        Intent game = new Intent(this, HaloActivity.class);

        // in the VR build, the headset opens the game immersive only when the
        // intent says so; without these it opens a 2D window, where the
        // OpenXR session never gets focus (a black screen, and the game
        // waiting on the headset between frames)
        if (BuildConfig.APPLICATION_ID.endsWith(".vr")) {
            game.addCategory("org.khronos.openxr.intent.category.IMMERSIVE_HMD");
            game.addCategory("com.oculus.intent.category.VR");
        }
        startActivity(game);
        finish();
        return true;
    }

    /** Plain Play reaches the native menus, even after an older launcher crashed before consuming its command. */
    private void startFromMenu() {
        if (!readyToPlay()) return;
        try {
            int retired = LauncherRequests.retire(gameRoot());
            if (retired > 0) RunLog.line("Preserved " + retired + " abandoned launch requests in launcher-history; opening menus");
        } catch (java.io.IOException e) {
            LauncherHelp.page(this, "Could not clear an old launch request",
                "The game was not started, so an old host/join request will not run. Your saved data is unchanged. "
                + "Check available storage and the active game folder, then try Play again. " + e.getMessage());
            return;
        }
        // An external invite may have waited here while its game files were imported.
        // Re-apply that explicit intent to the now-active set after retiring old commands.
        if (isInvite(getIntent())) {
            String invite = ServerInvite.normalize(getIntent().getData().toString());
            if (invite != null && !writeInvite(invite)) {
                LauncherHelp.page(this, "Could not open invite", "The invite could not be saved in the active game folder. Try opening the link again.");
                return;
            }
        }
        startGame();
    }

    /** The community catalog is a launcher fallback; the game keeps its own signed broker/LAN browser. */
    private void openServerBrowser(boolean campaign) {
        if (!readyToPlay()) return;
        new ServerBrowser(this, invite -> {
            if (!writeInvite(invite)) {
                if (status != null) status.setText("Could not save the server invite. Check available storage and try again.");
                return false;
            }
            return startGame();
        }, campaign);
    }

    /**
     * The folder the game takes its data from: the app's storage, or in the
     * VR build the headset's Documents/HaloCE when the data is there
     * (port/android/host/host_main.c chooses the same way).
     */
    File baseRoot() {
        File shared = new File("/sdcard/Documents/HaloCE");

        if (BuildConfig.APPLICATION_ID.endsWith(".vr") && new File(shared, "maps/ui.map").isFile())
            return shared;
        return dataRoot;
    }

    File gameRoot() { return GameDataLibrary.activeRoot(baseRoot()); }
    private GameDataManager dataManager() {
        if(dataManager==null)dataManager=new GameDataManager(this,baseRoot(),()->{passOnHardwareId();if(haveData())buildMenu();else buildInterface();});
        return dataManager;
    }
    @Override protected void onDestroy(){if(dataManager!=null)dataManager.close();super.onDestroy();}

    void withServerMap(String advertised,Runnable join) {
        if(busy||(dataManager!=null&&dataManager.busy()))return;
        String map=advertised==null?"":advertised.toLowerCase(java.util.Locale.ROOT);
        if(map.endsWith(".map"))map=map.substring(0,map.length()-4);
        if(!map.matches("[a-z0-9_-]{1,80}")){join.run();return;}
        File active=gameRoot();
        if(active!=null&&MapInfo.read(new File(active,"maps/"+map+".map")).multiplayer){join.run();return;}
        try {
            GameDataLibrary library=new GameDataLibrary(baseRoot());java.util.List<GameDataLibrary.Profile> matches=library.withMap(map);
            if(matches.isEmpty()){LauncherHelp.page(this,"Map unavailable", "The active set does not contain a supported Xbox cache for "+map+". Import a set containing it with Game files & versions. The directory does not advertise content fingerprints or Original/Rev1/Rev2 requirements, so revision compatibility cannot be guessed.");return;}
            String[] labels=new String[matches.size()];for(int i=0;i<labels.length;i++)labels[i]=matches.get(i).description();
            new GamepadNavigation.Builder(this).setTitle("Choose installed data containing "+map).setItems(labels,(d,index)->{
                try{library.select(matches.get(index).id);passOnHardwareId();buildMenu();join.run();}catch(Exception e){LauncherHelp.page(this,"Could not select data",e.getMessage());}
            }).setNegativeButton("Cancel",null).show();
        }catch(Exception e){LauncherHelp.page(this,"Game data",e.getMessage());}
    }

    // ---------- the menu: play, the mods, the settings

    private static final int HALO_BLUE = Color.rgb(88, 160, 220);
    private static final int PANEL = Color.rgb(22, 30, 38);

    private ModInstaller mods;
    private ModInstaller.Mod selectedMod;
    private LinearLayout modDetails;
    private TextView modState;
    private Button play, modAction, modDelete, modResources, resetSettings, mainMenu3d, fontToggle;
    private boolean busy;

    private Button menuButton(LinearLayout parent, String text) {
        Button button = new Button(this);
        LinearLayout.LayoutParams layout = new LinearLayout.LayoutParams(LinearLayout.LayoutParams.MATCH_PARENT,
            LinearLayout.LayoutParams.WRAP_CONTENT);

        button.setText(text);
        button.setAllCaps(true);
        LauncherTheme.button(button);
        layout.topMargin = dp(6);
        parent.addView(button, layout);
        return button;
    }

    private TextView label(LinearLayout parent, String text, float size, int color) {
        TextView view = new TextView(this);

        view.setText(text);
        view.setTextSize(TypedValue.COMPLEX_UNIT_SP, size);
        view.setTextColor(color);
        view.setGravity(Gravity.CENTER);
        parent.addView(view);
        return view;
    }

    private Button addFontToggle(LinearLayout parent) {
        fontToggle = menuButton(parent, LauncherFont.toggleLabel(this));
        fontToggle.setOnClickListener(v -> {
            if (busy)
                return;
            LauncherFont.toggle(this);
            fontToggle.setText(LauncherFont.toggleLabel(this));
            if (launcherRoot != null)
                LauncherFont.apply(this, launcherRoot);
        });
        return fontToggle;
    }

    private void buildMenu() {
        android.widget.ScrollView scroll = new android.widget.ScrollView(this);
        LinearLayout layout = new LinearLayout(this);

        mods = new ModInstaller(gameRoot());
        scroll.setBackground(new LauncherTheme());
        scroll.setFillViewport(true);
        layout.setOrientation(LinearLayout.VERTICAL);
        layout.setGravity(Gravity.CENTER_HORIZONTAL);
        layout.setPadding(dp(24), dp(24), dp(24), dp(24));
        scroll.addView(layout);

        TextView title = label(layout, "HALO: COMBAT EVOLVED", 30, Color.WHITE);
        title.setTypeface(android.graphics.Typeface.create("sans-serif-light", android.graphics.Typeface.NORMAL));
        title.setLetterSpacing(.14f);
        label(layout, BuildConfig.APPLICATION_ID.endsWith(".vr") ? "QUEST • VIRTUAL REALITY" : "ANDROID • TOUCH & GAMEPAD", 14, HALO_BLUE);

        label(layout, BuildConfig.VERSION_NAME + " • community release", 12, HALO_BLUE);
        updateStatus = Updater.launcher(this, gameRoot(), layout);
        LauncherFont.keepNormal(updateStatus);
        play = menuButton(layout, "Play");
        play.setOnClickListener(v -> startFromMenu());

        label(layout, "Community directory fallback · live reported populations · compatible versions only can join",
            13, Color.rgb(150, 160, 170));
        menuButton(layout, "Browse multiplayer servers").setOnClickListener(v -> openServerBrowser(false));
        menuButton(layout, "Browse campaign co-op servers").setOnClickListener(v -> openServerBrowser(true));

        label(layout, LauncherHelp.RECOMMENDED_ISO_NOTE, 14, Color.rgb(255, 190, 70));
        label(layout, LauncherHelp.SUPPORT_NOTE, 13, Color.rgb(150, 190, 210));
        addFontToggle(layout);

        label(layout, "Browse and host games in Play > Multiplayer. Use the guide below for online co-op, public servers, LAN and invites.",
            13, Color.rgb(150, 160, 170));
        menuButton(layout, "Multiplayer & co-op guide").setOnClickListener(v -> LauncherHelp.network(this));
        menuButton(layout, "Field guide • controls & help").setOnClickListener(v -> LauncherHelp.show(this));

        TextView modsTitle = label(layout, "MODS", 18, HALO_BLUE);
        modsTitle.setPadding(0, dp(24), 0, dp(4));

        // SPV1's maps fit the current CE reader's file-size bounds, but this mod
        // has not yet been verified in a real campaign on this port.
        Button spv1 = menuButton(layout, ModInstaller.SPV1.title + " (experimental; unverified)");
        spv1.setOnClickListener(v -> selectMod(ModInstaller.SPV1));

        // the selected mod: what it is, and what can be done with it
        modDetails = new LinearLayout(this);
        modDetails.setOrientation(LinearLayout.VERTICAL);
        modDetails.setGravity(Gravity.CENTER_HORIZONTAL);
        modDetails.setBackgroundColor(PANEL);
        modDetails.setPadding(dp(20), dp(14), dp(20), dp(14));
        modDetails.setVisibility(View.GONE);
        LinearLayout.LayoutParams detailsLayout = new LinearLayout.LayoutParams(LinearLayout.LayoutParams.MATCH_PARENT,
            LinearLayout.LayoutParams.WRAP_CONTENT);
        detailsLayout.topMargin = dp(8);
        layout.addView(modDetails, detailsLayout);

        TextView settingsTitle = label(layout, "SETTINGS", 18, HALO_BLUE);
        settingsTitle.setPadding(0, dp(24), 0, dp(4));
        if (BuildConfig.APPLICATION_ID.endsWith(".vr")) {
            mainMenu3d = menuButton(layout, "");
            mainMenu3d.setOnClickListener(v -> toggleMainMenu3d());
            refreshMainMenu3d();
        }
        menuButton(layout, "Versions & updates").setOnClickListener(v -> { if (!busy) Updater.show(this, gameRoot()); });
        menuButton(layout, "Network settings").setOnClickListener(v -> { if (!busy) NetworkSettings.show(this, gameRoot()); });
        if (!BuildConfig.APPLICATION_ID.endsWith(".vr"))
            menuButton(layout, "Controller & touch settings").setOnClickListener(v -> { if (!busy) GamepadSupport.show(this); });
        menuButton(layout, "Game files & versions").setOnClickListener(v -> {if(!busy)dataManager().show();});
        menuButton(layout, "Game data & compatibility").setOnClickListener(v -> { if (!busy) LauncherHelp.data(this, gameRoot()); });
        menuButton(layout, "Geometry compatibility").setOnClickListener(v -> { if (!busy) LauncherHelp.graphics(this, gameRoot()); });
        resetSettings = menuButton(layout, "Reset settings to defaults");
        resetSettings.setOnClickListener(v -> resetSettings());
        label(layout, BuildConfig.APPLICATION_ID.endsWith(".vr")
            ? "Settings are changed in the game: pause, then VR SETTINGS."
            : "Controller & touch settings controls automatic HUD hiding, gamepad response and gyro aim (off by default). In-game Options retains layout, sensitivity, invert and gyro aim.",
            13, Color.rgb(150, 160, 170));
        label(layout, "Something wrong? Each run's log is in this headset's Download folder, Download/HaloCE, "
            + "named by when the game started (halo_log_<date>_<time>.txt; the newest is the last run; the "
            + "last ten are kept). A copy of the last run's is in Android/data/" + BuildConfig.APPLICATION_ID
            + "/files/halo_log.txt. Send it with a few words on what happened.", 13, Color.rgb(150, 160, 170));

        progress = new ProgressBar(this, null, android.R.attr.progressBarStyleHorizontal);
        progress.setMax(1000);
        progress.setVisibility(View.GONE);
        LinearLayout.LayoutParams progressLayout = new LinearLayout.LayoutParams(LinearLayout.LayoutParams.MATCH_PARENT,
            LinearLayout.LayoutParams.WRAP_CONTENT);
        progressLayout.topMargin = dp(16);
        layout.addView(progress, progressLayout);

        status = label(layout, "", 14, Color.rgb(160, 200, 160));
        LauncherFont.keepNormal(status);
        label(layout, "This launch's log: " + RunLog.destination(), 13, Color.rgb(160, 200, 160));
        status.setPadding(0, dp(8), 0, 0);

        LauncherFont.apply(this, scroll);
        launcherRoot = scroll;
        setContentView(scroll);
        play.requestFocus();
    }

    /** Shows the mod's description and buttons (again: hides them). */
    static final String SPV1_COMPATIBILITY_NOTE = "SPV1 is experimental and has not been verified in a campaign "
        + "on this port. Its maps are about 157-253 MiB each, larger than the 128 MiB limit sometimes quoted. "
        + "This Custom Edition reader accepts map files up to 384 MiB (576 MiB for OpenSauce-upgraded caches), "
        + "so size alone does not rule SPV1 out; protected-map conversion, resources and gameplay still need "
        + "testing. Keep a backup; Restore returns the original campaign.";

    private void selectMod(ModInstaller.Mod mod) {
        if (selectedMod == mod && modDetails.getVisibility() == View.VISIBLE) {
            modDetails.setVisibility(View.GONE);
            return;
        }
        selectedMod = mod;
        modDetails.removeAllViews();

        TextView name = new TextView(this);
        name.setText(mod.title);
        name.setTextColor(Color.WHITE);
        name.setTextSize(TypedValue.COMPLEX_UNIT_SP, 18);
        modDetails.addView(name);

        TextView warning = new TextView(this);
        warning.setText(SPV1_COMPATIBILITY_NOTE);
        warning.setTextColor(Color.rgb(255, 190, 70));
        warning.setTextSize(TypedValue.COMPLEX_UNIT_SP, 15);
        warning.setPadding(0, dp(8), 0, 0);
        modDetails.addView(warning);

        TextView description = new TextView(this);
        description.setText(mod.description);
        description.setTextColor(Color.rgb(200, 205, 210));
        description.setTextSize(TypedValue.COMPLEX_UNIT_SP, 14);
        description.setPadding(0, dp(8), 0, dp(8));
        modDetails.addView(description);

        modState = new TextView(this);
        LauncherFont.keepNormal(modState);
        modState.setTextColor(HALO_BLUE);
        modState.setTextSize(TypedValue.COMPLEX_UNIT_SP, 14);
        modDetails.addView(modState);

        modResources = menuButton(modDetails, "Choose Custom Edition's bitmaps.map, sounds.map, loc.map");
        modResources.setOnClickListener(v -> {
            // all three at once, from wherever they were copied to
            Intent intent = new Intent(Intent.ACTION_OPEN_DOCUMENT);
            intent.addCategory(Intent.CATEGORY_OPENABLE);
            intent.setType("*/*");
            intent.putExtra(Intent.EXTRA_ALLOW_MULTIPLE, true);
            startActivityForResult(intent, PICK_RESOURCES);
        });
        modAction = menuButton(modDetails, "");
        modAction.setOnClickListener(v -> modActionClicked());
        modDelete = menuButton(modDetails, "Delete the download (frees 2.2 GB)");
        modDelete.setOnClickListener(v -> modDeleteClicked());
        refreshMod();
        modDetails.setVisibility(View.VISIBLE);
    }

    private void refreshMod() {
        if (selectedMod == null || modAction == null)
            return;
        boolean installed = mods.installed(selectedMod);
        boolean downloaded = mods.downloaded(selectedMod);
        boolean resources = mods.haveResources();

        modResources.setVisibility(resources ? View.GONE : View.VISIBLE);
        modResources.setEnabled(!busy);
        modState.setText((resources ? "" : "BEFORE INSTALLING: SPV1 needs three files from Halo Custom Edition "
                + "for PC (a free download for owners of Halo PC). They come with the game, so they cannot come "
                + "with this app; you only do this once.\n"
                + "1. On your computer, open Halo Custom Edition's folder (often C:\\Program Files (x86)\\"
                + "Microsoft Games\\Halo Custom Edition), then its maps folder.\n"
                + "2. Find bitmaps.map, sounds.map and loc.map. They must be the original ones: if a mod "
                + "(Halo Refined, a VR mod...) changed that folder, use its backup instead (often a folder "
                + "named MAPS_original). The original bitmaps.map is " + ModInstaller.megabytes(ModInstaller.RESOURCE_SIZES[0])
                + ", sounds.map " + ModInstaller.megabytes(ModInstaller.RESOURCE_SIZES[1]) + ".\n"
                + "3. Connect this headset to the computer with a USB cable (allow access in the headset), and "
                + "copy the three files into its Download folder.\n"
                + "4. Tap \"Choose Custom Edition's ...\" below, open Download, and select all three "
                + "(press and hold one, then tap the others).\n"
                + "(Or copy them with SideQuest straight into " + mods.resourcePath() + ".)\n"
                + "Still needed: " + mods.missingResources() + "\n\n")
            + (installed ? "Installed: the campaign plays as " + selectedMod.title + "."
            : downloaded ? "Downloaded, not installed: the original campaign plays."
            : "Not installed: the original campaign plays.")
            + "\nAfter switching, start levels anew: a checkpoint of one campaign does not load into the other.");
        modAction.setText(installed ? "Restore the original campaign"
            : downloaded ? "Install (already downloaded)" : "Download and install");
        modDelete.setText(installed ? "Uninstall and restore original campaign" : "Delete downloaded mod maps");
        modDelete.setVisibility(installed || downloaded || mods.hasDownloadFiles(selectedMod) ? View.VISIBLE : View.GONE);
        modAction.setEnabled(!busy && (installed || resources));
        modDelete.setEnabled(!busy);
    }

    private void setBusy(boolean value) {
        busy = value;
        play.setEnabled(!value);
        resetSettings.setEnabled(!value);
        if (fontToggle != null)
            fontToggle.setEnabled(!value);
        progress.setVisibility(value ? View.VISIBLE : View.GONE);
        refreshMod();
    }

    private void modActionClicked() {
        ModInstaller.Mod mod = selectedMod;
        boolean installed = mods.installed(mod);

        setBusy(true);
        new Thread(() -> {
            try {
                if (installed) {
                    mods.restore(mod, this::report);
                    report("The original campaign is back.", 1000);
                } else {
                    mods.install(mod, this::report);
                    report(mod.title + " is installed: play the campaign.", 1000);
                }
            } catch (Exception exception) {
                report((installed ? "Restoring" : "Installing") + " failed: " + exception.getMessage()
                    + (installed ? "" : " (the download resumes from the map it stopped at)"), -1);
            }
            handler.post(() -> setBusy(false));
        }).start();
    }

    private void modDeleteClicked() {
        ModInstaller.Mod mod = selectedMod;
        setBusy(true);
        new Thread(() -> {
            try {
                mods.delete(mod, this::report);
            } catch (Exception exception) {
                report("Uninstall failed: " + exception.getMessage(), -1);
            }
            handler.post(() -> setBusy(false));
        }, "mod-uninstall").start();
    }

    /**
     * The main menu in 3D around the player (the default), or on a flat
     * screen: the second leaves vr_menu_flat.on in the game's folder, which
     * the native side reads at start (port/android/host/host_main.c).
     */
    private File mainMenuFlatMarker() {
        return new File(gameRoot(), "vr_menu_flat.on");
    }

    private void refreshMainMenu3d() {
        mainMenu3d.setText(mainMenuFlatMarker().exists()
            ? "Main menu: on a flat screen (tap for 3D)"
            : "Main menu: in 3D around you (tap for a flat screen)");
    }

    private void toggleMainMenu3d() {
        File marker = mainMenuFlatMarker();
        boolean changed;

        if (marker.exists()) {
            changed = marker.delete();
        } else {
            try {
                changed = marker.createNewFile();
            } catch (java.io.IOException e) {
                changed = false;
            }
        }
        refreshMainMenu3d();
        if (!changed)
            status.setText("The main menu setting could not be changed.");
    }

    /**
     * Every setting back to its default: the game writes a new config.toml
     * with them the next time it starts. The old one is kept beside it.
     */
    private void resetSettings() {
        File config = new File(gameRoot(), "config.toml");
        File old = new File(gameRoot(), "config.toml.old");

        if (!config.exists()) {
            status.setText("The settings are already the defaults.");
            return;
        }
        old.delete();
        status.setText(config.renameTo(old)
            ? "Settings reset: the game starts with the defaults (the old ones are in config.toml.old)."
            : "The settings could not be reset.");
    }

    private int dp(float value) {
        return (int) TypedValue.applyDimension(TypedValue.COMPLEX_UNIT_DIP, value,
            getResources().getDisplayMetrics());
    }

    private void buildInterface() {
        LinearLayout layout = new LinearLayout(this);
        layout.setOrientation(LinearLayout.VERTICAL);
        layout.setGravity(Gravity.CENTER);
        layout.setPadding(dp(24), dp(24), dp(24), dp(24));
        layout.setBackgroundColor(Color.rgb(12, 16, 20));

        TextView title = new TextView(this);
        title.setText("Halo needs its game data");
        title.setTextColor(Color.WHITE);
        title.setTextSize(TypedValue.COMPLEX_UNIT_SP, 24);
        title.setGravity(Gravity.CENTER);
        layout.addView(title);

        TextView message = new TextView(this);
        menuButton(layout, "Game files & versions").setOnClickListener(v -> dataManager().show());
        message.setText("Choose an Xbox disc image of Halo: Combat Evolved (an .iso or .xiso file) "
            + "on this device. Its maps folder is copied into the app's storage (about 1.8 GB), "
            + "and you can delete the image afterwards.\n\n"
            + "You can also copy a maps folder from a computer:\n"
            + "adb push <folder with maps>/. " + (dataRoot != null ? dataRoot.getAbsolutePath() : "") + "/");
        message.setTextColor(Color.rgb(200, 205, 210));
        message.setTextSize(TypedValue.COMPLEX_UNIT_SP, 15);
        message.setGravity(Gravity.CENTER);
        message.setPadding(0, dp(16), 0, dp(16));
        layout.addView(message);

        TextView recommendation = new TextView(this);
        recommendation.setText(LauncherHelp.RECOMMENDED_ISO_NOTE);
        recommendation.setTextColor(Color.rgb(255, 190, 70));
        recommendation.setTextSize(TypedValue.COMPLEX_UNIT_SP, 15);
        recommendation.setGravity(Gravity.CENTER);
        recommendation.setPadding(0, 0, 0, dp(12));
        layout.addView(recommendation);

        addFontToggle(layout);
        TextView support = new TextView(this);
        support.setText(LauncherHelp.SUPPORT_NOTE);
        support.setTextColor(Color.rgb(150, 190, 210));
        support.setTextSize(TypedValue.COMPLEX_UNIT_SP, 14);
        support.setGravity(Gravity.CENTER);
        support.setPadding(0, 0, 0, dp(12));
        layout.addView(support);

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
        LinearLayout.LayoutParams progressLayout = new LinearLayout.LayoutParams(LinearLayout.LayoutParams.MATCH_PARENT,
            LinearLayout.LayoutParams.WRAP_CONTENT);
        progressLayout.topMargin = dp(16);
        layout.addView(progress, progressLayout);

        status = new TextView(this);
        LauncherFont.keepNormal(status);
        status.setTextColor(Color.rgb(160, 200, 160));
        status.setGravity(Gravity.CENTER);
        status.setPadding(0, dp(8), 0, 0);
        layout.addView(status);

        LauncherTheme.button(pick);
        updateStatus = Updater.launcher(this, gameRoot(), layout);
        LauncherFont.keepNormal(updateStatus);
        menuButton(layout, "Getting started & controls").setOnClickListener(v -> LauncherHelp.show(this));
        android.widget.ScrollView importScroll = new android.widget.ScrollView(this);
        importScroll.setFillViewport(true); importScroll.setBackground(new LauncherTheme());
        layout.setBackgroundColor(Color.TRANSPARENT); importScroll.addView(layout);
        LauncherFont.apply(this, importScroll);
        launcherRoot = importScroll;
        setContentView(importScroll);
        pick.requestFocus();
    }

    @Override
    protected void onResume() {
        super.onResume();
        Updater.resume(this, gameRoot(), updateStatus);
        // data pushed with adb while the import screen was open
        if (pick != null && pick.isEnabled() && play == null && haveData())
            buildMenu();
    }

    /** Copies the chosen files that are Custom Edition resource maps into the mods' keeping. */
    private void importResources(java.util.List<Uri> files) {
        int copied = 0;
        StringBuilder refused = new StringBuilder();

        for (Uri file : files) {
            try (java.io.InputStream in = getContentResolver().openInputStream(file)) {
                if (in == null)
                    throw new java.io.IOException("cannot open it");
                java.io.BufferedInputStream buffered = new java.io.BufferedInputStream(in, 1 << 16);
                byte[] header = new byte[4];

                buffered.mark(8);
                if (buffered.read(header) != 4 || ModInstaller.resourceType(header) == 0) {
                    refused.append(refused.length() > 0 ? "; " : "").append(file.getLastPathSegment())
                        .append(" is not bitmaps.map, sounds.map or loc.map");
                    continue;
                }
                buffered.reset();
                int type = ModInstaller.resourceType(header);
                mods.resourceDirectoryMade();
                File target = mods.resourceFile(type);
                File partial = new File(target.getPath() + ".part");
                try (OutputStream out = new FileOutputStream(partial)) {
                    byte[] buffer = new byte[1 << 16];
                    long total = 0;
                    int read;

                    while ((read = buffered.read(buffer)) > 0) {
                        out.write(buffer, 0, read);
                        total += read;
                        if ((total & ((8 << 20) - 1)) < read)
                            report("Copying " + ModInstaller.RESOURCE_NAMES[type - 1] + ".map (" + (total >> 20)
                                + " MB)", -1);
                    }
                }
                if (!ModInstaller.resourceOriginal(type, partial.length())) {
                    String name = ModInstaller.RESOURCE_NAMES[type - 1] + ".map";

                    refused.append(refused.length() > 0 ? "; " : "").append(name).append(" is ")
                        .append(ModInstaller.megabytes(partial.length())).append(", but the original is ")
                        .append(ModInstaller.megabytes(ModInstaller.RESOURCE_SIZES[type - 1]))
                        .append(": it was changed by a mod (Halo Refined, Chimera's tools...). Use the one from ")
                        .append("an unmodded install, or the backup such mods leave (MAPS_original)");
                    partial.delete();
                    continue;
                }
                target.delete();
                if (!partial.renameTo(target))
                    throw new java.io.IOException("cannot keep " + target);
                copied++;
            } catch (Exception exception) {
                refused.append(refused.length() > 0 ? ", " : "").append(file.getLastPathSegment())
                    .append(" (").append(exception.getMessage()).append(")");
            }
        }
        String summary = (copied > 0 ? copied + " file" + (copied == 1 ? "" : "s") + " taken." : "No file taken.")
            + (refused.length() > 0 ? " Not taken: " + refused + "." : "")
            + (mods.haveResources() ? " Ready: tap \"Download and install\"."
                : " Still needed: " + mods.missingResources() + ".");
        report(summary, -1);
        handler.post(() -> setBusy(false));
    }

    @Override
    protected void onActivityResult(int requestCode, int resultCode, Intent data) {
        super.onActivityResult(requestCode, resultCode, data);
        if((requestCode==GameDataManager.PICK_ISO||requestCode==GameDataManager.PICK_FOLDER)&&dataManager().result(requestCode,resultCode,data))return;
        if (requestCode == PICK_RESOURCES && resultCode == RESULT_OK && data != null) {
            java.util.List<Uri> files = new java.util.ArrayList<>();

            if (data.getClipData() != null) {
                for (int index = 0; index < data.getClipData().getItemCount(); index++)
                    files.add(data.getClipData().getItemAt(index).getUri());
            } else if (data.getData() != null) {
                files.add(data.getData());
            }
            setBusy(true);
            new Thread(() -> importResources(files)).start();
            return;
        }
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
                    buildMenu();
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
