package com.halo.decomp;

import org.libtorrent4j.AlertListener;
import org.libtorrent4j.SessionManager;
import org.libtorrent4j.TorrentInfo;
import org.libtorrent4j.alerts.Alert;
import org.libtorrent4j.alerts.AlertType;
import org.libtorrent4j.alerts.MetadataReceivedAlert;
import org.libtorrent4j.alerts.TorrentErrorAlert;
import org.libtorrent4j.swig.torrent_flags_t;

import java.io.File;
import java.io.IOException;
import java.nio.file.Files;

/**
 * Downloads the game data from a magnet link or a .torrent file the player
 * supplies, with libtorrent (libtorrent4j).
 *
 * No torrent ships with the app and none is looked up: what to download is
 * the player's choice and responsibility.
 *
 * The session's own counters are polled rather than a TorrentHandle: holding
 * one across threads crashes inside libtorrent's native handle.
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
        total = info.totalSize();
        metadata = true;
        prepare();
        try {
            manager.download(info, saveDirectory);
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
                        try {
                            total = ((MetadataReceivedAlert) alert).handle().torrentFile().totalSize();
                            metadata = true;
                        } catch (Throwable ignored) {
                            // (the size stays unknown; the bar runs indeterminate)
                        }
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
