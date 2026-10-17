package com.halo.decomp;

import android.app.AlertDialog;
import android.content.Context;
import android.content.res.ColorStateList;
import android.graphics.Color;
import android.graphics.Typeface;
import android.graphics.drawable.GradientDrawable;
import android.text.InputType;
import android.text.TextUtils;
import android.util.DisplayMetrics;
import android.util.TypedValue;
import android.view.Gravity;
import android.view.KeyEvent;
import android.view.MotionEvent;
import android.view.View;
import android.widget.CheckBox;
import android.widget.EditText;
import android.widget.FrameLayout;
import android.widget.LinearLayout;
import android.widget.ScrollView;
import android.widget.SeekBar;
import android.widget.Switch;
import android.widget.TextView;
import android.widget.Toast;

import java.util.ArrayList;
import java.util.List;
import java.util.Locale;

public class TouchSettingsPanel extends ScrollView {
    private static final int ACCENT = 0xFF4FC3F7;
    private static final int DELETE_COLOR = 0xFFFF5252;

    private final TouchControls controls;
    private final float density;

    private final TextView titleText;
    private final TextView hintText;
    private final TextView statusSummaryText;

    // Global Section Container
    private final LinearLayout globalSection;
    private final Switch rumbleSwitch, gyroSwitch, floatStickSwitch;
    private final SeekBar masterOpacityBar, lookBar;
    private final TextView masterOpacityValue, lookValue;

    // Button-Specific Section Container
    private final LinearLayout buttonSection;
    private final SeekBar sizeBar, opacityBar;
    private final TextView sizeValue, opacityValue;
    private final TextView showPill, modePill, renamePill, iconPill, stickKnobPill, deletePill;
    private final LinearLayout stateBindingsContainer;
    private final TextView pressKeyPill, longPressKeyPill;

    private int boundControl = -1;
    private boolean updating;

