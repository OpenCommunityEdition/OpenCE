"""Generate GulchCE's opt-in Settings frontend; never rewrite CE menus.

Layout inspiration: OpenH1/OpenH1 df16d2e56c3b69be46f964f741a23fd2d3b4f57a.
Original GulchCE implementation and geometric art; no upstream code/assets copied.
"""
import json
from pathlib import Path
import xml.etree.ElementTree as ET

from PIL import Image, ImageDraw, ImageFont

ROOT = Path(__file__).resolve().parents[1]
MENUS = ROOT / "port/assets/menus"
BASE = "modern"
# Reuse the maintained setting inventory, without changing the CE generator.
from port_settings import SCREENS
import frontend_navigation
import frontend_multiplayer

# Each page: key, tab, parent, heading, source inventories. Page zero is the
# stable entry tag used by the main-menu policy.
PAGES = [
    ("", 0, 0, "DISPLAY", ["video_settings"]),
    ("audio", 1, 1, "AUDIO", ["audio_settings", "audio_settings/sound_processing"]),
    ("controls", 2, 2, "MOUSE AND KEYBOARD", ["mouse_settings"]),
    ("online", 3, 3, "ONLINE", ["network_setup"]),
    ("interface", 4, 4, "INTERFACE", ["controller_appearance"]),
    ("profile", 5, 5, "PLAYER PROFILE", ["gamepad_setup"]),
    ("graphics", 0, 0, "ADVANCED GRAPHICS", ["video_settings/graphics", "video_settings/extra_effects"]),
    ("first_person", 0, 0, "FIRST PERSON", ["video_settings/fov_viewmodels"]),
    ("lighting", 0, 6, "LIGHTING AND SHIELDS", ["video_settings/dynamic_lighting"]),
    ("bots", 3, 3, "BOTS", ["network_setup/bots"]),
    ("sharing", 3, 3, "MAP SHARING", ["network_setup/map_sharing"]),
    ("browser", 3, 3, "SERVER BROWSER", ["network_setup/server_browser"]),
    ("hud", 4, 4, "MULTIPLAYER HUD", ["network_setup/multiplayer_hud"]),
    ("streamer", 4, 4, "STREAMER MODE", ["network_setup/streamer_mode"]),
    ("touch", 2, 2, "TOUCH CONTROLS", ["gamepad_setup/android", "gamepad_setup/gyro"]),
    ("shoulder_camera", 0, 7, "SHOULDER CAMERA", ["video_settings/shoulder_camera"]),
]
TABS = ("VIDEO", "AUDIO", "CONTROLS", "ONLINE", "INTERFACE", "PROFILE")
LINKS = {0: [(6, None), (7, None)], 6: [(8, "desktop web")],
         3: [(9, None), (10, "desktop android"), (11, None)], 4: [(12, None), (13, None)],
         2: [(14, "android")], 7: [(15, None)]}
STYLE = ("MENU STYLE", "display.menu_frontend",
         [("MODERN", "modern"), ("CLASSIC PC", "pc"), ("XBOX", "xbox"), ("STOCK", "off")],
         "Choose the submenu style. The starting menu keeps its existing layout. The switch takes effect when you leave Settings for the main menu. Pause menus and map UI stay unchanged.", None)
INTERFACE_RENDERING = ("display.high_res_text", "display.high_res_hud")
EDITORS = {
    2: [("keyboard", "KEYBOARD BINDINGS", "Set the base game's keyboard and mouse bindings in its existing editor. That editor's OK saves bindings independently of these Settings."),
        ("gamepad", "CONTROLLER REMAPPING", "Remap each player's controller in the existing editor. That editor's OK saves bindings independently of these Settings.")],
    5: [("profiles", "SELECT PROFILE", "Select or create a player profile using the existing profile manager. Save or cancel any profile edits before changing profiles."),
        ("name", "PLAYER NAME", "Edit the selected profile's name. Settings OK saves the profile; Cancel discards its pending changes."),
        ("color", "PLAYER COLOR", "Edit the selected profile's multiplayer color. Settings OK saves the profile; Cancel discards its pending changes.")],
}

# The stock UI fonts have very different widths. Measure against the narrowest
# supported layout so captions never run into arrows at 4:3.
FONTS = {size: ImageFont.truetype(str(ROOT / "port/assets/fonts" / file), 128)
         for size, file in ((13, "Overpass-900.ttf"), (10, "Overpass-750.ttf"))}

def text_width(value, size):
    font = FONTS[size]
    box = font.getbbox("H")
    return font.getlength(value) * size / (box[3] - box[1]) * .98

def value_caption(value):
    if text_width(value, 10) <= 136: return value
    while value and text_width(value + "...", 10) > 136: value = value[:-1]
    return value.rstrip() + "..."

