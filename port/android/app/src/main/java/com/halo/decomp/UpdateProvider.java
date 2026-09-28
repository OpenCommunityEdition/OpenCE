package com.halo.decomp;

import android.content.ContentProvider;
import android.content.ContentValues;
import android.database.Cursor;
import android.database.MatrixCursor;
import android.net.Uri;
import android.os.ParcelFileDescriptor;
import android.provider.OpenableColumns;

import java.io.File;
import java.io.FileNotFoundException;

/**
 * Hands Android's package installer the downloaded new version of the app
 * (Updater), read-only, the one file content://com.halo.decomp.update/halo.apk.
 * (The app has no AndroidX, whose FileProvider does the same.)
 */
public class UpdateProvider extends ContentProvider {
    static final String AUTHORITY = "com.halo.decomp.update";
    static final String DIRECTORY = "update";
    static final String APK = "halo.apk";

    private File apk() {
        return new File(new File(getContext().getCacheDir(), DIRECTORY), APK);
    }

    private boolean isApk(Uri uri) {
        return uri != null && APK.equals(uri.getLastPathSegment());
    }

    @Override
    public boolean onCreate() {
        return true;
    }

    @Override
    public ParcelFileDescriptor openFile(Uri uri, String mode) throws FileNotFoundException {
        if (!isApk(uri) || !"r".equals(mode))
            throw new FileNotFoundException(uri.toString());
        return ParcelFileDescriptor.open(apk(), ParcelFileDescriptor.MODE_READ_ONLY);
    }

    @Override
    public String getType(Uri uri) {
        return isApk(uri) ? "application/vnd.android.package-archive" : null;
    }

    @Override
    public Cursor query(Uri uri, String[] projection, String selection, String[] selectionArgs, String sortOrder) {
        if (!isApk(uri))
            return null;
        MatrixCursor cursor = new MatrixCursor(new String[] { OpenableColumns.DISPLAY_NAME, OpenableColumns.SIZE });
        cursor.addRow(new Object[] { APK, apk().length() });
        return cursor;
    }

    @Override
    public Uri insert(Uri uri, ContentValues values) {
        return null;
    }

    @Override
    public int delete(Uri uri, String selection, String[] selectionArgs) {
        return 0;
    }

    @Override
    public int update(Uri uri, ContentValues values, String selection, String[] selectionArgs) {
        return 0;
    }
}