    public TouchSettingsPanel(Context context, TouchControls controls) {
        super(context);
        this.controls = controls;
        this.density = context.getResources().getDisplayMetrics().density;

        setFillViewport(true);
        setVerticalScrollBarEnabled(true);
        setOverScrollMode(OVER_SCROLL_IF_CONTENT_SCROLLS);

        GradientDrawable card = new GradientDrawable();
        card.setColor(0xEB12181E);
        card.setCornerRadius(dp(16));
        card.setStroke(dp(1.5f), 0x5500E5FF);
        setBackground(card);
        setClickable(true);
        setVisibility(GONE);

        FrameLayout.LayoutParams flp = new FrameLayout.LayoutParams(dp(340), FrameLayout.LayoutParams.WRAP_CONTENT);
        flp.gravity = Gravity.TOP | Gravity.CENTER_HORIZONTAL;
        flp.topMargin = dp(16);
        setLayoutParams(flp);

        LinearLayout layout = new LinearLayout(context);
        layout.setOrientation(LinearLayout.VERTICAL);
        layout.setPadding(dp(14), dp(10), dp(14), dp(14));

        // Header: Drag handle title + Reset button
        LinearLayout header = new LinearLayout(context);
        header.setOrientation(LinearLayout.HORIZONTAL);
        header.setGravity(Gravity.CENTER_VERTICAL);

        titleText = text("Touch Controls", 12.5f, true, 0xFF00E5FF);
        titleText.setPadding(0, dp(4), dp(6), dp(4));
        header.addView(titleText, new LinearLayout.LayoutParams(0, LinearLayout.LayoutParams.WRAP_CONTENT, 1f));

        TextView resetBtn = pill("Reset", false);
        header.addView(resetBtn);
        layout.addView(header);

        makeHandle(titleText);

        hintText = text("", 11, false, 0xB3FFFFFF);
        hintText.setPadding(0, dp(4), 0, dp(4));
        layout.addView(hintText);

        statusSummaryText = text("", 10.5f, false, 0xFF81D4FA);
        statusSummaryText.setPadding(0, 0, 0, dp(6));
        layout.addView(statusSummaryText);

        // ================= GLOBAL SECTION (Shown when NO button is selected) =================
        globalSection = new LinearLayout(context);
        globalSection.setOrientation(LinearLayout.VERTICAL);

        rumbleSwitch = new Switch(context);
        rumbleSwitch.setText("Controls Vibration");
        rumbleSwitch.setTextColor(Color.WHITE);
        globalSection.addView(rumbleSwitch, margin(0, dp(2), 0, dp(2)));

        gyroSwitch = new Switch(context);
        gyroSwitch.setText("Gyroscope Aiming (Experimental)");
        gyroSwitch.setTextColor(Color.WHITE);
        globalSection.addView(gyroSwitch, margin(0, dp(2), 0, dp(2)));

        floatStickSwitch = new Switch(context);
        floatStickSwitch.setText("Floating Move Stick");
        floatStickSwitch.setTextColor(Color.WHITE);
        globalSection.addView(floatStickSwitch, margin(0, dp(2), 0, dp(4)));

        masterOpacityValue = text("", 11, false, 0xB3FFFFFF);
        lookValue = text("", 11, false, 0xB3FFFFFF);

        masterOpacityBar = slider(Math.round((TouchLayout.MAX_OPACITY - TouchLayout.MIN_OPACITY) * 100f));
        lookBar = slider(Math.round((TouchLayout.MAX_SENSITIVITY - TouchLayout.MIN_SENSITIVITY) * 100f));

        globalSection.addView(row("Master Opacity", masterOpacityBar, masterOpacityValue));
        globalSection.addView(row("Look Speed", lookBar, lookValue));

        LinearLayout addRow = new LinearLayout(context);
        addRow.setOrientation(LinearLayout.HORIZONTAL);
        addRow.setPadding(0, dp(6), 0, dp(4));

        TextView addBtn = pill("+ Add Button", false);
        TextView unhideBtn = pill("Show Hidden Buttons...", false);

        addRow.addView(addBtn, new LinearLayout.LayoutParams(0, LinearLayout.LayoutParams.WRAP_CONTENT, 1f));
        addRow.addView(spacing(dp(6)));
        addRow.addView(unhideBtn, new LinearLayout.LayoutParams(0, LinearLayout.LayoutParams.WRAP_CONTENT, 1f));
        globalSection.addView(addRow);

        layout.addView(globalSection);

        // ================= BUTTON SECTION (Shown when a specific button IS selected) =================
        buttonSection = new LinearLayout(context);
        buttonSection.setOrientation(LinearLayout.VERTICAL);

        sizeValue = text("", 11, false, 0xB3FFFFFF);
        opacityValue = text("", 11, false, 0xB3FFFFFF);

        sizeBar = slider(Math.round((TouchLayout.MAX_SIZE - TouchLayout.MIN_SIZE) * 100f));
        opacityBar = slider(Math.round((TouchLayout.MAX_OPACITY - TouchLayout.MIN_OPACITY) * 100f));

        buttonSection.addView(row("Button Size", sizeBar, sizeValue));
        buttonSection.addView(row("Button Opacity", opacityBar, opacityValue));

        LinearLayout toggles = new LinearLayout(context);
        toggles.setOrientation(LinearLayout.HORIZONTAL);
        showPill = pill("Visible", true);
        modePill = pill("Mode: Standard", false);
        toggles.addView(showPill);
        toggles.addView(spacing(dp(6)));
        toggles.addView(modePill);
        buttonSection.addView(toggles, margin(0, dp(6), 0, 0));

        stateBindingsContainer = new LinearLayout(context);
        stateBindingsContainer.setOrientation(LinearLayout.VERTICAL);

        LinearLayout rowKeys = new LinearLayout(context);
        rowKeys.setOrientation(LinearLayout.HORIZONTAL);
        pressKeyPill = pill("Tap: ...", false);
        longPressKeyPill = pill("Hold: ...", false);
        rowKeys.addView(pressKeyPill, new LinearLayout.LayoutParams(0, LinearLayout.LayoutParams.WRAP_CONTENT, 1f));
        rowKeys.addView(spacing(dp(6)));
        rowKeys.addView(longPressKeyPill, new LinearLayout.LayoutParams(0, LinearLayout.LayoutParams.WRAP_CONTENT, 1f));

        stateBindingsContainer.addView(rowKeys);
        buttonSection.addView(stateBindingsContainer, margin(0, dp(6), 0, 0));

        LinearLayout customRow = new LinearLayout(context);
        customRow.setOrientation(LinearLayout.HORIZONTAL);
        customRow.setGravity(Gravity.CENTER_VERTICAL);

        renamePill = pill("Rename", false);
        iconPill = pill("Skin: Default", false);
        stickKnobPill = pill("Knob Skin", false);
        deletePill = pill("Delete", false);
        styleDangerPill(deletePill);

        iconPill.setSingleLine(true);
        iconPill.setEllipsize(TextUtils.TruncateAt.MIDDLE);

        renamePill.setLayoutParams(new LinearLayout.LayoutParams(LinearLayout.LayoutParams.WRAP_CONTENT, LinearLayout.LayoutParams.WRAP_CONTENT));
        LinearLayout.LayoutParams iconParams = new LinearLayout.LayoutParams(0, LinearLayout.LayoutParams.WRAP_CONTENT, 1f);
        iconParams.setMargins(dp(4), 0, dp(4), 0);
        iconPill.setLayoutParams(iconParams);
        stickKnobPill.setLayoutParams(new LinearLayout.LayoutParams(LinearLayout.LayoutParams.WRAP_CONTENT, LinearLayout.LayoutParams.WRAP_CONTENT));
        deletePill.setLayoutParams(new LinearLayout.LayoutParams(LinearLayout.LayoutParams.WRAP_CONTENT, LinearLayout.LayoutParams.WRAP_CONTENT));

        customRow.addView(renamePill);
        customRow.addView(iconPill);
        customRow.addView(stickKnobPill);
        customRow.addView(spacing(dp(4)));
        customRow.addView(deletePill);
        buttonSection.addView(customRow, margin(0, dp(6), 0, 0));

        layout.addView(buttonSection);

        // ================= BOTTOM FOOTER =================
        LinearLayout layoutActions = new LinearLayout(context);
        layoutActions.setOrientation(LinearLayout.HORIZONTAL);
        layoutActions.setPadding(0, dp(10), 0, 0);

        TextView saveBtn = pill("Save Layout", true);
        TextView exportBtn = pill("Export", false);
        TextView importBtn = pill("Import", false);

        layoutActions.addView(saveBtn, new LinearLayout.LayoutParams(0, LinearLayout.LayoutParams.WRAP_CONTENT, 1.2f));
        layoutActions.addView(spacing(dp(6)));
        layoutActions.addView(exportBtn, new LinearLayout.LayoutParams(0, LinearLayout.LayoutParams.WRAP_CONTENT, 1f));
        layoutActions.addView(spacing(dp(6)));
        layoutActions.addView(importBtn, new LinearLayout.LayoutParams(0, LinearLayout.LayoutParams.WRAP_CONTENT, 1f));
        layout.addView(layoutActions);

        addView(layout);

        saveBtn.setOnClickListener(v -> {
            controls.stopEditing();
        });

        exportBtn.setOnClickListener(v -> ((HaloActivity) getContext()).chooseLayoutFile(true, controls.exportLayout()));
        importBtn.setOnClickListener(v -> ((HaloActivity) getContext()).chooseLayoutFile(false, controls.exportLayout()));

        addBtn.setOnClickListener(v -> showAddButtonDialog());
        unhideBtn.setOnClickListener(v -> showUnhideDialog());

        rumbleSwitch.setOnCheckedChangeListener((v, isChecked) -> {
            if (!updating) {
                controls.getLayout().rumbleEnabled = isChecked;
                controls.saveAndInvalidate();
            }
        });

        gyroSwitch.setOnCheckedChangeListener((v, isChecked) -> {
            if (!updating) {
                controls.getLayout().gyroscopeEnabled = isChecked;
                controls.saveAndInvalidate();
            }
        });

        floatStickSwitch.setOnCheckedChangeListener((v, isChecked) -> {
            if (!updating) {
                controls.getLayout().floatingStick = isChecked;
                controls.saveAndInvalidate();
            }
        });

        sizeBar.setOnSeekBarChangeListener(new Changed() {
            @Override
            void changed(int progress) {
                int sel = getActiveControl();
                if (sel >= 0) {
                    float s = TouchLayout.MIN_SIZE + (progress / 100f);
                    controls.getLayout().setSize(sel, s);
                    refreshValues();
                    controls.invalidate();
                }
            }
        });

        opacityBar.setOnSeekBarChangeListener(new Changed() {
            @Override
            void changed(int progress) {
                int sel = getActiveControl();
                if (sel >= 0) {
                    float op = TouchLayout.MIN_OPACITY + (progress / 100f);
                    controls.getLayout().setControlOpacity(sel, op);
                    refreshValues();
                    controls.invalidate();
                }
            }
        });

        masterOpacityBar.setOnSeekBarChangeListener(new Changed() {
            @Override
            void changed(int progress) {
                float op = TouchLayout.MIN_OPACITY + (progress / 100f);
                controls.getLayout().setOpacity(op);
                refreshValues();
                controls.invalidate();
            }
        });

        lookBar.setOnSeekBarChangeListener(new Changed() {
            @Override
            void changed(int progress) {
                float sens = TouchLayout.MIN_SENSITIVITY + (progress / 100f);
                controls.setSensitivity(sens);
                refreshValues();
            }
        });

        showPill.setOnClickListener(v -> {
            int sel = getActiveControl();
            if (sel >= 0) {
                if (controls.getLayout().type(sel) == TouchLayout.TB_HIDE) {
                    Toast.makeText(getContext(), "The Controls Toggle button must remain visible.", Toast.LENGTH_SHORT).show();
                    return;
                }
                controls.getLayout().setShown(sel, !controls.getLayout().shown(sel));
                refresh();
                controls.saveAndInvalidate();
            }
        });

        resetBtn.setOnClickListener(v -> handleSmartReset());

        deletePill.setOnClickListener(v -> {
            int sel = getActiveControl();
            if (sel >= 0) {
                if (controls.getLayout().type(sel) == TouchLayout.TB_HIDE) {
                    Toast.makeText(getContext(), "The Controls Toggle button cannot be deleted.", Toast.LENGTH_SHORT).show();
                    return;
                }
                controls.getLayout().removeControl(sel);
                controls.selectControl(-1);
                controls.saveAndInvalidate();
                refresh();
            }
        });

        modePill.setOnClickListener(v -> {
            int sel = getActiveControl();
            if (sel >= 0) {
                int type = controls.getLayout().type(sel);
                if (type != TouchLayout.LEFT && type != TouchLayout.TB_HIDE) {
                    int next = (controls.getLayout().mode(sel) + 1) % 3;
                    controls.getLayout().setMode(sel, next);
                    refresh();
                    controls.saveAndInvalidate();
                }
            }
        });

        pressKeyPill.setOnClickListener(v -> showActionOrKeycodePicker(false));
        longPressKeyPill.setOnClickListener(v -> showActionOrKeycodePicker(true));

        renamePill.setOnClickListener(v -> showRenameDialog());
        iconPill.setOnClickListener(v -> showCategorizedIconDialog(false));
        stickKnobPill.setOnClickListener(v -> showCategorizedIconDialog(true));

        refresh();
    }