SHORT_LABELS = {
    "input.mouse_sensitivity": "HORIZONTAL SENS.", "input.mouse_vertical_sensitivity": "VERTICAL SENS.",
    "input.invert_mouse": "INVERT MOUSE", "network.allow_upnp": "UPNP FORWARDING",
    "network.join_from_clipboard": "CLIPBOARD INVITES", "input.auto_button_detection": "AUTO DETECTION",
    "display.passenger_third_person": "PASSENGER VIEW", "display.muzzle_lighting": "MUZZLE LIGHTS",
    "network.browser_show_empty": "EMPTY SERVERS", "display.scoreboard_team_layout": "SCOREBOARD STYLE",
    "input.gyro_sensitivity_horizontal": "HORIZONTAL SENS.", "input.gyro_sensitivity_vertical": "VERTICAL SENS.",
    "display.hud_button_style": "MANUAL STYLE", "display.hud_button_style_xbox": "XBOX STYLE",
    "display.hud_button_style_ps": "PLAYSTATION STYLE", "display.hud_button_style_switch": "SWITCH STYLE",
}

def page_base(index):
    return "modern" + ("/" + PAGES[index][0] if PAGES[index][0] else "")


def document():
    doc = ET.Element("menus")
    def widget(path, **attrs):
        return ET.SubElement(doc, "widget", name=path, **{k: str(v) for k, v in attrs.items() if v is not None})
    def child(parent, path, x=0, y=0, platform=None):
        attrs = dict(widget=path, x=str(x), y=str(y))
        if platform: attrs["platform"] = platform
        ET.SubElement(parent, "child", **attrs)
    def event(parent, key, callback):
        ET.SubElement(parent, "on", event=key, run=callback)
    def activate(parent, callback):
        for key in ("a", "start"): event(parent, key, callback)
        event(parent, "left_mouse", "mouse emit accept event")
    def text(path, value, width, height=24, **attrs):
        return widget(path, type="text", text=value, width=width, height=height,
                      font="ui\\large_ui", color="#FF2896FF", **attrs)
    def strings(path, values):
        node = ET.SubElement(doc, "strings", name=path)
        for value in values:
            ET.SubElement(node, "string", text=" ".join(value.split()))

    for page, (_, tab, parent, heading, inventories) in enumerate(PAGES):
        base = page_base(page)
        w = lambda name, **attrs: widget(base + "/" + name, **attrs)
        c = lambda node, name, *args, **attrs: child(node, base + "/" + name, *args, **attrs)
        screen = w("settings", width=640, height=480, bitmap="modern/panel", flags="pass_unhandled_to_focused_child")
        ET.SubElement(screen, "on", event="created", run="main menu intialize", label="ring_loop")
        event(screen, "created", "port frontend init")
        for key in ("b", "back"): event(screen, key, "port frontend back")
        event(screen, "y", "port frontend next tab")
        for name, x, y in (("title",70,40), ("section",32,108), ("rule_display",28,130),
                           ("help_panel",380,106), ("options",0,0), ("help_title",396,119),
                           ("help_value",396,138), ("status",28,389), ("prompts",28,450)):
            c(screen, name, x, y)
        w("title", width=200, height=28, bitmap="modern/title", controller=1)
        text(base+"/section", heading,336,controller=1).set("color", "#FF80A2C0")
        w("rule_display",width=336,height=1,bitmap="modern/rule",controller=1)
        w("help_panel",width=232,height=294,bitmap="modern/help_panel",controller=1,flags="nifty_fx")
        w("help_title",type="text",controller=1,width=200,height=22,string_list=base+"/labels",font="ui\\large_ui",color="#FF80A2C0")
        w("help_value",type="text",controller=1,width=200,height=24,font="ui\\large_ui",color="#FFDDDDDD")
        w("status",type="text",controller=1,width=336,height=20,font="ui\\small_ui",color="#FF80A2C0")
        text(base+"/prompts"," ",580,22).set("font","ui\\small_ui")
        w("help",type="text",controller=1,left=396,top=165,width=200,height=222,string_list=base+"/help_strings",font="ui\\large_ui",color="#FF2896FF")
        options=w("options",type="column_list",width=640,height=480,flags="pass_unhandled_to_focused_child up_down_tabs_children",description=base+"/help")
        ET.SubElement(options,"data",input="port frontend update")
        c(options,"tabs",28,76)
        tabs=w("tabs",type="column_list",width=584,height=26,flags="pass_unhandled_to_focused_child left_right_tabs_items")
        for number, caption in enumerate(TABS):
            c(tabs,f"tab_{number}",number*98)
            button=text(base+f"/tab_{number}",caption,94,26,align="center",text_y=3,
                        bitmap="modern/tab" if number==tab else None,flags="nifty_fx")
            # Six tabs fit 4:3 without reducing the approved control height.
            button.set("font","ui\\small_ui")
            activate(button,"port frontend page")
        rows=[]
        if page==4:
            rows.append(STYLE)
            rows.extend(next(row for row in SCREENS["video_settings/graphics"]["rows"] if row[1] == key)
                        for key in INTERFACE_RENDERING)
        for inventory in inventories:
            rows.extend(row for row in SCREENS[inventory]["rows"] if row[1] not in INTERFACE_RENDERING)
        # Labels/help are local to each page; dynamic spinner captions come from
        # the real instance, including enumerated monitors and audio devices.
        labels=[]; helps=["OK saves pending settings across pages. Defaults resets this page. Cancel discards pending settings. Tab or Y changes tabs."]
        for index, (label, setting, choices, help_text, platform, *_) in enumerate(rows):
            key = "style" if setting=="display.menu_frontend" else setting.replace(".","_")+str(index)
            label = SHORT_LABELS.get(setting, label).rstrip(":")
            if setting == "input.auto_button_detection" or setting.startswith("display.hud_button_style"):
                help_text = help_text.replace("Requires High-Res HUD in Graphics.", "Gameplay artwork requires High-Res HUD; menu glyphs work independently.").replace("Requires High-Res HUD.", "Gameplay artwork requires High-Res HUD; menu glyphs work independently.")
            labels.append(label.rstrip(":")); helps.append(help_text)
            c(options,f"row_{key}",28,137+index*30,platform=platform)
            row=w(f"row_{key}",width=336,height=26,bitmap="modern/row",platform=platform,flags="pass_unhandled_to_focused_child nifty_fx")
            c(row,f"label_{key}",12,3); c(row,f"spinner_{key}",180,3)
            w(f"label_{key}",type="text",controller=1,width=152,height=24,string_list=base+"/labels",string_index=index,font="ui\\large_ui",color="#FF2896FF")

            if choices is None:
                # Reuse the existing read-only master-server status service.
                row.remove(row.findall("child")[1])
                c(row,"master_servers_value",180,3)
                w("master_servers_value",type="text",controller=1,width=140,height=20,font="ui\\large_ui",color="#FFDDDDDD",align="center")
                continue
            spinner=w(f"spinner_{key}",type="spinner",width=140,height=20,flags="pass_unhandled_to_focused_child left_right_tabs_items",
                      strings="|".join(value_caption(caption) for caption,_ in choices),values="|".join(value for _,value in choices),setting=setting,
                      font="ui\\large_ui",color="#FF2896FF",align="center",
                      header_bitmap="bitmaps/arrow_sm_left",footer_bitmap="bitmaps/arrow_sm_right",header_bounds="4 -10 16 -4",footer_bounds="4 144 16 150")
            event(spinner,"created","port setting load")
        actions=[(f"link_{target}",PAGES[target][3]+" >",f"Open {PAGES[target][3].lower()} settings. Pending changes stay until OK or Cancel.",platform,"port frontend page") for target,platform in LINKS.get(page,[])]
        actions += [(key,caption+" >",help_text,None,"port frontend editor") for key,caption,help_text in EDITORS.get(page,[])]
        if page>=6: actions.append((f"link_{parent}","< BACK","Return to the parent page, keeping pending changes.",None,"port frontend page"))
        for key,label,help_text,platform,callback in actions:
            index=len(labels); labels.append(label); helps.append(help_text)
            c(options,"row_"+key,28,137+index*30,platform=platform)
            row=w("row_"+key,width=336,height=26,bitmap="modern/row",platform=platform,flags="nifty_fx")
            activate(row,callback)
            c(row,"label_"+key,12,3)
            w("label_"+key,type="text",controller=1,width=312,height=24,string_list=base+"/labels",string_index=index,font="ui\\large_ui",color="#FF2896FF")
        strings(base+"/labels",labels); strings(base+"/help_strings",helps)
        c(options,"button_bar",26,414)
        bar=w("button_bar",type="column_list",width=588,height=26,flags="pass_unhandled_to_focused_child left_right_tabs_items")
        for key,caption,x in (("defaults","DEFAULTS",0),("apply","OK",332),("cancel","CANCEL",462)):
            c(bar,"button_"+key,x)
            button=text(base+"/button_"+key,caption,124,26,align="center",text_y=3,bitmap="modern/button",flags="nifty_fx")
            activate(button,"port frontend "+key)
    frontend_navigation.append(doc)
    frontend_multiplayer.append(doc)
    entry=widget("modern/entry",type="column_list",width=184,height=24,flags="pass_unhandled_to_focused_child")
    event(entry,"up down","port frontend focus"); child(entry,"modern/entry_button")
    button=text("modern/entry_button"," ",184,24,bitmap="bitmaps/text_button_background")
    button.set("font","ui\\small_ui"); activate(button,"port frontend open")
    for name,width,height in ART:
        bitmap=ET.SubElement(doc,"bitmap",name="modern/"+name)
        for frame in range(1 if name in ("panel","help_panel","rule") or name.startswith("title") else 2):
            ET.SubElement(bitmap,"frame",png=f"modern/{name}_{frame}.png",width=str(width),height=str(height))
    ET.indent(doc,space="\t")
    return ET.tostring(doc,encoding="unicode")+"\n"


