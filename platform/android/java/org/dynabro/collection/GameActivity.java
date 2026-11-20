package org.dynabro.collection;

import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Paint;
import android.graphics.Typeface;
import android.graphics.drawable.GradientDrawable;
import android.os.Bundle;
import android.view.Gravity;
import android.view.KeyEvent;
import android.view.MotionEvent;
import android.view.View;
import android.view.ViewGroup;
import android.widget.FrameLayout;
import android.widget.LinearLayout;
import android.widget.TextView;

import org.libsdl.app.SDLActivity;

import java.util.ArrayList;
import java.util.List;

public final class GameActivity extends SDLActivity {
    private final List<HeldButton> buttons = new ArrayList<>();
    private DirectionPad directions;
    private FrameLayout controls;

    @Override protected String[] getLibraries() {
        int game = getIntent().getIntExtra("game", 0);
        return new String[]{"SDL2", LibraryActivity.engine(game)};
    }

    @Override protected String[] getArguments() {
        nativeSetenv("GENESIS_USER_DIR", getIntent().getStringExtra("profile"));
        return new String[]{"--no-launcher", getIntent().getStringExtra("rom")};
    }

    @Override protected void onCreate(Bundle state) {
        super.onCreate(state);
        if (mLayout == null) return;
        controls = new FrameLayout(this);
        controls.setMotionEventSplittingEnabled(true);
        mLayout.addView(controls, new ViewGroup.LayoutParams(-1, -1));

        directions = new DirectionPad();
        FrameLayout.LayoutParams dpad = new FrameLayout.LayoutParams(dp(126), dp(126), Gravity.BOTTOM | Gravity.LEFT);
        dpad.setMargins(dp(16), 0, 0, dp(14));
        controls.addView(directions, dpad);

        LinearLayout face = new LinearLayout(this);
        for (HeldButton button : new HeldButton[]{
                new HeldButton("A", KeyEvent.KEYCODE_Z),
                new HeldButton("B", KeyEvent.KEYCODE_X),
                new HeldButton("C", KeyEvent.KEYCODE_C)}) {
            LinearLayout.LayoutParams placement = new LinearLayout.LayoutParams(dp(48), dp(48));
            placement.setMarginStart(dp(6));
            face.addView(button, placement);
        }
        FrameLayout.LayoutParams facePosition = new FrameLayout.LayoutParams(-2, -2, Gravity.BOTTOM | Gravity.RIGHT);
        facePosition.setMargins(0, 0, dp(16), dp(24));
        controls.addView(face, facePosition);

        FrameLayout.LayoutParams start = new FrameLayout.LayoutParams(dp(64), dp(38), Gravity.BOTTOM | Gravity.CENTER_HORIZONTAL);
        start.bottomMargin = dp(14);
        controls.addView(new HeldButton("Start", KeyEvent.KEYCODE_ENTER), start);

        FrameLayout.LayoutParams menu = new FrameLayout.LayoutParams(dp(64), dp(38), Gravity.TOP | Gravity.RIGHT);
        menu.setMargins(0, dp(12), dp(16), 0);
        controls.addView(new HeldButton("Menu", KeyEvent.KEYCODE_ESCAPE), menu);

        TextView toggle = label("Touch");
        toggle.setContentDescription("Show or hide touch controls");
        toggle.setOnClickListener(view -> {
            releaseControls();
            controls.setVisibility(controls.getVisibility() == View.VISIBLE ? View.GONE : View.VISIBLE);
        });
        FrameLayout.LayoutParams togglePosition = new FrameLayout.LayoutParams(dp(64), dp(38), Gravity.TOP | Gravity.LEFT);
        togglePosition.setMargins(dp(16), dp(12), 0, 0);
        FrameLayout fullWindow = new FrameLayout(this);
        fullWindow.addView(toggle, togglePosition);
        mLayout.addView(fullWindow, new ViewGroup.LayoutParams(-1, -1));
    }

    @Override protected void onPause() {
        releaseControls();
        super.onPause();
    }

    @Override public void onBackPressed() {
        if (controls == null) {
            super.onBackPressed();
            return;
        }
        onNativeKeyDown(KeyEvent.KEYCODE_ESCAPE);
        onNativeKeyUp(KeyEvent.KEYCODE_ESCAPE);
    }