    private int getActiveControl() {
        return boundControl >= 0 ? boundControl : controls.getSelectedControl();
    }

    public void bindControl(int controlIndex) {
        this.boundControl = controlIndex;
        refresh();
    }

    @Override
    protected void onMeasure(int widthMeasureSpec, int heightMeasureSpec) {
        DisplayMetrics dm = getContext().getResources().getDisplayMetrics();
        int maxHeight = (int) (dm.heightPixels * 0.90f);
        int constrainedHeight = MeasureSpec.makeMeasureSpec(maxHeight, MeasureSpec.AT_MOST);
        super.onMeasure(widthMeasureSpec, constrainedHeight);
    }

    private void showAddButtonDialog() {
        String[] options = {
            "Add Built-in Gamepad Button...",
            "Create Custom Button (Keycode & Label)..."
        };

        new AlertDialog.Builder(getContext())
            .setTitle("Add Button")
            .setItems(options, (d, which) -> {
                if (which == 0) showAddBuiltinButtonDialog();
                else showCreateCustomButtonDialog();
            })
            .setNegativeButton("Cancel", null)
            .show();
    }

    private void showAddBuiltinButtonDialog() {
        String[] names = new String[TouchLayout.BASE_COUNT];
        for (int i = 0; i < names.length; i++)
            names[i] = controls.controlName(i);

        new AlertDialog.Builder(getContext())
            .setTitle("Select Button to Add")
            .setItems(names, (d, type) -> {
                int added = controls.getLayout().add(type);
                if (added < 0) {
                    Toast.makeText(getContext(), "Cannot add duplicate stick or exceed maximum controls.", Toast.LENGTH_SHORT).show();
                } else {
                    controls.selectControl(added);
                    controls.saveAndInvalidate();
                    bindControl(added);
                }
            })
            .setNegativeButton("Cancel", null)
            .show();
    }