ART = (("panel", 640, 480), ("help_panel", 232, 294), ("row", 336, 26),
       ("button", 124, 26), ("tab", 94, 26), ("title", 200, 28), ("rule", 336, 1),
       ("title_multiplayer", 300, 28), ("title_host", 300, 28)) + tuple(
           (name, 300, 28) for name in frontend_multiplayer.TITLES)


def generate(folder=MENUS):
    target = folder / "modern"
    target.mkdir(parents=True, exist_ok=True)
    (target / "settings.xml").write_text('<?xml version="1.0" encoding="UTF-8"?>\n<!-- Generated by tools/frontend_menu.py -->\n' + document(), encoding="utf-8")
    if Path(folder).resolve() == MENUS.resolve():
        frontend_multiplayer.write_metadata(*frontend_multiplayer.append(ET.Element("menus")))
    for name, width, height in ART:
        for frame in range(1 if name in ("panel", "help_panel", "rule") or name.startswith("title") else 2):
            # Four samples per menu unit keep the native renderer's outlines smooth.
            scale = 4
            image = Image.new("RGBA", (width * scale, height * scale), (0, 0, 0, 0))
            draw = ImageDraw.Draw(image)
            blue = (40, 150, 255, 255)
            def box(bounds, **kwargs):
                draw.rectangle(tuple(round(v * scale) for v in bounds), **kwargs)
            def rounded(bounds, radius, **kwargs):
                draw.rounded_rectangle(tuple(round(v * scale) for v in bounds), round(radius * scale), **kwargs)
            if name == "panel":
                for y in range(70, 410):
                    alpha = round(172 - 85 * (y - 70) / 340)
                    box((0, y, width, y + 1), fill=(0, 20, 46, alpha))
                box((0, 410, width, 444), fill=(0, 0, 0, 175))
                box((0, 70, width, 70.5), fill=blue)
                box((0, 410, width, 410.5), fill=blue)
            elif name.startswith("title"):
                font = ImageFont.truetype(str(ROOT / "port/assets/fonts/OpenCE-Regular.ttf"), 128)
                caption = "SETTINGS" if name == "title" else frontend_multiplayer.TITLES[name] if name in frontend_multiplayer.TITLES else frontend_navigation.SCREENS[name[6:]][0]
                bounds = font.getbbox(caption)
                ink = Image.new("RGBA", (bounds[2] - bounds[0], bounds[3] - bounds[1]), (0, 0, 0, 0))
                ImageDraw.Draw(ink).text((-bounds[0], -bounds[1]), caption, font=font, fill=blue)
                title_width = 184 if name == "title" else min(width, round(22 * ink.width / ink.height))
                ink = ink.resize((title_width * scale, 22 * scale), Image.Resampling.LANCZOS)
                image.alpha_composite(ink, (0, 4 * scale))
            elif name == "rule":
                box((0, 0, width, 1), fill=(35, 90, 140, 255))
            elif name == "help_panel":
                rounded((1, 1, width - 1, height - 1), 10, fill=(0, 12, 30, 225), outline=blue, width=2 * scale)
            elif name in ("button", "tab") or frame:
                rounded((1, 1, width - 1, height - 1), 12, fill=(3, 35, 65, 220), outline=blue, width=2 * scale)
                if name == "button" and frame:
                    rounded((2, 2, width - 2, height - 2), 11, outline=(166, 218, 255, 255), width=scale)
            image.save(target / f"{name}_{frame}.png")
    manifest = folder / "menus.json"
    data = json.loads(manifest.read_text(encoding="utf-8"))
    files = [name for name in data["files"] if not name.startswith("modern/")]
    data["files"] = files + sorted(path.relative_to(folder).as_posix() for path in target.iterdir()
                                   if path.suffix in (".xml", ".png"))
    manifest.write_text(json.dumps(data, indent=1) + "\n", encoding="utf-8")


if __name__ == "__main__":
    generate()
    header = "/* Generated by tools/frontend_menu.py; page routes, never legacy widgets. */\n"
    header += "static struct { char const *path; short tab, parent; } const frontend_pages[] = {\n"
    for index, (_, tab, parent, _, _) in enumerate(PAGES):
        path = "pc\\" + (page_base(index) + "/settings").replace("/", "\\")
        header += "    {" + json.dumps(path) + f", {tab}, {parent}" + "},\n"
    header += "};\n"
    (ROOT / "port/linux/game/frontend_pages.inl").write_text(header, encoding="utf-8")
