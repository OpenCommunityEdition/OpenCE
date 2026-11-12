package com.halo.decomp;

import java.io.File;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.io.RandomAccessFile;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.nio.channels.FileChannel;
import java.nio.channels.FileLock;
import java.nio.channels.OverlappingFileLockException;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.LinkOption;
import java.nio.file.Path;
import java.nio.file.StandardCopyOption;
import java.nio.file.StandardOpenOption;
import java.util.HashMap;
import java.util.HashSet;
import java.util.List;
import java.util.Locale;
import java.util.Map;
import java.util.Set;
import java.util.zip.ZipEntry;
import java.util.zip.ZipInputStream;

/* Storage Access Framework URIs are streams, never filesystem paths. Both
 * document trees and ZIPs feed this bounded, platform-independent importer. */
final class MccPackageImporter {
    static final int PACKAGE = 0, SINGLEPLAYER_RESOURCES = 1, MULTIPLAYER_RESOURCES = 2;
    interface Receiver { void file(String path, InputStream stream) throws IOException; }
    interface Source { void copy(Receiver receiver) throws IOException; }
    static final long MAX_TOTAL = 16L << 30;
    static final int MAX_FILES = 4096;

    static Source zip(InputStream input) {
        return receiver -> {
            try (ZipInputStream zip = new ZipInputStream(input)) {
                ZipEntry entry;
                int count = 0;
                while ((entry = zip.getNextEntry()) != null) {
                    if (++count > MAX_FILES) throw new IOException("Too many archive entries");
                    String name = entry.getName();
                    if (entry.isDirectory()) {
                        path(name.endsWith("/") ? name.substring(0, name.length() - 1) : name);
                    } else receiver.file(name, zip);
                    zip.closeEntry();
                }
            }
        };
    }

    static final class Prepared implements AutoCloseable {
        final Path staging, content;
        final String destination, title;
        final int maps;
        final long bytes;
        private final FileChannel lockChannel;
        private final FileLock lock;
        private boolean installed;
        Prepared(Path staging, Path content, String destination, String title, int maps, long bytes, FileChannel lockChannel, FileLock lock) {
            this.staging = staging; this.content = content; this.destination = destination;
            this.title = title; this.maps = maps; this.bytes = bytes;
            this.lockChannel = lockChannel; this.lock = lock;
        }
        boolean exists(File root) throws IOException { return Files.exists(target(root.toPath(), destination), LinkOption.NOFOLLOW_LINKS); }
        void install(File root, boolean replace) throws IOException {
            if (installed) throw new IOException("This import was already installed");
            if (!root.toPath().toAbsolutePath().normalize().equals(staging.getParent().toAbsolutePath().normalize()))
                throw new IOException("Import destination changed after preparation");
            Path target = target(root.toPath(), destination);
            safeDirectory(target.getParent());
            if (Files.isSymbolicLink(target)) throw new IOException("The destination is a symbolic link");
            boolean existing = Files.exists(target, LinkOption.NOFOLLOW_LINKS);
            if (existing && !replace) throw new IOException("Package already exists; replacement requires confirmation");
            Path backup = staging.resolve("previous");
            Files.write(staging.resolve("destination"), destination.getBytes(StandardCharsets.UTF_8));
            if (existing) Files.move(target, backup, StandardCopyOption.ATOMIC_MOVE);
            try {
                Files.move(content, target, StandardCopyOption.ATOMIC_MOVE);
                installed = true;
            } catch (IOException error) {
                if (existing) {
                    try { Files.move(backup, target, StandardCopyOption.ATOMIC_MOVE); }
                    catch (IOException rollback) { error.addSuppressed(rollback); throw new IOException("Restore failed; previous package retained at " + backup, error); }
                }
                throw error;
            }
        }
        @Override public void close() throws IOException {
            // A failed rollback must keep its recovery copy.
            try { if (installed || !Files.exists(staging.resolve("previous"))) remove(staging); }
            finally { try { if (lock.isValid()) lock.release(); } finally { lockChannel.close(); } }
        }
    }