    private void showCreateCustomButtonDialog() {
        Context ctx = getContext();
        LinearLayout layout = new LinearLayout(ctx);
        layout.setOrientation(LinearLayout.VERTICAL);
        layout.setPadding(dp(18), dp(8), dp(18), dp(8));

        final EditText labelInput = new EditText(ctx);
        labelInput.setHint("Button Label (e.g. EXTRA, CROUCH)");
        labelInput.setSingleLine(true);
        layout.addView(labelInput);

        final EditText keyInput = new EditText(ctx);
        keyInput.setInputType(InputType.TYPE_CLASS_NUMBER);
        keyInput.setHint("Android Keycode (e.g. 62=Space, 31=C, 0=None)");
        keyInput.setSingleLine(true);
        layout.addView(keyInput);

        new AlertDialog.Builder(ctx)
            .setTitle("Create Custom Button")
            .setView(layout)
            .setPositiveButton("Create", (dialog, which) -> {
                String label = labelInput.getText().toString().trim();
                if (label.isEmpty()) label = "BTN";
                int code = 0;
                try {
                    code = Integer.parseInt(keyInput.getText().toString().trim());
                } catch (NumberFormatException ignored) {}

                int idx = controls.getLayout().addCustomButton(label, code);
                if (idx >= 0) {
                    controls.selectControl(idx);
                    controls.saveAndInvalidate();
                    bindControl(idx);
                    Toast.makeText(ctx, "Custom button created!", Toast.LENGTH_SHORT).show();
                } else {
                    Toast.makeText(ctx, "Maximum controls reached.", Toast.LENGTH_SHORT).show();
                }
            })
            .setNegativeButton("Cancel", null)
            .show();
    }

