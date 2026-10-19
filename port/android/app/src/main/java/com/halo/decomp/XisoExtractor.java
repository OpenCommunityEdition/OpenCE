package com.halo.decomp;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.nio.ByteBuffer;
import java.nio.channels.FileChannel;
import java.nio.charset.StandardCharsets;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.BitSet;
import java.util.List;
import java.util.Locale;

/**
 * Copies the maps folder out of the player's Xbox disc image of Halo, for
 * LauncherActivity, as the desktop games do (port/linux/src/xiso.c; the web
 * page's is port/web/site/xiso-worker.js). The image is a document the
 * player picked, read through its FileChannel at absolute positions.
 *
 * Written from a description of the Xbox's disc file system, XDVDFS
 * (xboxdevwiki's XDVDFS page, https://xboxdevwiki.net/XDVDFS, and the format
 * facts OpenCE's readers rely on). What this class relies on:
 *
 * - Sectors are 2048 bytes and integers little-endian. Sector numbers count
 *   from the start of the game partition: an image of the partition alone
 *   has it at 0, an image of a whole disc further in (0x0FD90000, 0x02080000
 *   or 0x18300000 for XGD2, XGD3 and XGD1 discs). They are tried in that
 *   order.
 * - The volume descriptor is the partition's sector 32 (at 0x10000). It
 *   holds "MICROSOFT*XBOX*MEDIA" at its start and again at 0x7EC, and the
 *   root directory table's start sector (u32 at 0x14) and size in bytes (u32
 *   at 0x18).
 * - A directory table is a binary tree of entries whose root is the entry at
 *   offset 0. An entry is a left and a right subtree link (u16 each, in
 *   4-byte units from the table's start, 0 for none), the start sector and
 *   the size of the file or of the subdirectory's table (u32 each), the
 *   attributes (u8, 0x10 for a directory), the name's length (u8) and the
 *   name. Unused bytes of a table are 0xFF, so a left link of 0xFFFF is
 *   padding, not an entry.
 * - A file is one contiguous run of bytes from the start of its sector.
 *
 * The image is the player's file and nothing read from it is trusted. Only
 * the root's table and the maps folder's are read, each whole and at most
 * 4 MB; the walk of a table visits each offset once, goes at most 64 links
 * deep and looks at most 4096 entries; and a file is written only under a
 * plain name that no file before it had (ignoring case).
 *
 * The loading screen's picture, which the retail disc has no file of, is
 * taken out of the root's default.xbe (read whole, at most 64 MB) and
 * written beside the maps as the loading.tga the game reads, when the maps
 * folder has none (loadingPicture, as xiso.c's xiso_write_loading_picture);
 * without it the light sweeps over black, as the game's own does.
 *
 * The app starts the game once maps/ui.map exists, and makes the maps folder
 * itself, so the copy is written to maps.partial (emptied first of what an
 * earlier attempt left) and its files moved into maps once all are
 * complete, ui.map last.
 */
final class XisoExtractor {
    private static final int SECTOR_SIZE = 2048;
    private static final int DESCRIPTOR_OFFSET = 0x10000;
    private static final byte[] MAGIC = "MICROSOFT*XBOX*MEDIA".getBytes(StandardCharsets.ISO_8859_1);
    private static final int SECOND_MAGIC_OFFSET = 0x7EC;
    private static final int ENTRY_FIXED_SIZE = 14;
    private static final int ATTRIBUTE_DIRECTORY = 0x10;
    private static final int PADDING_LINK = 0xFFFF;
    /** where the game partition may start, in the order they are tried */
    private static final long[] PARTITION_STARTS = {
        0x00000000L, // an image of the game partition alone
        0x0FD90000L, // a whole XGD2 disc
        0x02080000L, // a whole XGD3 disc
        0x18300000L, // a whole XGD1 disc (the original Xbox's)
    };
    /** directory tables are a few sectors; anything much larger is not one */
    private static final int MAXIMUM_TABLE_SIZE = 4 * 1024 * 1024;
    private static final int MAXIMUM_DEPTH = 64;
    private static final int MAXIMUM_VISITS = 4096;
    private static final int MAXIMUM_ENTRIES = 256;
    private static final int COPY_CHUNK = 1024 * 1024;
    private static final String PARTIAL_DIRECTORY = "maps.partial";
    /** the loading screen's picture (source/interface/progress_bar.c), out of default.xbe; the executable is a few MB */
    private static final int LOADING_WIDTH = 320;
    private static final int LOADING_HEIGHT = 240;
    private static final int TGA_HEADER_SIZE = 18;
    private static final long MAXIMUM_XBE_SIZE = 64 * 1024 * 1024;
    private static final String LOADING_PICTURE = "loading.tga";

