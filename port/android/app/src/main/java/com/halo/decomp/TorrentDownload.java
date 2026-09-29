package com.halo.decomp;

import org.libtorrent4j.AlertListener;
import org.libtorrent4j.FileStorage;
import org.libtorrent4j.Priority;
import org.libtorrent4j.SessionManager;
import org.libtorrent4j.TorrentHandle;
import org.libtorrent4j.TorrentInfo;
import org.libtorrent4j.alerts.Alert;
import org.libtorrent4j.alerts.AlertType;
import org.libtorrent4j.alerts.MetadataReceivedAlert;
import org.libtorrent4j.alerts.TorrentErrorAlert;
import org.libtorrent4j.swig.torrent_flags_t;

import java.io.File;
import java.io.IOException;
import java.nio.file.Files;
import java.util.Locale;

/**
 * Downloads the game data from a magnet link or a .torrent file the player
 * supplies, with libtorrent (libtorrent4j).
 *
 * Only the disc image is wanted, so a multi-file torrent's .iso/.xiso files
 * are selected and everything else is ignored — archive.org items, for
 * instance, usually pair the image with other files.
 *
 * The session's own counters are polled rather than a TorrentHandle: one
 * held across threads crashes inside libtorrent's native handle. (A handle
 * is used inside the metadata alert, where the alert still owns it.)
 */
final class TorrentDownload {
    interface Progress {
        /** called as the download goes; returns false to stop */
        boolean report(String state, long done, long total, long rate);
    }

    private final File saveDirectory;
    private final Progress progress;
    private final SessionManager manager = new SessionManager();
    private volatile long total;
    private volatile Priority[] priorities;
    private volatile boolean metadata;
    private volatile String error;
    private volatile boolean finished;
    private volatile boolean cancelled;

    TorrentDownload(File saveDirectory, Progress progress) {
        this.saveDirectory = saveDirectory;
        this.progress = progress;
    }

    void cancel() {
        cancelled = true;
    }

    /** downloads a magnet link; returns null on success, or why it stopped */
    String runMagnet(String magnetUri) {
        prepare();
        try {
            manager.download(magnetUri, saveDirectory, new torrent_flags_t());
        } catch (RuntimeException exception) {
            manager.stop();
            return message(exception, "that magnet link is not usable");
        }
        return await();
    }

    /** downloads a .torrent file; returns null on success, or why it stopped */
    String runFile(File torrentFile) {
        TorrentInfo info;
        try {
            info = new TorrentInfo(Files.readAllBytes(torrentFile.toPath()));
        } catch (IOException | RuntimeException exception) {
            return message(exception, "that torrent file could not be read");
        }
        if (!info.isValid())
            return "that torrent file is not valid";
        select(info);
        prepare();
        try {
            manager.download(info, saveDirectory, null, priorities, null, new torrent_flags_t());
        } catch (RuntimeException exception) {
            manager.stop();
            return message(exception, "that torrent file is not usable");
        }
        return await();
    }

    private void prepare() {
        if (!saveDirectory.isDirectory())
            saveDirectory.mkdirs();
        manager.start();
        manager.addListener(new AlertListener() {
            @Override
            public int[] types() {
                return new int[] {
                    AlertType.METADATA_RECEIVED.swig(),
                    AlertType.TORRENT_FINISHED.swig(),
                    AlertType.TORRENT_ERROR.swig(),
                };
            }

            @Override
            public void alert(Alert<?> alert) {
                switch (alert.type()) {
                    case METADATA_RECEIVED:
                        received((MetadataReceivedAlert) alert);
                        break;
                    case TORRENT_FINISHED:
                        finished = true;
                        break;
                    case TORRENT_ERROR:
                        TorrentErrorAlert failure = (TorrentErrorAlert) alert;
                        error = failure.error() != null ? failure.error().getMessage() : "the torrent failed";
                        finished = true;
                        break;
                    default:
                        break;
                }
            }
        });
    }

    /**
     * A magnet has no file list until its metadata arrives, so the disc
     * image is picked there. The handle is only touched inside the alert.
     */
    private void received(MetadataReceivedAlert alert) {
        try {
            TorrentHandle handle = alert.handle();
            TorrentInfo info = handle.torrentFile();
            if (info != null && info.isValid()) {
                select(info);
                if (priorities != null)
                    handle.prioritizeFiles(priorities);
            }
            metadata = true;
        } catch (Throwable ignored) {
            // (the size stays unknown; the bar runs indeterminate)
        }
    }

    /**
     * Selects one disc image and ignores the rest, or null when the torrent
     * has none (then nothing is filtered). A "rev N" in the torrent's name
     * picks that revision; otherwise the largest image wins.
     */
    private void select(TorrentInfo info) {
        FileStorage files = info.files();
        int chosen = bestDiscImage(files, info.name());
        android.util.Log.i("halo-import", "torrent \"" + info.name() + "\" -> "
            + (chosen >= 0 ? files.fileName(chosen) : "no disc image"));
        if (chosen < 0) {
            total = info.totalSize();
            priorities = null;
        } else {
            /* the selected image's size, not the whole torrent's: the rest is
               ignored, so the bar would otherwise never fill */
            total = files.fileSize(chosen);
            priorities = Priority.array(Priority.IGNORE, files.numFiles());
            priorities[chosen] = Priority.DEFAULT;
        }
        metadata = true;
    }

    private static int bestDiscImage(FileStorage files, String torrentName) {
        String wanted = revision(torrentName);
        int best = -1;
        long bestSize = -1;
        for (int index = 0; index < files.numFiles(); index++) {
            if (!isDiscImage(files.fileName(index)))
                continue;
            if (wanted != null && wanted.equals(revision(files.fileName(index))))
                return index;
            long size = files.fileSize(index);
            if (size > bestSize) {
                bestSize = size;
                best = index;
            }
        }
        return best;
    }

    /** the N of a "rev N" in a name, or null */
    private static String revision(String name) {
        java.util.regex.Matcher matcher = java.util.regex.Pattern
            .compile("rev[ ._-]*(\\d+)", java.util.regex.Pattern.CASE_INSENSITIVE).matcher(name);
        return matcher.find() ? matcher.group(1) : null;
    }

    private static boolean isDiscImage(String name) {
        String lower = name.toLowerCase(Locale.US);
        return lower.endsWith(".iso") || lower.endsWith(".xiso");
    }

    /** polls until the download ends, the player stops it, or a timeout gives up */
    private String await() {
        long lastProgress = System.currentTimeMillis();
        long lastDone = -1;
        while (!finished && !cancelled) {
            long done = manager.totalDownload();
            long rate = manager.downloadRate();
            if (progress != null
                && !progress.report(metadata ? "downloading" : "finding peers", done, metadata ? total : 0, rate))
                cancelled = true;
            if (done != lastDone) {
                lastDone = done;
                lastProgress = System.currentTimeMillis();
            } else if (System.currentTimeMillis() - lastProgress > 120000) {
                error = "no data arrived for two minutes";
                break;
            }
            try {
                Thread.sleep(500);
            } catch (InterruptedException exception) {
                cancelled = true;
            }
        }
        manager.stop();
        if (error != null)
            return error;
        return cancelled ? "stopped" : null;
    }

    private static String message(Throwable exception, String fallback) {
        return exception.getMessage() != null ? exception.getMessage() : fallback;
    }
}