    private void showUnhideDialog() {
        final List<Integer> hiddenIndices = new ArrayList<>();
        final List<String> hiddenNames = new ArrayList<>();

        for (int i = 0; i < controls.getLayout().size(); i++) {
            if (!controls.getLayout().shown(i)) {
                hiddenIndices.add(i);
                hiddenNames.add(controls.controlName(controls.getLayout().type(i)));
            }
        }

        if (hiddenIndices.isEmpty()) {
            Toast.makeText(getContext(), "No buttons are currently hidden.", Toast.LENGTH_SHORT).show();
            return;
        }

        new AlertDialog.Builder(getContext())
            .setTitle("Unhide Button")
            .setItems(hiddenNames.toArray(new String[0]), (d, which) -> {
                int target = hiddenIndices.get(which);
                controls.getLayout().setShown(target, true);
                controls.selectControl(target);
                controls.saveAndInvalidate();
                bindControl(target);
                Toast.makeText(getContext(), hiddenNames.get(which) + " is now visible", Toast.LENGTH_SHORT).show();
            })
            .setNegativeButton("Cancel", null)
            .show();
    }

    // Direct, single confirmation without icon questions
    private void handleSmartReset() {
        final int sel = getActiveControl();
        if (sel >= 0 && sel < controls.getLayout().size()) {
            String name = controls.controlName(controls.getLayout().type(sel));
            new AlertDialog.Builder(getContext())
                .setTitle("Reset Button")
                .setMessage("Are you sure you want to reset " + name + " to default?")
                .setPositiveButton("Reset", (d, which) -> {
                    controls.getLayout().resetSingleControl(sel, true);
                    controls.refreshAllDrawables();
                    controls.saveAndInvalidate();
                    refresh();
                    Toast.makeText(getContext(), name + " reset to defaults", Toast.LENGTH_SHORT).show();
                })
                .setNegativeButton("Cancel", null)
                .show();
        } else {
            new AlertDialog.Builder(getContext())
                .setTitle("Reset Entire Layout")
                .setMessage("Are you sure you want to reset all controls to default layout?")
                .setPositiveButton("Reset", (d, which) -> {
                    controls.getLayout().resetAllDefaults(true);
                    controls.setTouchControlsHidden(false);
                    controls.refreshAllDrawables();
                    controls.saveAndInvalidate();
                    refresh();
                    Toast.makeText(getContext(), "All controls reset to defaults", Toast.LENGTH_SHORT).show();
                })
                .setNegativeButton("Cancel", null)
                .show();
        }
    }

    private void showActionOrKeycodePicker(final boolean forHold) {
        final int sel = getActiveControl();
        if (sel < 0) return;

        String[] options = {
            "Select Gamepad Action...",
            "Set Android Keycode...",
            "Clear / None"
        };

        new AlertDialog.Builder(getContext())
            .setTitle(forHold ? "Configure Hold Action" : "Configure Tap Action")
            .setItems(options, (d, which) -> {
                if (which == 0) {
                    showGamepadActionPicker(sel, forHold);
                } else if (which == 1) {
                    showCustomKeycodeDialog(sel, forHold);
                } else {
                    if (forHold) {
                        controls.getLayout().setLongType(sel, -1);
                        controls.getLayout().setLongKeycode(sel, -1);
                    } else {
                        controls.getLayout().setCustomKeycode(sel, -1);
                    }
                    controls.saveAndInvalidate();
                    refresh();
                }
            })
            .setNegativeButton("Cancel", null)
            .show();
    }

    private void showGamepadActionPicker(final int sel, final boolean forHold) {
        String[] names = new String[TouchLayout.BASE_COUNT];
        for (int i = 0; i < names.length; i++)
            names[i] = controls.controlName(i);

        new AlertDialog.Builder(getContext())
            .setTitle(forHold ? "Select Hold Action" : "Select Tap Action")
            .setItems(names, (d, which) -> {
                if (forHold) {
                    controls.getLayout().setLongType(sel, which);
                    controls.getLayout().setLongKeycode(sel, -1);
                } else {
                    controls.getLayout().setType(sel, which);
                    controls.getLayout().setCustomKeycode(sel, -1);
                }
                controls.saveAndInvalidate();
                refresh();
            })
            .setNegativeButton("Cancel", null)
            .show();
    }

    private void showCustomKeycodeDialog(final int sel, final boolean forHold) {
        EditText input = new EditText(getContext());
        input.setInputType(InputType.TYPE_CLASS_NUMBER);
        input.setHint("Android Keycode (e.g. 62 Space, 66 Enter)");

        int current = forHold ? controls.getLayout().longKeycode(sel) : controls.getLayout().customKeycode(sel);
        if (current > 0) input.setText(String.valueOf(current));

        new AlertDialog.Builder(getContext())
            .setTitle(forHold ? "Set Hold Keycode" : "Set Tap Keycode")
            .setView(input)
            .setPositiveButton("Set", (d, which) -> {
                try {
                    int val = Integer.parseInt(input.getText().toString().trim());
                    if (forHold) controls.getLayout().setLongKeycode(sel, val);
                    else controls.getLayout().setCustomKeycode(sel, val);
                    controls.saveAndInvalidate();
                    refresh();
                } catch (NumberFormatException ignored) {}
            })
            .setNeutralButton("Clear", (d, which) -> {
                if (forHold) controls.getLayout().setLongKeycode(sel, -1);
                else controls.getLayout().setCustomKeycode(sel, -1);
                controls.saveAndInvalidate();
                refresh();
            })
            .setNegativeButton("Cancel", null)
            .show();
    }