    /** Told as the copy goes: the file being copied, and the bytes copied of all the files'. */
    interface Progress {
        void report(String file, long done, long total);
    }

    /** A reason the player can act on: its message is shown as it is. */
    static final class ExtractException extends IOException {
        ExtractException(String message) {
            super(message);
        }
    }

    /** a file or folder found in a directory table */
    private static final class Entry {
        final String name;
        final long sector;
        final long size;

        Entry(String name, long sector, long size) {
            this.name = name;
            this.sector = sector;
            this.size = size;
        }
    }

    private XisoExtractor() {
    }

    /** Copies the image's maps folder to destination/maps. */
    static void extractMaps(FileChannel image, File destination, Progress progress) throws IOException {
        long imageSize = image.size();
        long partition = -1;
        long rootSector = 0, rootSize = 0;
        ByteBuffer descriptor = ByteBuffer.allocate(SECTOR_SIZE);

        for (long start : PARTITION_STARTS) {
            if (start + DESCRIPTOR_OFFSET + SECTOR_SIZE > imageSize)
                continue;
            descriptor.clear();
            if (!readFully(image, descriptor, start + DESCRIPTOR_OFFSET))
                continue;
            byte[] bytes = descriptor.array();
            if (!hasMagic(bytes, 0) || !hasMagic(bytes, SECOND_MAGIC_OFFSET))
                continue;
            partition = start;
            rootSector = u32(bytes, 0x14);
            rootSize = u32(bytes, 0x18);
            break;
        }
        if (partition < 0)
            throw new ExtractException("This is not an Xbox disc image.");

        // the root's maps folder
        byte[] root = readTable(image, partition, rootSector, rootSize, "The disc image's file system is damaged.");
        Entry maps = null;
        Entry xbe = null;
        for (Entry entry : walkTable(root, false)) {
            if (entry.name.equalsIgnoreCase("default.xbe"))
                xbe = entry;
        }
        List<Entry> directories = walkTable(root, true);
        for (Entry entry : directories) {
            if (entry.name.equalsIgnoreCase("maps")) {
                maps = entry;
                break;
            }
        }
        /* a multi-language European disc has no "maps" but a "maps_xx" per
        language: the one for the device's language, else English, else the
        first (as port/linux/src/xiso.c) */
        if (maps == null) {
            String wanted = "maps_" + Locale.getDefault().getLanguage();
            Entry english = null, first = null;
            for (Entry entry : directories) {
                if (entry.name.length() != 7 || !entry.name.regionMatches(true, 0, "maps_", 0, 5))
                    continue;
                if (entry.name.equalsIgnoreCase(wanted)) {
                    maps = entry;
                    break;
                }
                if (english == null && entry.name.equalsIgnoreCase("maps_en"))
                    english = entry;
                if (first == null)
                    first = entry;
            }
            if (maps == null)
                maps = english != null ? english : first;
        }
        if (maps == null)
            throw new ExtractException("The disc image has no maps folder: it is not a Halo disc.");

        // its files
        byte[] table = readTable(image, partition, maps.sector, maps.size, "The disc image's maps folder is damaged.");
        List<Entry> files = walkTable(table, false);
        Entry userInterface = null;
        boolean hasLoading = false;
        for (Entry file : files) {
            if (file.name.equalsIgnoreCase("ui.map"))
                userInterface = file;
            hasLoading |= file.name.equalsIgnoreCase(LOADING_PICTURE);
        }
        if (userInterface == null)
            throw new ExtractException("The disc image's maps folder has no ui.map: it is not a Halo disc.");
        // (an image that ends before a file does is refused before anything
        // is written; an empty file's sector need not be anywhere)
        long total = 0;
        for (Entry file : files) {
            if (file.size > 0 && partition + file.sector * SECTOR_SIZE + file.size > imageSize)
                throw new ExtractException(readFailure(file));
            total += file.size;
        }

        // the copy, to maps.partial
        File partial = new File(destination, PARTIAL_DIRECTORY);
        if (partial.isDirectory()) {
            File[] leftovers = partial.listFiles();
            if (leftovers != null) {
                for (File leftover : leftovers)
                    leftover.delete();
            }
        } else {
            partial.delete();
            partial.mkdirs();
        }
        if (!partial.isDirectory())
            throw new ExtractException("Could not create " + partial.getPath() + ".");
        ByteBuffer buffer = ByteBuffer.allocate(COPY_CHUNK);
        long done = 0;
        for (Entry file : files)
            done = copyFile(image, partition, file, new File(partial, file.name), buffer, done, total, progress);
        if (!hasLoading && writeLoadingPicture(image, imageSize, partition, xbe, new File(partial, LOADING_PICTURE)))
            files.add(new Entry(LOADING_PICTURE, 0, 0));

        // then into maps, ui.map (the app's sign that the data is there) last
        File mapsFolder = new File(destination, "maps");
        if (!mapsFolder.isDirectory() && !mapsFolder.mkdirs() && !mapsFolder.isDirectory())
            throw new ExtractException("Could not create " + mapsFolder.getPath() + ".");
        List<Entry> order = new ArrayList<>(files);
        order.remove(userInterface);
        order.add(userInterface);
        for (Entry file : order) {
            File target = new File(mapsFolder, file.name);
            target.delete();
            if (!new File(partial, file.name).renameTo(target))
                throw new ExtractException("Could not move " + file.name + " into " + mapsFolder.getPath() + ".");
        }
        partial.delete();
    }