    private static void safeDirectory(Path directory) throws IOException {
        if (Files.isSymbolicLink(directory)) throw new IOException("Destination directories must not be symbolic links");
        Files.createDirectories(directory);
    }
    private static Path child(Path parent, String name) throws IOException {
        Path found = null;
        if (Files.isSymbolicLink(parent)) throw new IOException("Destination is a symbolic link");
        if (Files.isDirectory(parent)) try (java.nio.file.DirectoryStream<Path> entries = Files.newDirectoryStream(parent)) {
            for (Path entry : entries) if (entry.getFileName().toString().equalsIgnoreCase(name)) {
                if (found != null) throw new IOException("Ambiguous destination names: " + name);
                found = entry;
            }
        }
        return found == null ? parent.resolve(name) : found;
    }
    private static Path target(Path root, String destination) throws IOException {
        if (!destination.matches("[A-Za-z0-9][A-Za-z0-9 _-]{0,39}|_shared/(singleplayer|multiplayer)"))
            throw new IOException("Invalid import destination");
        Path result = child(root, "mcc_maps");
        for (String part : destination.split("/")) result = child(result, part);
        return result;
    }
    private static void recover(Path root) throws IOException {
        try (java.nio.file.DirectoryStream<Path> entries = Files.newDirectoryStream(root, ".mcc-import-*")) {
            for (Path staging : entries) {
                Path owner = staging.resolve("owner"), destination = staging.resolve("destination"), previous = staging.resolve("previous");
                if (Files.isSymbolicLink(staging) || !Files.isRegularFile(owner, LinkOption.NOFOLLOW_LINKS) ||
                    Files.size(owner) != 13 || !new String(Files.readAllBytes(owner), StandardCharsets.US_ASCII).equals("mcc-import-v1")) continue;
                if (Files.exists(previous, LinkOption.NOFOLLOW_LINKS)) {
                    if (!Files.isRegularFile(destination, LinkOption.NOFOLLOW_LINKS) || Files.size(destination) > 64)
                        throw new IOException("Import recovery needs attention: " + staging);
                    Path target = target(root, new String(Files.readAllBytes(destination), StandardCharsets.UTF_8));
                    if (!Files.exists(target, LinkOption.NOFOLLOW_LINKS)) {
                        safeDirectory(target.getParent());
                        Files.move(previous, target, StandardCopyOption.ATOMIC_MOVE);
                    }
                }
                remove(staging);
            }
        }
    }
    static String path(String value) throws IOException {
        value = value.replace('\\', '/');
        if (value.length() > 512 || value.isEmpty() || value.startsWith("/") || value.endsWith("/"))
            throw new IOException("Invalid package path");
        String[] parts = value.split("/", -1);
        if (parts.length > 8) throw new IOException("Package path is too deep");
        for (String part : parts) {
            if (part.isEmpty() || part.equals(".") || part.equals("..") || part.endsWith(".") || part.endsWith(" "))
                throw new IOException("Unsafe package path: " + value);
            for (int i = 0; i < part.length(); i++) {
                char c = part.charAt(i);
                if (c < 32 || c == 127 || ":*?\"<>|".indexOf(c) >= 0) throw new IOException("Unsafe package path: " + value);
            }
        }
        return value;
    }
    private static String lower(String value) { return value.toLowerCase(Locale.ROOT); }
    private static void remove(Path root) throws IOException {
        if (!Files.exists(root, LinkOption.NOFOLLOW_LINKS)) return;
        if (Files.isDirectory(root, LinkOption.NOFOLLOW_LINKS)) {
            try (java.nio.file.DirectoryStream<Path> children = Files.newDirectoryStream(root)) {
                for (Path child : children) remove(child);
            }
        }
        Files.delete(root);
    }
    private static Object json(Path file) throws IOException {
        if (file == null || Files.size(file) > 262144) throw new IOException("Missing or oversized campaign metadata");
        byte[] bytes = Files.readAllBytes(file);
        String text = StandardCharsets.UTF_8.newDecoder().decode(ByteBuffer.wrap(bytes)).toString();
        if (text.startsWith("\uFEFF")) text = text.substring(1);
        return MccJson.parse(text);
    }
    private static String text(Object object, String key) { return MccJson.string(MccJson.member(object, key)); }
    private static String title(Object object) { return text(MccJson.member(object, "Title"), "Neutral"); }