    private void showCategorizedIconDialog(final boolean isKnob) {
        final int sel = getActiveControl();
        if (sel < 0) return;

        String[] categories = {
            "Default Icons (Built-in)",
            "Custom Icons (Data Folder)",
            "Reset to Default"
        };

        new AlertDialog.Builder(getContext())
            .setTitle(isKnob ? "Select Joystick Knob Skin" : "Select Button Skin")
            .setItems(categories, (d, which) -> {
                if (which == 0) {
                    showBuiltinIconsSubDialog(sel, isKnob);
                } else if (which == 1) {
                    showCustomFilesSubDialog(sel, isKnob);
                } else {
                    applySkinFile(sel, isKnob, "");
                }
            })
            .setNegativeButton("Cancel", null)
            .show();
    }

    private void showBuiltinIconsSubDialog(final int sel, final boolean isKnob) {
        final String[] prettyLabels = {
            "Button A (Jump)", "Button B (Melee)", "Button X (Reload/Action)", "Button Y (Weapon Swap)",
            "Right Trigger (RT / Fire)", "Left Trigger (LT / Grenade)", "Left Stick Click (LS / Crouch)", "Right Stick Click (RS / Zoom)",
            "White Button (Flashlight)", "Black Button (Grenade Type)", "Start Button (Pause)", "Back Button (Scores)",
            "Controls Toggle (Show / Hide)", "Joystick Base Ring", "Joystick Knob (Nub)"
        };
        final String[] resourceKeys = {
            "tc_ic_a", "tc_ic_b", "tc_ic_x", "tc_ic_y",
            "tc_ic_rt", "tc_ic_lt", "tc_ic_ls", "tc_ic_rs",
            "tc_ic_white", "tc_ic_black", "tc_ic_start", "tc_ic_back",
            "tc_ic_hide", "tc_stick_base", "tc_stick_knob"
        };

        new AlertDialog.Builder(getContext())
            .setTitle("Default Built-in Icons")
            .setItems(prettyLabels, (d, which) -> {
                applySkinFile(sel, isKnob, resourceKeys[which]);
            })
            .setNegativeButton("Back", (d, which) -> showCategorizedIconDialog(isKnob))
            .show();
    }

    private void showCustomFilesSubDialog(final int sel, final boolean isKnob) {
        List<String> files = controls.listCustomExternalIcons();
        if (files.isEmpty()) {
            Toast.makeText(getContext(), "No image files found in icons folder.", Toast.LENGTH_LONG).show();
            return;
        }

        final String[] fileArray = files.toArray(new String[0]);
        new AlertDialog.Builder(getContext())
            .setTitle("Custom Icons (Data Folder)")
            .setItems(fileArray, (d, which) -> {
                applySkinFile(sel, isKnob, fileArray[which]);
            })
            .setNegativeButton("Back", (d, which) -> showCategorizedIconDialog(isKnob))
            .show();
    }

    private void applySkinFile(int sel, boolean isKnob, String skinPath) {
        if (isKnob) {
            controls.getLayout().setKnobFile(sel, skinPath);
        } else {
            controls.getLayout().setIconFile(sel, skinPath);
        }
        controls.refreshAllDrawables();
        controls.saveAndInvalidate();
        refresh();
    }

    private void showRenameDialog() {
        final int sel = getActiveControl();
        if (sel < 0) return;

        EditText input = new EditText(getContext());
        input.setText(controls.getLayout().customLabel(sel));

        new AlertDialog.Builder(getContext())
            .setTitle("Button Label")
            .setView(input)
            .setPositiveButton("Save", (d, which) -> {
                controls.getLayout().setCustomLabel(sel, input.getText().toString().trim());
                controls.saveAndInvalidate();
                refresh();
            })
            .setNegativeButton("Cancel", null)
            .show();
    }