    /** No path, no . or .., and only printable ASCII that every file system takes as a plain name. */
    static boolean isPlainName(String name) {
        if (name == null || name.isEmpty() || name.equals(".") || name.equals(".."))
            return false;
        for (int index = 0; index < name.length(); index++) {
            char character = name.charAt(index);

            if (character < 0x20 || character > 0x7E || "/\\:*?\"<>|".indexOf(character) >= 0)
                return false;
        }
        return true;
    }

    /* ---------- the image */

    private static long u16(byte[] bytes, int offset) {
        return (bytes[offset] & 0xFF) | (bytes[offset + 1] & 0xFF) << 8;
    }

    private static long u32(byte[] bytes, int offset) {
        return u16(bytes, offset) | u16(bytes, offset + 2) << 16;
    }

    private static boolean hasMagic(byte[] bytes, int offset) {
        for (int index = 0; index < MAGIC.length; index++) {
            if (bytes[offset + index] != MAGIC[index])
                return false;
        }
        return true;
    }

    /** Fills buffer from the image at position; false if the image ends first. */
    private static boolean readFully(FileChannel image, ByteBuffer buffer, long position) throws IOException {
        while (buffer.hasRemaining()) {
            int count = image.read(buffer, position);

            if (count <= 0)
                return false;
            position += count;
        }
        return true;
    }

