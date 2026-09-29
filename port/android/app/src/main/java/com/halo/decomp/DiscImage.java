package com.halo.decomp;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.RandomAccessFile;
import java.nio.ByteBuffer;
import java.nio.channels.FileChannel;
import java.nio.charset.StandardCharsets;
import java.util.ArrayList;
import java.util.List;

/**
 * Extracts the maps/ folder from an Xbox disc image (XDVDFS).
 *
 * A Java port of port/linux/src/xiso.c, so the app can recover the game data
 * on the device itself: 2048-byte sectors; a volume descriptor at 0x10000
 * that starts and ends with "MICROSOFT*XBOX*MEDIA" and gives the root
 * directory's sector and size; directories whose entries form a binary tree
 * (left and right subtree offsets in 4-byte units, the start sector, the
 * size, attributes, the name's length and the name).
 *
 * The files are written to maps.partial, which becomes maps once they all
 * are, so an interrupted extraction never leaves data that looks complete.
 */
final class DiscImage {
    private DiscImage() {}

    interface Progress {
        /** called as the copy goes; returns false to stop (the player cancelled) */
        boolean report(String name, long done, long total);
    }

    /** the player stopped the extraction */
    static final class Cancelled extends IOException {
        Cancelled() { super("cancelled"); }
    }

    private static final int SECTOR = 2048;
    private static final long VOLUME_DESCRIPTOR_OFFSET = 0x10000L;
    private static final int ENTRY_HEADER = 14;
    private static final int ATTR_DIRECTORY = 0x10;
    private static final int MAX_DIRECTORY = 4 << 20;
    private static final int MAX_FILES = 256;
    private static final int BUFFER = 1 << 20;
    private static final byte[] MAGIC = "MICROSOFT*XBOX*MEDIA".getBytes(StandardCharsets.US_ASCII);
    /* a plain image, and whole-disc images (extract-xiso's GLOBAL, XGD3 and XGD1 offsets) */
    private static final long[] PARTITIONS = { 0L, 0x0FD90000L, 0x02080000L, 0x18300000L };

    private static final class Entry {
        String name;
        long sector;
        long size;
    }

    /** a seekable disc image: a file, or a document the picker handed us */
    static FileChannel open(File file) throws IOException {
        return new RandomAccessFile(file, "r").getChannel();
    }

    /** what probing an image found: whether it is a Halo disc and how whole it is */
    static final class Info {
        boolean halo;
        int mapCount;
        long dataBytes;
        long fileBytes;
        boolean complete;

        String summary() {
            if (!halo)
                return "NOT A HALO DISC  ·  " + human(fileBytes);
            return mapCount + " MAPS  ·  " + human(dataBytes) + (complete ? "  ·  READY" : "  ·  INCOMPLETE");
        }

        private static String human(long bytes) {
            if (bytes >= 1L << 30)
                return String.format(java.util.Locale.US, "%.1f GB", bytes / (double) (1L << 30));
            return String.format(java.util.Locale.US, "%.0f MB", bytes / (double) (1L << 20));
        }
    }

    /**
     * Reads an image's file system just far enough to tell a Halo disc from
     * another ROM, and whether every map in it lies inside the file.
     */
    static Info probe(FileChannel channel) {
        Info info = new Info();
        try {
            info.fileBytes = channel.size();
            long[] volume = findVolume(channel);
            if (volume == null)
                return info;
            byte[] root = readDirectory(channel, volume[0], volume[1], volume[2]);
            List<Entry> directories = new ArrayList<>();
            walk(root, 0, 0, true, directories, new int[1]);
            Entry maps = find(directories, "maps");
            if (maps == null)
                return info;
            byte[] table = readDirectory(channel, volume[0], maps.sector, maps.size);
            List<Entry> files = new ArrayList<>();
            walk(table, 0, 0, false, files, new int[1]);
            if (find(files, "ui.map") == null)
                return info;
            info.halo = true;
            info.mapCount = files.size();
            long length = channel.size();
            long total = 0;
            boolean complete = true;
            for (Entry file : files) {
                total += file.size;
                if (volume[0] + file.sector * SECTOR + file.size > length)
                    complete = false;
            }
            info.dataBytes = total;
            info.complete = complete;
        } catch (IOException | RuntimeException exception) {
            // not readable as an Xbox disc
        }
        return info;
    }

    /** true if the image is an Xbox disc with a Halo maps/ui.map in it */
    static boolean isHaloDisc(FileChannel channel) {
        return probe(channel).halo;
    }