    public void refresh() {
        int sel = getActiveControl();
        boolean hasSelectedButton = sel >= 0 && sel < controls.getLayout().size();

        updating = true;
        rumbleSwitch.setChecked(controls.getLayout().rumbleEnabled);
        gyroSwitch.setChecked(controls.getLayout().gyroscopeEnabled);
        floatStickSwitch.setChecked(controls.getLayout().floatingStick);

        if (hasSelectedButton) {
            globalSection.setVisibility(GONE);
            buttonSection.setVisibility(VISIBLE);

            titleText.setText(controls.controlName(controls.getLayout().type(sel)));
            hintText.setText("Drag button on screen to move, or tune settings below:");
        } else {
            buttonSection.setVisibility(GONE);
            globalSection.setVisibility(VISIBLE);

            titleText.setText("Touch Controls");
            hintText.setText("Tap any button on screen to select, move, and customize it.");
        }

        sizeBar.setEnabled(hasSelectedButton);
        sizeBar.setAlpha(hasSelectedButton ? 1f : 0.4f);
        opacityBar.setEnabled(hasSelectedButton);
        opacityBar.setAlpha(hasSelectedButton ? 1f : 0.4f);

        if (hasSelectedButton) {
            sizeBar.setProgress(Math.round((controls.getLayout().sizeScale(sel) - TouchLayout.MIN_SIZE) * 100f));
            opacityBar.setProgress(Math.round((controls.getLayout().controlOpacity(sel) - TouchLayout.MIN_OPACITY) * 100f));
        }
        masterOpacityBar.setProgress(Math.round((controls.getLayout().opacity - TouchLayout.MIN_OPACITY) * 100f));
        lookBar.setProgress(Math.round((controls.getSensitivity() - TouchLayout.MIN_SENSITIVITY) * 100f));

        boolean isStick = hasSelectedButton && controls.getLayout().type(sel) == TouchLayout.LEFT;
        boolean isHideBtn = hasSelectedButton && controls.getLayout().type(sel) == TouchLayout.TB_HIDE;
        boolean isGameplayButton = hasSelectedButton && !isStick && !isHideBtn;

        showPill.setVisibility(!isHideBtn && hasSelectedButton ? VISIBLE : GONE);
        showPill.setEnabled(!isHideBtn && hasSelectedButton);
        showPill.setAlpha(!isHideBtn && hasSelectedButton ? 1f : 0.4f);
        stylePill(showPill, hasSelectedButton && controls.getLayout().shown(sel));

        modePill.setVisibility(isGameplayButton ? VISIBLE : GONE);
        stateBindingsContainer.setVisibility(isGameplayButton ? VISIBLE : GONE);
        renamePill.setVisibility(!isStick && hasSelectedButton ? VISIBLE : GONE);
        stickKnobPill.setVisibility(isStick ? VISIBLE : GONE);
        iconPill.setVisibility(hasSelectedButton ? VISIBLE : GONE);
        deletePill.setVisibility(hasSelectedButton && !isHideBtn ? VISIBLE : GONE);

        if (isGameplayButton) {
            int mode = controls.getLayout().mode(sel);
            if (mode == TouchLayout.MODE_STANDARD) {
                modePill.setText("Mode: Standard");
                longPressKeyPill.setVisibility(GONE);
            } else if (mode == TouchLayout.MODE_TAP_AND_HOLD) {
                modePill.setText("Mode: Tap + Hold");
                longPressKeyPill.setVisibility(VISIBLE);
            } else {
                modePill.setText("Mode: Toggle");
                longPressKeyPill.setVisibility(GONE);
            }

            int ck = controls.getLayout().customKeycode(sel);
            int primaryType = controls.getLayout().type(sel);

            String primaryStr;
            if (ck > 0) {
                primaryStr = KeyEvent.keyCodeToString(ck) + " (" + ck + ")";
            } else {
                primaryStr = controls.controlName(primaryType);
            }
            pressKeyPill.setText("Tap: " + primaryStr);

            int lk = controls.getLayout().longKeycode(sel);
            int lt = controls.getLayout().longType(sel);
            String holdStr;
            if (lk > 0) {
                holdStr = KeyEvent.keyCodeToString(lk) + " (" + lk + ")";
            } else if (lt >= 0) {
                holdStr = controls.controlName(lt);
            } else {
                holdStr = "None";
            }
            longPressKeyPill.setText("Hold: " + holdStr);
        }

        if (hasSelectedButton) {
            String iconFile = controls.getLayout().iconFile(sel);
            String cleanIconName = iconFile.startsWith("tc_ic_") ? iconFile.substring(6).toUpperCase(Locale.US) : iconFile;
            if (isStick) {
                iconPill.setText(iconFile.isEmpty() ? "Base Skin: Default" : "Base: " + cleanIconName);
            } else {
                iconPill.setText(iconFile.isEmpty() ? "Skin: Default" : "Skin: " + cleanIconName);
            }

            StringBuilder sb = new StringBuilder();
            sb.append("Type: ").append(controls.controlName(controls.getLayout().type(sel)));
            if (controls.getLayout().customKeycode(sel) > 0) {
                sb.append(" | Key: ").append(controls.getLayout().customKeycode(sel));
            }
            if (controls.getLayout().rawControlOpacity(sel) >= 0) {
                sb.append(" | Opacity: ").append(Math.round(controls.getLayout().controlOpacity(sel) * 100)).append("%");
            } else {
                sb.append(" | Master Opacity");
            }
            statusSummaryText.setText(sb.toString());
            statusSummaryText.setVisibility(VISIBLE);
        } else {
            statusSummaryText.setVisibility(GONE);
        }

        updating = false;
        refreshValues();
    }