    private static byte[] readTable(FileChannel image, long partition, long sector, long size, String damaged)
            throws IOException {
        if (size == 0 || size > MAXIMUM_TABLE_SIZE)
            throw new ExtractException(damaged);
        ByteBuffer table = ByteBuffer.allocate((int) size);
        if (!readFully(image, table, partition + sector * SECTOR_SIZE))
            throw new ExtractException("Could not read the disc image (is it complete?).");
        return table.array();
    }

    /* ---------- directory tables */

    private static final class Walk {
        final byte[] table;
        final boolean directories;
        final BitSet visited = new BitSet();
        final List<Entry> entries = new ArrayList<>();
        int visits;
        boolean stopped;

        Walk(byte[] table, boolean directories) {
            this.table = table;
            this.directories = directories;
        }
    }

    /** The folders (or the files) of a table with plain names, in the tree's order. */
    private static List<Entry> walkTable(byte[] table, boolean directories) {
        Walk walk = new Walk(table, directories);

        walkEntry(walk, 0, 0);
        return walk.entries;
    }

    /**
     * The subtree whose root entry is at offset: its left subtree, the
     * entry, its right subtree. A link to padding, past the table or back to
     * an entry already visited ends that branch.
     */
    private static void walkEntry(Walk walk, int offset, int depth) {
        byte[] table = walk.table;

        if (walk.stopped || depth > MAXIMUM_DEPTH)
            return;
        if (offset >= table.length || table.length - offset < ENTRY_FIXED_SIZE)
            return;
        int left = (int) u16(table, offset);
        int right = (int) u16(table, offset + 2);
        if (left == PADDING_LINK || walk.visited.get(offset / 4))
            return;
        walk.visited.set(offset / 4);
        if (walk.visits >= MAXIMUM_VISITS) {
            walk.stopped = true;
            return;
        }
        walk.visits++;

        if (left != 0)
            walkEntry(walk, left * 4, depth + 1);
        int nameLength = table[offset + 13] & 0xFF;
        if (nameLength != 0 && nameLength <= table.length - offset - ENTRY_FIXED_SIZE)
            collect(walk, offset, nameLength);
        if (right != 0)
            walkEntry(walk, right * 4, depth + 1);
    }

    private static void collect(Walk walk, int offset, int nameLength) {
        byte[] table = walk.table;
        boolean directory = (table[offset + 12] & ATTRIBUTE_DIRECTORY) != 0;

        if (directory != walk.directories || walk.entries.size() >= MAXIMUM_ENTRIES)
            return;
        String name = new String(table, offset + ENTRY_FIXED_SIZE, nameLength, StandardCharsets.ISO_8859_1);
        if (!isPlainName(name))
            return;
        // (the destination's file system may ignore case)
        for (Entry entry : walk.entries) {
            if (entry.name.equalsIgnoreCase(name))
                return;
        }
        walk.entries.add(new Entry(name, u32(table, offset + 4), u32(table, offset + 8)));
    }

    /* ---------- the loading screen's picture */