    static Prepared prepare(File root, Source source, String requestedName, int mode) throws IOException {
        if (root == null || mode < PACKAGE || mode > MULTIPLAYER_RESOURCES) throw new IOException("Storage unavailable");
        safeDirectory(root.toPath());
        FileChannel lockChannel = FileChannel.open(root.toPath().resolve(".mcc-import.lock"), StandardOpenOption.CREATE, StandardOpenOption.WRITE);
        FileLock lock;
        try { lock = lockChannel.tryLock(); }
        catch (IOException | OverlappingFileLockException error) { lockChannel.close(); throw new IOException("Another import is in progress", error); }
        if (lock == null) { lockChannel.close(); throw new IOException("Another import is in progress"); }
        Path staging;
        try { recover(root.toPath()); staging = Files.createTempDirectory(root.toPath(), ".mcc-import-"); }
        catch (IOException error) { lock.release(); lockChannel.close(); throw error; }
        Path raw = staging.resolve("raw"), content = staging.resolve("content");
        Map<String,Path> files = new HashMap<>();
        Set<String> allPaths = new HashSet<>();
        long[] total = {0};
        try {
            Files.write(staging.resolve("owner"), "mcc-import-v1".getBytes(StandardCharsets.US_ASCII));
            Files.createDirectory(raw); Files.createDirectory(content);
            source.copy((name, stream) -> {
                String normalized = path(name), key = lower(normalized);
                if (allPaths.size() >= MAX_FILES || !allPaths.add(key)) throw new IOException("Too many files or duplicate package path");
                boolean retain = key.endsWith(".map") || key.endsWith(".json") || key.endsWith(".png") || key.endsWith(".txt");
                long limit = key.endsWith(".map") ? 0x7fffffffL : key.endsWith(".json") ? 262144 : 8L << 20;
                Path out = raw.resolve(normalized);
                if (retain) Files.createDirectories(out.getParent());
                try (OutputStream output = retain ? Files.newOutputStream(out) : new OutputStream() { public void write(int b) {} public void write(byte[] b, int off, int len) {} }) {
                    byte[] buffer = new byte[65536];
                    long bytes = 0;
                    int got;
                    while ((got = stream.read(buffer)) != -1) {
                        if (Thread.currentThread().isInterrupted()) throw new IOException("Import cancelled");
                        bytes += got; total[0] += got;
                        if (bytes > limit || total[0] > MAX_TOTAL) throw new IOException("Package exceeds import size limits");
                        output.write(buffer, 0, got);
                    }
                }
                if (retain) files.put(key, out);
            });
            if (mode != PACKAGE) {
                String[] names = mode == SINGLEPLAYER_RESOURCES ? new String[]{"bitmaps.map"} : new String[]{"bitmaps.map", "sounds.map"};
                for (String name : names) {
                    Path found = null;
                    for (Map.Entry<String,Path> entry : files.entrySet()) {
                        if (entry.getKey().equals(name) || entry.getKey().endsWith("/" + name)) {
                            if (found != null) throw new IOException("Select one resource set; multiple " + name + " files found");
                            found = entry.getValue();
                        }
                    }
                    if (found == null || Files.size(found) < 16) throw new IOException("Missing or truncated " + name);
                    Files.move(found, content.resolve(name));
                }
                String kind = mode == SINGLEPLAYER_RESOURCES ? "singleplayer" : "multiplayer";
                return new Prepared(staging, content, "_shared/" + kind, kind + " shared resources", 0, total[0], lockChannel, lock);
            }
            String manifestKey = null;
            for (String key : files.keySet()) {
                if (key.equals("modinfo.json") || key.endsWith("/modinfo.json")) {
                    if (manifestKey != null) throw new IOException("Choose one Workshop package at a time");
                    manifestKey = key;
                }
            }
            if (manifestKey == null) throw new IOException("Select the complete package containing ModInfo.json");
            String prefix = manifestKey.substring(0, manifestKey.length() - "modinfo.json".length());
            Object info = json(files.get(manifestKey));
            if (!text(info, "Engine").equals("Halo1")) throw new IOException("Only Halo 1 MCC packages are supported");
            Object idValue = MccJson.member(MccJson.member(MccJson.member(info, "ModIdentifier"), "HostedModIds"), "SteamWorkshopId");
            String id = idValue instanceof MccJson.Number ? ((MccJson.Number)idValue).text : MccJson.string(idValue);
            if (!id.matches("[0-9]{1,20}")) id = "";
            String name = requestedName.trim();
            if (name.isEmpty()) name = id;
            if (!name.matches("[A-Za-z0-9][A-Za-z0-9 _-]{0,39}") || name.endsWith(" "))
                throw new IOException("Choose a package name of 1-40 letters, digits, spaces, underscores or hyphens");
            Map<String,Integer> maps = new HashMap<>();
            Set<String> outputNames = new HashSet<>();
            for (Map.Entry<String,Path> entry : files.entrySet()) {
                if (!entry.getKey().startsWith(prefix)) continue;
                String relative = entry.getKey().substring(prefix.length());
                String output = relative;
                if (relative.endsWith(".map")) {
                    if (relative.startsWith("halo1/maps/")) output = relative.substring(11);
                    else if (relative.startsWith("maps/")) output = relative.substring(5);
                    if (output.contains("/")) throw new IOException("Unsupported nested map path: " + relative);
                    if (!output.equals("bitmaps.map") && !output.equals("sounds.map") && !output.equals("loc.map")) {
                        String leaf = output.substring(0, output.length() - 4);
                        if (!leaf.matches("[a-z0-9][a-z0-9 _-]*") || 9 + name.length() + 1 + leaf.length() > 63)
                            throw new IOException("Unlaunchable MCC map name: " + output);
                        maps.put(leaf, mapType(entry.getValue()));
                    }
                }
                if (!outputNames.add(output)) throw new IOException("Conflicting flattened package files: " + output);
                Path destination = content.resolve(output);
                Files.createDirectories(destination.getParent());
                Files.move(entry.getValue(), destination);
            }
            if (maps.isEmpty() || maps.size() > 1024) throw new IOException("Package has no supported MCC maps or exceeds the map limit");
            Object game = MccJson.member(info, "GameModContents");
            if (Boolean.TRUE.equals(MccJson.member(game, "HasCampaign"))) {
                Object campaign = json(content.resolve("campaigninfo.json"));
                Object missions = MccJson.member(campaign, "CampaignMaps");
                if (!(missions instanceof List) || ((List<?>)missions).isEmpty() || ((List<?>)missions).size() > 64)
                    throw new IOException("Campaign must declare 1-64 missions");
                Set<String> seen = new HashSet<>();
                for (Object mission : (List<?>)missions) {
                    String relative = lower(path(MccJson.string(mission)));
                    Object metadata = json(content.resolve(relative));
                    String scenario = path(text(metadata, "ScenarioFile"));
                    String leaf = lower(scenario.substring(scenario.lastIndexOf('/') + 1));
                    if (leaf.endsWith(".map")) leaf = leaf.substring(0, leaf.length() - 4);
                    Object kind = MccJson.member(metadata, "CampaignMapKind");
                    if (metadata instanceof Map && ((Map<?,?>)metadata).containsKey("CampaignMapKind") &&
                            (!(kind instanceof String) || !"Default".equalsIgnoreCase((String)kind)))
                        throw new IOException("Unsupported campaign mission kind");
                    if (!seen.add(leaf) || !Integer.valueOf(0).equals(maps.get(leaf)))
                        throw new IOException("Missing, duplicate or non-campaign mission: " + leaf);
                }
            }
            String display = title(info);
            if (!id.isEmpty()) {
                String safeTitle = display.replaceAll("[^ -~]", "");
                if (safeTitle.length() > 40) safeTitle = safeTitle.substring(0, 40);
                Files.write(content.resolve("workshop_id.txt"), (id + "\n" + safeTitle + "\n").getBytes(StandardCharsets.US_ASCII));
            }
            return new Prepared(staging, content, name, display.isEmpty() ? name : display, maps.size(), total[0], lockChannel, lock);
        } catch (IOException | RuntimeException error) {
            try { remove(staging); } catch (IOException cleanup) { error.addSuppressed(cleanup); }
            try { lock.release(); lockChannel.close(); } catch (IOException cleanup) { error.addSuppressed(cleanup); }
            throw error;
        }
    }

