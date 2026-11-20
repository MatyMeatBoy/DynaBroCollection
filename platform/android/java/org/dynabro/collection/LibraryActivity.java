package org.dynabro.collection;

import android.app.Activity;
import android.content.Intent;
import android.content.res.AssetManager;
import android.graphics.Color;
import android.os.Bundle;
import android.system.ErrnoException;
import android.system.Os;
import android.view.ViewGroup;
import android.widget.Button;
import android.widget.LinearLayout;
import android.widget.ScrollView;
import android.widget.TextView;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;

public final class LibraryActivity extends Activity {
    private static final String[] GAMES = {"Dyna Brothers", "Dyna Brothers 2 Special"};
    private static final String[] ENGINES = {"DynaBrothers1AOT", "DynaBrothers2AOT"};
    private static final int PICK_ROM = 100;
    private static final int MAX_ROM_SIZE = 8 * 1024 * 1024;
    private static native boolean validateFirstRom(String path);
    private static native boolean validateSecondRom(String path);

    private final ExecutorService worker = Executors.newSingleThreadExecutor();
    private final Button[] play = new Button[2];
    private final Button[] choose = new Button[2];
    private TextView status;
    private boolean busy;

    static String engine(int game) { return ENGINES[game]; }

    private File romFile(int game) {
        return new File(getFilesDir(), "game-" + (game + 1) + ".md");
    }

    @Override public void onCreate(Bundle state) {
        super.onCreate(state);
        getWindow().setStatusBarColor(Color.rgb(17, 22, 29));
        getWindow().setNavigationBarColor(Color.rgb(17, 22, 29));

        ScrollView scroll = new ScrollView(this);
        LinearLayout library = new LinearLayout(this);
        library.setOrientation(LinearLayout.VERTICAL);
        library.setPadding(dp(24), dp(48), dp(24), dp(28));
        library.setBackgroundColor(Color.rgb(17, 22, 29));
        scroll.setFillViewport(true);
        scroll.addView(library);
        setContentView(scroll);

        library.addView(label("DynaBroCollection", 30));
        library.addView(label("Choose a Mega Drive ROM you own to play.", 16));
        for (int game = 0; game < GAMES.length; game++) {
            final int slot = game;
            TextView title = label(GAMES[game], 22);
            title.setPadding(0, dp(26), 0, 0);
            library.addView(title);
            LinearLayout row = new LinearLayout(this);
            play[game] = new Button(this);
            play[game].setText("Play");
            play[game].setOnClickListener(view -> launch(slot));
            choose[game] = new Button(this);
            choose[game].setOnClickListener(view -> pick(slot));
            row.addView(choose[game], new LinearLayout.LayoutParams(0, ViewGroup.LayoutParams.WRAP_CONTENT, 1));
            row.addView(play[game], new LinearLayout.LayoutParams(0, ViewGroup.LayoutParams.WRAP_CONTENT, 1));
            library.addView(row);
        }
        status = label("", 16);
        status.setAccessibilityLiveRegion(TextView.ACCESSIBILITY_LIVE_REGION_POLITE);
        status.setPadding(0, dp(24), 0, 0);
        library.addView(status);
        refresh();
    }

    private int dp(int value) { return Math.round(value * getResources().getDisplayMetrics().density); }

    private TextView label(String text, int size) {
        TextView view = new TextView(this);
        view.setTextColor(Color.WHITE);
        view.setText(text);
        view.setTextSize(size);
        return view;
    }

    private void refresh() {
        for (int game = 0; game < GAMES.length; game++) {
            boolean imported = romFile(game).isFile();
            choose[game].setText(imported ? "Replace ROM" : "Choose ROM");
            choose[game].setEnabled(!busy);
            play[game].setEnabled(!busy && imported);
        }
    }

    private void pick(int game) {
        Intent picker = new Intent(Intent.ACTION_OPEN_DOCUMENT);
        picker.setType("*/*");
        picker.addCategory(Intent.CATEGORY_OPENABLE);
        startActivityForResult(picker, PICK_ROM + game);
    }