    /**
     * The loading screen's picture out of a default.xbe, as the TGA the game
     * reads (uncompressed, 24 bits, rows from the bottom), or null. The
     * executable holds it, 320 by 240, as runs of 4-bit greys (a byte each:
     * the grey's high four bits in its low nibble, the run's length in its
     * high one) after the width and height as u32s: found by those two
     * numbers followed by runs that fill the picture exactly (a run of none,
     * or one past the picture's end, is not it).
     */
    static byte[] loadingPicture(byte[] xbe) {
        int pixels = LOADING_WIDTH * LOADING_HEIGHT;
        byte[] greys = new byte[pixels];
        boolean found = false;

        for (int at = 0; at + 8 <= xbe.length && !found; at++) {
            if (u32(xbe, at) != LOADING_WIDTH || u32(xbe, at + 4) != LOADING_HEIGHT)
                continue;
            int pixel = 0;
            for (int next = at + 8; next < xbe.length && pixel < pixels; next++) {
                int run = (xbe[next] & 0xFF) >> 4;

                if (run == 0 || run > pixels - pixel)
                    break;
                Arrays.fill(greys, pixel, pixel + run, (byte) ((xbe[next] & 0x0F) << 4));
                pixel += run;
            }
            found = pixel == pixels;
        }
        if (!found)
            return null;
        byte[] tga = new byte[TGA_HEADER_SIZE + pixels * 3];
        tga[2] = 2;
        tga[12] = (byte) (LOADING_WIDTH & 0xFF);
        tga[13] = (byte) (LOADING_WIDTH >> 8);
        tga[14] = (byte) (LOADING_HEIGHT & 0xFF);
        tga[15] = (byte) (LOADING_HEIGHT >> 8);
        tga[16] = 24;
        for (int y = 0; y < LOADING_HEIGHT; y++) {
            int row = (LOADING_HEIGHT - 1 - y) * LOADING_WIDTH;
            for (int x = 0; x < LOADING_WIDTH; x++) {
                int out = TGA_HEADER_SIZE + (y * LOADING_WIDTH + x) * 3;

                tga[out] = tga[out + 1] = tga[out + 2] = greys[row + x];
            }
        }
        return tga;
    }

    /**
     * Writes default.xbe's picture to output, if it has one; whether it did.
     * One that cannot be written whole is removed (the loading screen would
     * read past one cut short).
     */
    private static boolean writeLoadingPicture(FileChannel image, long imageSize, long partition, Entry xbe,
            File output) {
        if (xbe == null || xbe.size <= 0 || xbe.size > MAXIMUM_XBE_SIZE
                || partition + xbe.sector * SECTOR_SIZE + xbe.size > imageSize)
            return false;
        try {
            ByteBuffer bytes = ByteBuffer.allocate((int) xbe.size);
            if (!readFully(image, bytes, partition + xbe.sector * SECTOR_SIZE))
                return false;
            byte[] tga = loadingPicture(bytes.array());
            if (tga == null)
                return false;
            try (FileOutputStream stream = new FileOutputStream(output)) {
                stream.write(tga);
            }
            return true;
        } catch (IOException | OutOfMemoryError exception) {
            output.delete();
            return false;
        }
    }

    /* ---------- the copy */

    private static String readFailure(Entry file) {
        return "Could not read " + file.name + " from the disc image (is it complete?).";
    }

    /** Copies one file of the image to output; returns the bytes copied of all the files so far. */
    private static long copyFile(FileChannel image, long partition, Entry file, File output, ByteBuffer buffer,
            long done, long total, Progress progress) throws IOException {
        String writeFailure = "Could not write " + output.getPath() + " (is the storage full?).";
        long position = partition + file.sector * SECTOR_SIZE;
        long remaining = file.size;
        FileOutputStream stream = null;

        try {
            stream = new FileOutputStream(output);
            FileChannel channel = stream.getChannel();
            while (remaining > 0) {
                int count = (int) Math.min(remaining, COPY_CHUNK);
                boolean read;

                buffer.clear();
                buffer.limit(count);
                try {
                    read = readFully(image, buffer, position);
                } catch (IOException exception) {
                    read = false;
                }
                if (!read)
                    throw new ExtractException(readFailure(file));
                buffer.flip();
                while (buffer.hasRemaining()) {
                    if (channel.write(buffer) <= 0)
                        throw new ExtractException(writeFailure);
                }
                position += count;
                remaining -= count;
                done += count;
                if (progress != null)
                    progress.report(file.name, done, total);
            }
            FileOutputStream closing = stream;
            stream = null;
            closing.close();
        } catch (ExtractException exception) {
            throw exception;
        } catch (IOException exception) {
            throw new ExtractException(writeFailure);
        } finally {
            if (stream != null) {
                try {
                    stream.close();
                } catch (IOException ignored) {
                    // (the copy failed already)
                }
            }
        }
        return done;
    }
}