    /** extracts maps/ into destination; throws with a player-readable reason */
    static File extract(FileChannel channel, File destination, Progress progress) throws IOException {
        long[] volume = findVolume(channel);
        if (volume == null)
            throw new IOException("This is not an Xbox disc image.");

        byte[] root = readDirectory(channel, volume[0], volume[1], volume[2]);
        List<Entry> directories = new ArrayList<>();
        walk(root, 0, 0, true, directories, new int[1]);
        Entry maps = find(directories, "maps");
        if (maps == null)
            throw new IOException("The disc image has no maps folder: it is not a Halo disc.");

        byte[] table = readDirectory(channel, volume[0], maps.sector, maps.size);
        List<Entry> files = new ArrayList<>();
        walk(table, 0, 0, false, files, new int[1]);
        boolean hasUi = false;
        long total = 0;
        for (Entry file : files) {
            total += file.size;
            hasUi |= matches(file.name, "ui.map");
        }
        if (!hasUi)
            throw new IOException("The disc image's maps folder has no ui.map: it is not a Halo disc.");

        File partial = new File(destination, "maps.partial");
        deleteTree(partial);
        if (!partial.mkdirs())
            throw new IOException("Could not write " + partial);

        byte[] buffer = new byte[BUFFER];
        long done = 0;
        try {
            for (Entry file : files) {
                File output = new File(partial, file.name);
                long offset = volume[0] + file.sector * SECTOR;
                long remaining = file.size;
                try (FileOutputStream stream = new FileOutputStream(output)) {
                    while (remaining > 0) {
                        int count = (int) Math.min(remaining, BUFFER);
                        readAt(channel, offset, buffer, count);
                        stream.write(buffer, 0, count);
                        offset += count;
                        remaining -= count;
                        done += count;
                        if (progress != null && !progress.report(file.name, done, total))
                            throw new Cancelled();
                    }
                }
            }
        } catch (IOException | RuntimeException error) {
            deleteTree(partial);
            throw error;
        }

        File finalDirectory = new File(destination, "maps");
        deleteTree(finalDirectory);
        if (!partial.renameTo(finalDirectory)) {
            deleteTree(partial);
            throw new IOException("Could not move the extracted data into place.");
        }
        return finalDirectory;
    }

    /** the partition's volume descriptor: the root directory's sector and size */
    private static long[] findVolume(FileChannel channel) throws IOException {
        byte[] descriptor = new byte[SECTOR];
        for (long partition : PARTITIONS) {
            if (channel.size() < partition + VOLUME_DESCRIPTOR_OFFSET + SECTOR)
                continue;
            readAt(channel, partition + VOLUME_DESCRIPTOR_OFFSET, descriptor, SECTOR);
            if (!regionEquals(descriptor, 0, MAGIC) || !regionEquals(descriptor, 0x7EC, MAGIC))
                continue;
            return new long[] { partition, readU32(descriptor, 20), readU32(descriptor, 24) };
        }
        return null;
    }

    private static byte[] readDirectory(FileChannel channel, long partition, long sector, long size) throws IOException {
        if (size <= 0 || size > MAX_DIRECTORY)
            throw new IOException("The disc image's file system is damaged.");
        byte[] table = new byte[(int) size];
        readAt(channel, partition + sector * SECTOR, table, (int) size);
        return table;
    }

    /** collects the entries of the subtree at offset (4-byte units) */
    private static void walk(byte[] table, long offset, int depth, boolean wantDirectory, List<Entry> out,
        int[] visited) {
        offset *= 4;
        if (depth > 64 || visited[0] > 4096 || offset + ENTRY_HEADER > table.length)
            return;
        visited[0]++;
        int index = (int) offset;
        int left = (table[index] & 0xFF) | (table[index + 1] & 0xFF) << 8;
        int right = (table[index + 2] & 0xFF) | (table[index + 3] & 0xFF) << 8;
        if (left == 0xFFFF)  // padding: an empty directory
            return;
        int nameLength = table[index + 13] & 0xFF;
        if (left != 0)
            walk(table, left, depth + 1, wantDirectory, out, visited);
        if (offset + ENTRY_HEADER + nameLength <= table.length && nameLength > 0
            && ((table[index + 12] & ATTR_DIRECTORY) != 0) == wantDirectory && out.size() < MAX_FILES) {
            String name = new String(table, index + ENTRY_HEADER, nameLength, StandardCharsets.US_ASCII);
            if (!name.equals(".") && !name.equals("..") && name.indexOf('/') < 0 && name.indexOf('\\') < 0) {
                Entry entry = new Entry();
                entry.name = name;
                entry.sector = readU32(table, index + 4);
                entry.size = readU32(table, index + 8);
                out.add(entry);
            }
        }
        if (right != 0)
            walk(table, right, depth + 1, wantDirectory, out, visited);
    }

    private static Entry find(List<Entry> entries, String name) {
        for (Entry entry : entries)
            if (matches(entry.name, name))
                return entry;
        return null;
    }

    private static boolean matches(String a, String b) {
        return a.equalsIgnoreCase(b);
    }

    private static void readAt(FileChannel channel, long offset, byte[] buffer, int length) throws IOException {
        ByteBuffer target = ByteBuffer.wrap(buffer, 0, length);
        long position = offset;
        while (target.hasRemaining()) {
            int count = channel.read(target, position);
            if (count < 0)
                throw new IOException("Could not read the disc image (is it complete?).");
            position += count;
        }
    }

    private static boolean regionEquals(byte[] bytes, int offset, byte[] expected) {
        for (int i = 0; i < expected.length; i++)
            if (bytes[offset + i] != expected[i])
                return false;
        return true;
    }

    private static long readU32(byte[] bytes, int offset) {
        return (bytes[offset] & 0xFFL) | (bytes[offset + 1] & 0xFFL) << 8
            | (bytes[offset + 2] & 0xFFL) << 16 | (bytes[offset + 3] & 0xFFL) << 24;
    }

    private static void deleteTree(File file) {
        if (file.isDirectory()) {
            File[] children = file.listFiles();
            if (children != null)
                for (File child : children)
                    deleteTree(child);
        }
        file.delete();
    }
}