    private void refreshValues() {
        int sel = getActiveControl();
        if (sel >= 0 && sel < controls.getLayout().size()) {
            sizeValue.setText(Math.round(controls.getLayout().sizeScale(sel) * 100f) + "%");
            opacityValue.setText(Math.round(controls.getLayout().controlOpacity(sel) * 100f) + "%");
        } else {
            sizeValue.setText("");
            opacityValue.setText("");
        }
        masterOpacityValue.setText(Math.round(controls.getLayout().opacity * 100f) + "%");
        lookValue.setText(String.format(Locale.US, "%.2fx", controls.getSensitivity()));
    }

    private void makeHandle(View handle) {
        handle.setOnTouchListener(new OnTouchListener() {
            private float downX, downY, startX, startY;

            @Override
            public boolean onTouch(View v, MotionEvent e) {
                switch (e.getActionMasked()) {
                    case MotionEvent.ACTION_DOWN:
                        downX = e.getRawX();
                        downY = e.getRawY();
                        startX = getTranslationX();
                        startY = getTranslationY();
                        requestDisallowInterceptTouchEvent(true);
                        return true;
                    case MotionEvent.ACTION_MOVE:
                        setTranslationX(startX + e.getRawX() - downX);
                        setTranslationY(startY + e.getRawY() - downY);
                        return true;
                    case MotionEvent.ACTION_UP:
                    case MotionEvent.ACTION_CANCEL:
                        requestDisallowInterceptTouchEvent(false);
                        return true;
                    default:
                        return false;
                }
            }
        });
    }

    private abstract class Changed implements SeekBar.OnSeekBarChangeListener {
        abstract void changed(int progress);
        @Override public void onProgressChanged(SeekBar bar, int progress, boolean fromUser) {
            if (fromUser && !updating) changed(progress);
        }
        @Override public void onStartTrackingTouch(SeekBar bar) {}
        @Override public void onStopTrackingTouch(SeekBar bar) { controls.saveAndInvalidate(); }
    }

    private int dp(float value) { return Math.round(value * density); }

    private View spacing(int width) {
        View v = new View(getContext());
        v.setLayoutParams(new LinearLayout.LayoutParams(width, 1));
        return v;
    }

    private LinearLayout.LayoutParams margin(int l, int t, int r, int b) {
        LinearLayout.LayoutParams p = new LinearLayout.LayoutParams(
                LinearLayout.LayoutParams.MATCH_PARENT, LinearLayout.LayoutParams.WRAP_CONTENT);
        p.setMargins(l, t, r, b);
        return p;
    }

    private TextView text(String s, float sp, boolean bold, int color) {
        TextView t = new TextView(getContext());
        t.setText(s);
        t.setTextSize(TypedValue.COMPLEX_UNIT_SP, sp);
        t.setTextColor(color);
        if (bold) t.setTypeface(Typeface.DEFAULT_BOLD);
        return t;
    }

    private TextView pill(String s, boolean filled) {
        TextView t = text(s, 11, true, 0xFFFFFFFF);
        t.setGravity(Gravity.CENTER);
        t.setPadding(dp(8), dp(5), dp(8), dp(5));
        t.setClickable(true);
        stylePill(t, filled);
        return t;
    }

    private void stylePill(TextView t, boolean filled) {
        GradientDrawable g = new GradientDrawable();
        g.setCornerRadius(dp(16));
        if (filled) {
            g.setColor(ACCENT);
            t.setTextColor(0xFF06222E);
        } else {
            g.setColor(0x22FFFFFF);
            g.setStroke(dp(1), 0x66FFFFFF);
            t.setTextColor(0xFFFFFFFF);
        }
        t.setBackground(g);
    }

    private void styleDangerPill(TextView t) {
        GradientDrawable g = new GradientDrawable();
        g.setCornerRadius(dp(16));
        g.setColor(0x33FF5252);
        g.setStroke(dp(1), DELETE_COLOR);
        t.setTextColor(DELETE_COLOR);
        t.setBackground(g);
    }

    private SeekBar slider(int max) {
        SeekBar bar = new SeekBar(getContext());
        bar.setMax(max);
        bar.setProgressTintList(ColorStateList.valueOf(ACCENT));
        bar.setThumbTintList(ColorStateList.valueOf(ACCENT));
        return bar;
    }

    private LinearLayout row(String label, SeekBar bar, TextView value) {
        LinearLayout row = new LinearLayout(getContext());
        row.setOrientation(LinearLayout.HORIZONTAL);
        row.setGravity(Gravity.CENTER_VERTICAL);
        TextView name = text(label, 11, false, 0xFFFFFFFF);
        row.addView(name, new LinearLayout.LayoutParams(dp(84), LinearLayout.LayoutParams.WRAP_CONTENT));
        row.addView(bar, new LinearLayout.LayoutParams(0, LinearLayout.LayoutParams.WRAP_CONTENT, 1f));
        value.setGravity(Gravity.END);
        row.addView(value, new LinearLayout.LayoutParams(dp(44), LinearLayout.LayoutParams.WRAP_CONTENT));
        return row;
    }
}