    private static int mapType(Path path) throws IOException {
        byte[] header = new byte[2048];
        long actual = Files.size(path);
        try (RandomAccessFile file = new RandomAccessFile(path.toFile(), "r")) { file.readFully(header); }
        ByteBuffer h = ByteBuffer.wrap(header).order(ByteOrder.LITTLE_ENDIAN);
        long length = Integer.toUnsignedLong(h.getInt(8));
        if (length == 0) length = actual;
        long offset = Integer.toUnsignedLong(h.getInt(16)), size = Integer.toUnsignedLong(h.getInt(20));
        int type = Short.toUnsignedInt(h.getShort(0x60));
        boolean nameEnd = false, buildEnd = false;
        for (int i = 0; i < 32; i++) { nameEnd |= header[0x20 + i] == 0; buildEnd |= header[0x40 + i] == 0; }
        if (h.getInt(0) != 0x68656164 || h.getInt(4) != 13 || h.getInt(0x7fc) != 0x666f6f74 ||
            !nameEnd || !buildEnd || actual > 0x7fffffffL || length < 2048 || length > actual || type > 1 || offset < 2048 ||
            size < 40 || size > 0x4000000 || offset > length || size > length - offset)
            throw new IOException("Invalid Halo 1 MCC map header: " + path.getFileName());
        return type;
    }
}