    @Override protected void onActivityResult(int request, int result, Intent data) {
        super.onActivityResult(request, result, data);
        int game = request - PICK_ROM;
        if (game < 0 || game >= GAMES.length || result != RESULT_OK || data == null || data.getData() == null) return;

        busy = true;
        status.setText("Checking ROM…");
        refresh();
        worker.execute(() -> {
            String message;
            File pending = null;
            try {
                pending = File.createTempFile("rom-", ".tmp", getCacheDir());
                try (InputStream input = getContentResolver().openInputStream(data.getData());
                     FileOutputStream output = new FileOutputStream(pending)) {
                    if (input == null) throw new IOException("File provider returned no stream");
                    byte[] buffer = new byte[16384];
                    int count, total = 0;
                    while ((count = input.read(buffer)) != -1) {
                        total += count;
                        if (total > MAX_ROM_SIZE) throw new IOException("ROM exceeds 8 MiB");
                        output.write(buffer, 0, count);
                    }
                    output.getFD().sync();
                }
                System.loadLibrary(ENGINES[game]);
                String path = pending.getAbsolutePath();
                boolean valid = game == 0 ? validateFirstRom(path) : validateSecondRom(path);
                if (valid) {
                    Os.rename(path, romFile(game).getAbsolutePath());
                    message = GAMES[game] + " is ready to play.";
                } else {
                    message = "This ROM is incompatible with " + GAMES[game] + ".";
                }
            } catch (IOException | ErrnoException | UnsatisfiedLinkError | SecurityException ex) {
                android.util.Log.e("DynaBroCollection", "ROM import failed", ex);
                message = "The ROM could not be imported. Try a readable Mega Drive ROM file.";
            } finally {
                if (pending != null) pending.delete();
            }
            final String completed = message;
            runOnUiThread(() -> {
                if (isDestroyed()) return;
                status.setText(completed);
                busy = false;
                refresh();
            });
        });
    }

    private void launch(int game) {
        busy = true;
        status.setText("Opening " + GAMES[game] + "…");
        refresh();
        worker.execute(() -> {
            try {
                String version = getPackageManager().getPackageInfo(getPackageName(), 0).versionName;
                if (!version.equals(getPreferences(MODE_PRIVATE).getString("asset-version", ""))) {
                    copyAssets(getAssets(), "assets", new File(getFilesDir(), "assets"));
                    getPreferences(MODE_PRIVATE).edit().putString("asset-version", version).apply();
                }
                File profile = new File(getFilesDir(), "profile");
                if (!profile.mkdirs() && !profile.isDirectory()) throw new IOException("Cannot create profile");
                Intent intent = new Intent(this, GameActivity.class);
                intent.putExtra("game", game);
                intent.putExtra("rom", romFile(game).getAbsolutePath());
                intent.putExtra("profile", profile.getAbsolutePath());
                runOnUiThread(() -> {
                    if (isDestroyed()) return;
                    busy = false;
                    status.setText("");
                    refresh();
                    startActivity(intent);
                });
            } catch (Exception ex) {
                android.util.Log.e("DynaBroCollection", "Game preparation failed", ex);
                runOnUiThread(() -> {
                    if (isDestroyed()) return;
                    busy = false;
                    status.setText("The game files could not be prepared. Check available storage and try again.");
                    refresh();
                });
            }
        });
    }

    private static void copyAssets(AssetManager assets, String source, File destination) throws IOException {
        String[] children = assets.list(source);
        if (children != null && children.length > 0) {
            if (!destination.mkdirs() && !destination.isDirectory()) throw new IOException("Cannot create " + destination);
            for (String child : children) copyAssets(assets, source + "/" + child, new File(destination, child));
        } else {
            try (InputStream input = assets.open(source); FileOutputStream output = new FileOutputStream(destination)) {
                byte[] buffer = new byte[16384];
                int count;
                while ((count = input.read(buffer)) != -1) output.write(buffer, 0, count);
            }
        }
    }

    @Override protected void onDestroy() {
        worker.shutdown();
        super.onDestroy();
    }
}