    private void releaseControls() {
        if (directions != null) directions.setMask(0);
        for (HeldButton button : buttons) button.setHeld(false);
    }

    private int dp(int value) { return Math.round(value * getResources().getDisplayMetrics().density); }

    private void style(TextView view, String label) {
        view.setText(label);
        view.setTextSize(15);
        view.setTextColor(Color.WHITE);
        view.setGravity(Gravity.CENTER);
        GradientDrawable background = new GradientDrawable();
        background.setColor(0xbb17202b);
        background.setStroke(dp(1), 0xddffffff);
        background.setCornerRadius(dp(22));
        view.setBackground(background);
    }

    private TextView label(String text) {
        TextView view = new TextView(this);
        style(view, text);
        return view;
    }

    private final class HeldButton extends TextView {
        private final int key;
        private boolean held;

        HeldButton(String label, int key) {
            super(GameActivity.this);
            this.key = key;
            style(this, label);
            setClickable(true);
            buttons.add(this);
        }

        @Override public boolean performClick() {
            super.performClick();
            return true;
        }

        @Override public boolean onTouchEvent(MotionEvent event) {
            switch (event.getActionMasked()) {
                case MotionEvent.ACTION_DOWN:
                    setHeld(true);
                    return true;
                case MotionEvent.ACTION_UP:
                    setHeld(false);
                    performClick();
                    return true;
                case MotionEvent.ACTION_CANCEL:
                    setHeld(false);
                    return true;
                default:
                    return true;
            }
        }

        void setHeld(boolean down) {
            if (held == down) return;
            held = down;
            setAlpha(down ? 0.65f : 1f);
            if (down) onNativeKeyDown(key);
            else onNativeKeyUp(key);
        }
    }

    private final class DirectionPad extends View {
        private final int[] keys = {KeyEvent.KEYCODE_DPAD_UP, KeyEvent.KEYCODE_DPAD_DOWN,
                KeyEvent.KEYCODE_DPAD_LEFT, KeyEvent.KEYCODE_DPAD_RIGHT};
        private final String[] arrows = {"▲", "▼", "◀", "▶"};
        private final Paint paint = new Paint(Paint.ANTI_ALIAS_FLAG);
        private int mask;

        DirectionPad() {
            super(GameActivity.this);
            setContentDescription("Directional pad");
        }

        @Override public boolean onTouchEvent(MotionEvent event) {
            switch (event.getActionMasked()) {
                case MotionEvent.ACTION_DOWN:
                case MotionEvent.ACTION_MOVE:
                    float x = event.getX() / getWidth() - 0.5f;
                    float y = event.getY() / getHeight() - 0.5f;
                    int next = 0;
                    if (Math.abs(y) > 0.11f && Math.abs(y) > Math.abs(x) * 0.58f) next |= y < 0 ? 1 : 2;
                    if (Math.abs(x) > 0.11f && Math.abs(x) > Math.abs(y) * 0.58f) next |= x < 0 ? 4 : 8;
                    setMask(next);
                    return true;
                case MotionEvent.ACTION_UP:
                case MotionEvent.ACTION_CANCEL:
                    setMask(0);
                    return true;
                default:
                    return true;
            }
        }

        void setMask(int next) {
            if (mask == next) return;
            for (int i = 0; i < keys.length; i++) {
                int bit = 1 << i;
                if (((mask ^ next) & bit) == 0) continue;
                if ((next & bit) != 0) onNativeKeyDown(keys[i]);
                else onNativeKeyUp(keys[i]);
            }
            mask = next;
            invalidate();
        }

        @Override protected void onDraw(Canvas canvas) {
            paint.setColor(0xbb17202b);
            canvas.drawCircle(getWidth() * 0.5f, getHeight() * 0.5f, getWidth() * 0.48f, paint);
            paint.setTextAlign(Paint.Align.CENTER);
            paint.setTextSize(dp(23));
            paint.setTypeface(Typeface.DEFAULT_BOLD);
            for (int i = 0; i < arrows.length; i++) {
                float x = i < 2 ? 0.5f : i == 2 ? 0.22f : 0.78f;
                float y = i > 1 ? 0.56f : i == 0 ? 0.30f : 0.82f;
                paint.setColor((mask & (1 << i)) != 0 ? 0xffeac85d : Color.WHITE);
                canvas.drawText(arrows[i], getWidth() * x, getHeight() * y, paint);
            }
        }
    }
}
