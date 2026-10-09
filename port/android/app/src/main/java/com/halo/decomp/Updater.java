package com.halo.decomp;

import android.app.Activity;
import android.app.AlertDialog;
import android.content.Intent;
import android.content.pm.PackageInfo;
import android.content.pm.PackageManager;
import android.content.pm.Signature;
import android.net.Uri;
import android.widget.TextView;
import org.json.JSONArray;
import org.json.JSONObject;
import java.io.*;
import java.net.HttpURLConnection;
import java.net.URL;
import java.nio.charset.StandardCharsets;
import java.security.MessageDigest;
import java.util.Arrays;
import java.util.concurrent.atomic.AtomicBoolean;

/** Updates the integrated VR/flat project, never substitutes an upstream engine. */
final class Updater {
    private static final long MAX_APK=128L*1024*1024;
    private static final AtomicBoolean busy=new AtomicBoolean();
    private static final long CHECK_INTERVAL=6L*60*60*1000;
    private static android.content.SharedPreferences prefs(Activity a){return a.getSharedPreferences("project-updates",0);}
    static void start(Activity activity) { /* Gameplay never updates its loaded engine. */ }
    static TextView launcher(Activity a,File root,android.widget.LinearLayout parent) {
        TextView status=new TextView(a);status.setTextColor(0xff99d9ff);status.setTextSize(14);status.setPadding(8,12,8,12);
        status.setText(prefs(a).getString("status","Checking compatible updates...")+"\nTap for updates and version details.");
        status.setOnClickListener(v->show(a,root));parent.addView(status);
        resume(a,root,status);
        return status;
    }
    static void resume(Activity a,File root,TextView status) {
        if(status==null)return;
        long now=System.currentTimeMillis(),last=prefs(a).getLong("last_check",0);
        if(prefs(a).getBoolean("automatic",true) && (last>now || now-last>=CHECK_INTERVAL))check(a,root,true,status);
        else if(!prefs(a).getBoolean("automatic",true))status.setText("Automatic update checks off. Tap to check now.");
    }
    static void show(Activity a,File gameRoot) {
        android.widget.LinearLayout box=new android.widget.LinearLayout(a);box.setOrientation(1);box.setPadding(28,16,28,16);
        TextView text=new TextView(a);text.setText(prefs(a).getString("status","No update check yet.")+"\n\nInstalled: "+BuildConfig.VERSION_NAME+" ("+BuildConfig.VERSION_CODE+")\n"+BuildConfig.APPLICATION_ID+
            "\nJoin target: "+NetworkProfile.label(NetworkProfile.selected(a))+"; hosting: network 24. Legacy 11–22 targets support Xbox-map PvP; co-op and Custom Edition use 23–24."+
            "\nCo-op: OpenCE's, up to 128 players, with OpenCE players too\n\nThe launcher checks for compatible project releases and upstream network changes. A project update includes the integrated engine, VR, co-op and launcher together. New upstream changes require integration before they can update this mod safely. Upstream netcode: OpenCE build 157 (network 24), with its co-op, CE map checksum matching, signed public discovery, Custom Edition fixes, PC vehicle set, analog trigger pressure, and spatialized world stereo audio.\n\nDownload verifies compatibility metadata, SHA-256, package, newer Android version and the current signing certificate. The current APK, active configuration and touch preferences are backed up privately before Android opens its installer. Game maps and saves are preserved. Android asks you to approve installation; rollback may require ADB. Test candidates are never offered as public updates.");box.addView(text);
        android.widget.CheckBox automatic=new android.widget.CheckBox(a);automatic.setText("Check automatically when the launcher opens (every 6 hours)");automatic.setChecked(prefs(a).getBoolean("automatic",true));box.addView(automatic);
        automatic.setOnCheckedChangeListener((b,v)->prefs(a).edit().putBoolean("automatic",v).apply());
        TextView privacy=new TextView(a);privacy.setText("Checks contact GitHub for release metadata and the upstream protocol header. No saves, logs, invites or configuration are uploaded. Offline play is always available.");box.addView(privacy);
        android.widget.ScrollView scroll=new android.widget.ScrollView(a);scroll.addView(box);
        new GamepadNavigation.Builder(a).setTitle("Versions & compatible updates").setView(scroll)
            .setPositiveButton("Check now",(d,w)->check(a,gameRoot,false,null)).setNeutralButton("Official upstream",(d,w)->UpstreamDownloads.show(a)).setNegativeButton("Close",null).show();
    }
    private static void status(Activity a,TextView view,String text) {
        prefs(a).edit().putString("status",text).apply();
        RunLog.line("Project update: "+text);
        if(view!=null)a.runOnUiThread(()->{if(!a.isFinishing()&&!a.isDestroyed())view.setText(text+"\nTap for updates and version details.");});
    }
    private static String upstreamStatus() {
        try {
            JSONObject header=new JSONObject(new String(fetch("https://api.github.com/repos/cybersecurity/halo-ce-universal/contents/port/linux/include/halo_port_limits.h",128*1024),StandardCharsets.UTF_8));
            if(!"base64".equals(header.optString("encoding")))return "Upstream status unavailable.";
            String source=new String(android.util.Base64.decode(header.getString("content"),android.util.Base64.DEFAULT),StandardCharsets.UTF_8);
            int version=UpdatePolicy.upstreamVersion(source);
            if(version<1)return "Upstream protocol format needs review.";
            return NetworkProfile.supported(version) ? "Upstream network v"+version+" is available in the launcher network selector." : "Upstream network v"+version+" needs a matching project implementation. Compatible servers remain playable.";
        }catch(Exception e){return "Upstream status unavailable (offline or service limit).";}
    }
    private static void message(Activity a,String text) {a.runOnUiThread(()->{if(!a.isFinishing()&&!a.isDestroyed())new GamepadNavigation.Builder(a).setTitle("Project update").setMessage(text).setPositiveButton("OK",null).show();});}
    static HttpURLConnection open(String address) throws Exception {
        for(int i=0;i<6;i++) {
            if(!UpdatePolicy.allowedUrl(address))throw new IOException("Untrusted update URL");
            HttpURLConnection c=(HttpURLConnection)new URL(address).openConnection();
            c.setConnectTimeout(20000);c.setReadTimeout(20000);c.setInstanceFollowRedirects(false);
            c.setRequestProperty("User-Agent","HaloCE-Quest-project-updater");c.setRequestProperty("Accept","application/vnd.github+json");
            int status=c.getResponseCode();
            if(status>=300&&status<400){String next=c.getHeaderField("Location");c.disconnect();if(next==null)throw new IOException("Missing redirect");address=new URL(new URL(address),next).toString();continue;}
            if(status!=200){c.disconnect();throw new IOException("Update service returned HTTP "+status);}
            return c;
        }
        throw new IOException("Too many redirects");
    }
    static byte[] fetch(String url,int limit) throws Exception {
        HttpURLConnection c=open(url);
        try(InputStream in=c.getInputStream();ByteArrayOutputStream out=new ByteArrayOutputStream()) {
            byte[] b=new byte[16384];int n;
            while((n=in.read(b))!=-1){if(out.size()+n>limit)throw new IOException("Update metadata too large");out.write(b,0,n);}return out.toByteArray();
        } finally{c.disconnect();}
    }
    private static void check(Activity a,File gameRoot,boolean silent,TextView statusView) {
        if(!busy.compareAndSet(false,true)){if(!silent)message(a,"An update operation is already running.");return;}
        prefs(a).edit().putLong("last_check",System.currentTimeMillis()).apply();
        if(!silent)android.widget.Toast.makeText(a,"Checking compatible project and network updates...",android.widget.Toast.LENGTH_SHORT).show();
        new Thread(()->{
            try {
                String upstream=upstreamStatus();
                JSONObject release=new JSONObject(new String(fetch("https://api.github.com/repos/"+UpdatePolicy.REPOSITORY+"/releases/latest",1024*1024),StandardCharsets.UTF_8));
                String tag=release.optString("tag_name"),name=UpdatePolicy.asset(tag,BuildConfig.APPLICATION_ID.endsWith(".vr"));
                if(release.optBoolean("draft")||release.optBoolean("prerelease")||name==null)throw new IOException("No recognized stable project release");
                JSONArray assets=release.getJSONArray("assets");JSONObject found=null,manifestAsset=null;
                for(int i=0;i<assets.length();i++){JSONObject item=assets.getJSONObject(i);if("compatibility.json".equals(item.optString("name"))){if(manifestAsset!=null)throw new IOException("Duplicate compatibility metadata");manifestAsset=item;}if(name.equals(item.optString("name"))){if(found!=null)throw new IOException("Duplicate release asset");found=item;}}
                if(found==null)throw new IOException("The release has no APK for this edition");
                final String url=found.getString("browser_download_url"),digest=found.optString("digest");final long size=found.optLong("size");
                if(!UpdatePolicy.digest(digest)||!UpdatePolicy.allowedUrl(url)||size<=0||size>MAX_APK)throw new IOException("Release APK lacks valid integrity metadata");
                if(manifestAsset==null)throw new IOException("New release needs project compatibility metadata before it can be installed here");
                String manifestDigest=manifestAsset.optString("digest");
                byte[] manifestBytes=fetch(manifestAsset.getString("browser_download_url"),65536);
                if(!UpdatePolicy.digest(manifestDigest)||!manifestDigest.equalsIgnoreCase("sha256:"+hex(MessageDigest.getInstance("SHA-256").digest(manifestBytes))))throw new IOException("Compatibility metadata checksum mismatch");
                JSONObject compatibility=new JSONObject(new String(manifestBytes,StandardCharsets.UTF_8));
                if(compatibility.optInt("schema")!=1||!UpdatePolicy.REPOSITORY.equals(compatibility.optString("project"))||!tag.equals(compatibility.optString("tag"))||
                    !"preserve".equals(compatibility.optString("save_policy"))||!"preserve".equals(compatibility.optString("config_policy"))||
                    !compatibility.optBoolean("vr_and_coop_integrated")||compatibility.optInt("minimum_app_code",Integer.MAX_VALUE)>BuildConfig.VERSION_CODE)
                    throw new IOException("This update needs a reviewed migration; your current installation is preserved");
                JSONObject edition=compatibility.getJSONObject("editions").getJSONObject(BuildConfig.APPLICATION_ID);
                if(!name.equals(edition.optString("apk"))||!digest.substring(7).equalsIgnoreCase(edition.optString("sha256"))||edition.optLong("bytes")!=size||
                    edition.optInt("version_code")<=0||edition.optInt("min_sdk",Integer.MAX_VALUE)>android.os.Build.VERSION.SDK_INT)throw new IOException("Release edition metadata is inconsistent");
                final int expectedVersion=edition.getInt("version_code");
                if(!UpdatePolicy.newerCode(expectedVersion,BuildConfig.VERSION_CODE)) {
                    String text=(expectedVersion==BuildConfig.VERSION_CODE?"You already have this published build.":"Your installed build is newer than the public release.")+" Installed "+BuildConfig.VERSION_NAME+" (code "+BuildConfig.VERSION_CODE+"); public "+tag+" (code "+expectedVersion+"). "+upstream;
                    status(a,statusView,text);if(!silent)message(a,text);return;
                }
                int minimum=compatibility.optInt("native_minimum"),maximum=compatibility.optInt("native_maximum");
                if(minimum<1||maximum<minimum||maximum>65535)throw new IOException("Invalid network compatibility range");
                String available="Compatible update available: "+tag+" (native v"+minimum+"-"+maximum+"). Tap to check and install. "+upstream;
                status(a,statusView,available);
                if(silent)return;
                a.runOnUiThread(()->{if(!a.isFinishing()&&!a.isDestroyed())new GamepadNavigation.Builder(a).setTitle("Update to "+tag+"?").setMessage("Download "+name+" ("+(size/1048576)+" MiB), validate it, back up the installed app and settings, then open Android's installer. Maps and saves are not replaced.")
                    .setPositiveButton("Download",(d,w)->download(a,gameRoot,url,digest,size,expectedVersion)).setNegativeButton("Later",null).show();});
            }catch(Exception e){prefs(a).edit().putLong("last_check",System.currentTimeMillis()-CHECK_INTERVAL+15L*60*1000).apply();String text="Update check: "+e.getMessage()+". Current game remains available.";status(a,statusView,text);if(!silent)message(a,text);}
            finally{busy.set(false);}
        },"project-update-check").start();
    }
    static String hex(byte[] bytes){StringBuilder b=new StringBuilder();for(byte v:bytes)b.append(String.format(java.util.Locale.ROOT,"%02x",v&255));return b.toString();}
    private static void copy(File source,File target,long limit) throws IOException {
        File temporary=new File(target.getParentFile(),target.getName()+".tmp");
        try(InputStream in=new FileInputStream(source);FileOutputStream out=new FileOutputStream(temporary)){
            byte[] b=new byte[65536];long total=0;int n;while((n=in.read(b))!=-1){total+=n;if(total>limit)throw new IOException("Backup exceeds limit");out.write(b,0,n);}out.getFD().sync();
        }
        if(!temporary.renameTo(target))throw new IOException("Could not finalize backup");
    }
    private static void download(Activity a,File gameRoot,String url,String expected,long size,int expectedVersion) {
        if(!busy.compareAndSet(false,true))return;
        TextView status=new TextView(a);status.setPadding(32,24,32,24);status.setText("Downloading and verifying update...");
        AtomicBoolean cancel=new AtomicBoolean();AlertDialog dialog=new GamepadNavigation.Builder(a).setTitle("Project update").setView(status).setNegativeButton("Cancel",(d,w)->cancel.set(true)).create();
        dialog.setOnCancelListener(d->cancel.set(true));dialog.show();
        new Thread(()->{
            File directory=new File(a.getCacheDir(),UpdateProvider.DIRECTORY),part=new File(directory,"download.part"),apk=new File(directory,UpdateProvider.APK);
            try {
                if(!directory.isDirectory()&&!directory.mkdirs())throw new IOException("Cannot create download folder");
                HttpURLConnection c=open(url);MessageDigest hash=MessageDigest.getInstance("SHA-256");long count=0;
                try(InputStream in=c.getInputStream();FileOutputStream out=new FileOutputStream(part)){
                    byte[] b=new byte[65536];int n;while((n=in.read(b))!=-1){if(cancel.get())throw new IOException("Cancelled");count+=n;if(count>size||count>MAX_APK)throw new IOException("APK size mismatch");out.write(b,0,n);hash.update(b,0,n);}out.getFD().sync();
                }finally{c.disconnect();}
                if(count!=size||!expected.equalsIgnoreCase("sha256:"+hex(hash.digest())))throw new IOException("APK checksum mismatch");
                PackageManager pm=a.getPackageManager();PackageInfo current=pm.getPackageInfo(a.getPackageName(),PackageManager.GET_SIGNING_CERTIFICATES);
                PackageInfo next=pm.getPackageArchiveInfo(part.getAbsolutePath(),PackageManager.GET_SIGNING_CERTIFICATES);
                if(next==null||!current.packageName.equals(next.packageName)||next.getLongVersionCode()<=current.getLongVersionCode()||next.getLongVersionCode()!=expectedVersion||next.signingInfo==null||current.signingInfo==null)throw new IOException("APK edition or version is incompatible");
                Signature[] before=current.signingInfo.getApkContentsSigners(),after=next.signingInfo.getApkContentsSigners();
                if(before.length!=1||after.length!=1||!Arrays.equals(before[0].toByteArray(),after[0].toByteArray()))throw new IOException("APK signing certificate does not match this installation");
                try(java.util.zip.ZipFile zip=new java.util.zip.ZipFile(part)) {
                    if(zip.getEntry("assets/halo_guest.elf")==null||zip.getEntry("lib/arm64-v8a/libmain.so")==null||
                        (zip.getEntry("lib/arm64-v8a/libopenxr_loader.so")!=null)!=BuildConfig.APPLICATION_ID.endsWith(".vr"))
                        throw new IOException("Update has the wrong native engine or VR edition");
                }
                if(cancel.get())throw new IOException("Cancelled");
                File backup=new File(a.getFilesDir(),"update-backup");if(!backup.isDirectory()&&!backup.mkdirs())throw new IOException("Cannot create backup");
                copy(new File(a.getApplicationInfo().sourceDir),new File(backup,"previous.apk"),MAX_APK);
                File config=gameRoot==null?null:new File(gameRoot,"config.toml");
                if(config!=null&&config.isFile())copy(config,new File(backup,"config.toml"),1024*1024);
                JSONObject prefs=new JSONObject(a.getSharedPreferences("phone-controls",0).getAll());
                try(FileOutputStream out=new FileOutputStream(new File(backup,"touch.json"))){out.write(prefs.toString(2).getBytes(StandardCharsets.UTF_8));out.getFD().sync();}
                if(cancel.get())throw new IOException("Cancelled");
                if(!part.renameTo(apk))throw new IOException("Cannot finalize downloaded APK");
                a.runOnUiThread(()->{if(a.isFinishing()||a.isDestroyed()||cancel.get())return;
                    try{Intent install=new Intent(Intent.ACTION_VIEW).setDataAndType(Uri.parse("content://"+UpdateProvider.AUTHORITY+"/"+UpdateProvider.APK),"application/vnd.android.package-archive").addFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION);a.startActivity(install);}
                    catch(RuntimeException e){message(a,"Android could not open the installer. Allow installs from this app in system settings, then retry. Your installed version is unchanged.");}});
            }catch(Exception e){message(a,"Update not installed: "+e.getMessage());}
            finally{part.delete();busy.set(false);a.runOnUiThread(()->{if(!a.isDestroyed())dialog.dismiss();});}
        },"project-update-download").start();
    }
}
