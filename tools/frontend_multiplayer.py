"""Independent modern views of the existing multiplayer widget contracts.

Read the generated CE declarations as a service inventory (names, callbacks,
strings and lifecycle), never modify them. Only child/description dependencies
are copied: opening an editor remains a native service handoff. Presentation
and viewport metadata belong exclusively to this generator.
"""
from copy import deepcopy
from pathlib import Path
import json
import re
import xml.etree.ElementTree as ET

ROOT = Path(__file__).resolve().parents[1]
MP = "main_menu/multiplayer_type_select/"
PAGES = {
    "maps": (MP + "connected/connected_map_select_wrapper", "SELECT MAP", "MAPS"),
    "gametypes": (MP + "connected/gametype_select_screen_wrapper", "GAMETYPE", "GAME RULES"),
    "server": (MP + "server_settings/server_settings_screen", "SERVER SETUP", "HOST OPTIONS"),
    "session": (MP + "server_settings/session_options/session_options_screen", "SESSION OPTIONS", "COMPATIBILITY"),
    "lobby": (MP + "lobby/lobby_screen", "GAME LOBBY", "PLAYERS"),
    "preview": (MP + "lobby/preview_screen", "JOIN GAME", "GAME IN PROGRESS"),
    "bots": ("main_menu/settings_select/player_setup/player_profile_edit/network_setup/bots/bots_screen", "BOTS", "BOT OPTIONS"),
}
TITLES = {"title_" + key: title for key, (_, title, _) in PAGES.items()}


def inventory():
    return {w.get("name"): w for p in sorted((ROOT / "port/assets/menus/ce").glob("*.xml"))
            for w in ET.parse(p).getroot().findall("widget")}


def append(doc):
    source = inventory()
    roles = []
    routes = []
    def put(node, **attrs):
        node.attrib.update({k: str(v) for k, v in attrs.items()})
    def leaf(path):
        return path.rsplit("/", 1)[-1]
    def add(kind, name, **attrs):
        return ET.SubElement(doc, kind, name=name, **{k: str(v) for k,v in attrs.items()})
    def role(widget, value):
        roles.append((widget.get("name"), value))
    for page, (original, caption, heading) in PAGES.items():
        base = "modern/network/" + page
        copies = {}
        def copy(path):
            if path in copies:
                return copies[path]
            w = deepcopy(source[path])
            # Preserve semantic leaf names, including repeated names in modals.
            w.set("name", base + "/" + str(len(copies)) + "/" + leaf(path))
            copies[path] = w
            doc.append(w)
            for c in w.findall("child"):
                c.set("widget", copy(c.get("widget")).get("name"))
            if w.get("description"):
                w.set("description", copy(w.get("description")).get("name"))
            w.set("flags", " ".join(f for f in w.get("flags", "").split() if f != "pause_game"))
            return w
        root = copy(original)
        routes.append((original, root.get("name")))
        by_name = {w.get("name"): w for w in copies.values()}
        # The native wrappers retain their B/back and server-defer lifecycle.
        screens = [w for w in copies.values() if w.get("bitmap") in ("bitmaps/gradient", "bitmaps/gradient_big")]
        body = next(iter(screens), root)
        for w in screens:
            w.set("bitmap", "modern/panel")
            role(w, "full")
        for w in copies.values():
            name = leaf(w.get("name"))
            if w.get("width")=="640" and w.get("height")=="480": role(w,"full")
            children = w.findall("child")
            for c in children:
                target = by_name[c.get("widget")]
                n = leaf(target.get("name"))
                if n == "sel_list_desc_bkd" or n == "current_profile_name" or (n == "header" or n.startswith("header_")) and w is body:
                    w.remove(c)
            # Lists keep child order: existing services address rows by index.
            data = {c.get("input") for c in w.findall("data")}
            scroll = bool(data & {"mp map list update", "gt select list update", "server settings update", "port lobby update", "game settings lists text update", "port settings help"})
            if scroll:
                ET.SubElement(w, "data", input="port frontend multiplayer update")
                index = 0
                for c in w.findall("child"):
                    target = by_name[c.get("widget")]
                    n = leaf(target.get("name"))
                    if re.match(r"list_item_\d", n) or n.startswith("op_"):
                        put(c, x=28, y=110+30*index)
                        put(target, left=0, top=0, width=336, height=26, bitmap="modern/row")
                        target.set("flags", target.get("flags", "") + " nifty_fx")
                        role(target, "row")
                        index += 1
                        for inner in target.findall("child"):
                            item = by_name[inner.get("widget")]
                            item_name = leaf(item.get("name"))
                            if item_name in ("list_item_arrows", "scroll_up_button", "scroll_down_button"):
                                target.remove(inner)
                                continue
                            if item.get("type") == "spinner" or item_name.endswith("_value"):
                                chooser = n.startswith("list_item_0")
                                value_width = 296 if chooser else 140 if page == "session" else 98
                                put(inner, x=20 if chooser else 180 if page == "session" else 218, y=3)
                                put(item, left=0, top=0, width=value_width, height=24, text_x=0, text_y=0, font="ui\\large_ui", align="center")
                                if item.get("type") == "spinner":
                                    put(item, header_bounds="5 -12 17 -6", footer_bounds=f"5 {value_width} 17 {value_width + 6}")
                                    if item_name == "session_compatibility_spinner":
                                        # The native service reads indices; short captions fit
                                        # the modern value column even at 4:3.
                                        put(item, strings="CROSSPLAY|THIRD PERSON")
                                role(item, "chooser" if chooser else "value")
                            elif item.get("type") == "text":
                                put(inner, x=12, y=3)
                                put(item, left=0, top=0, width=312 if item_name=="list_item_text" else 152 if page == "session" else 196, height=24, text_x=0, text_y=0, font="ui\\large_ui", align="left")
                                if item_name == "session_compatibility_label":
                                    put(item, text="COMPATIBILITY")
                                role(item, "label_full" if item_name=="list_item_text" else "label")
            if name.endswith("button_bar") or name == "button_bar":
                put(w, left=0, top=0, width=588, height=26)
                role(w, "bar")
                count = len(w.findall("child"))
                slot = min(130, 588 // max(1, count))
                for i,c in enumerate(w.findall("child")):
                    put(c, x=588-slot*(count-i), y=0)
                    button = by_name[c.get("widget")]
                    put(button, left=0, top=0, width=slot-6, height=26, bitmap="modern/button", text_y=3, font="ui\\small_ui")
                    button.set("flags", button.get("flags", "")+" nifty_fx")
                    role(button, "button")
            for c in w.findall("child"):
                if leaf(c.get("widget")).endswith("button_bar"):
                    put(c, x=26, y=414)
            if name.endswith("_right_item"):
                put(w, left=0, top=0, width=640, height=480)
                w.attrib.pop("bitmap", None)
                for c in w.findall("child"):
                    put(c,x=0,y=0)
            if name.endswith("_extended_desc") or name == "lobby_desc":
                for c in w.findall("child"):
                    put(c,x=0,y=0)
            if name.endswith("_right_pic") or name == "lobby_map_pic":
                put(w,left=426,top=104,width=140,height=114)
                role(w,"detail_fixed")
            if name.endswith("_right_name") or name in ("lobby_map_name", "mcc_folder_name"):
                put(w,left=396,top=232,width=200,height=42,font="ui\\large_ui")
                role(w,"detail")
            if name.endswith("_right_data") or name in ("lobby_game_data", "mcc_folder_help"):
                put(w,left=396,top=280,width=200,height=110,font="ui\\small_ui")
                role(w,"detail")
            if name == "lobby_join_help":
                put(w,left=28,top=387,width=584,height=20,align="left",text_y=0)
                role(w,"status")
            if name == "preview_status":
                put(w,left=40,top=120,width=312,height=260,font="ui\\large_ui")
                role(w,"heading")
            if w.get("type") == "text" and name == "help":
                put(w,left=396,top=174 if page in ("server", "session") else 108,width=200,height=212 if page in ("server", "session") else 278,font="ui\\large_ui",color="#FF2896FF")
                role(w,"detail_wrap")
        if page in ("server", "session"):
            w=add("widget",base+"/help_value",type="text",controller=1,left=396,top=108,width=200,height=56,font="ui\\large_ui",color="#FFFFFFFF")
            ET.SubElement(body,"child",widget=w.get("name"))
            role(w,"detail_wrap")
        # The fixed chrome is not inserted into service lists (row indices).
        parent, title, chrome_base = body, page, base
        for n,x,y,width,height,bitmap in [("title",70,40,300,28,"modern/title_"+title), ("help_panel",380,82,232,318,"modern/help_panel"), ("rule",28,104,336,1,"modern/rule")]:
            w=add("widget",chrome_base+"/"+n,left=x,top=y,width=width,height=height,controller=1,bitmap=bitmap,flags="nifty_fx" if n=="help_panel" else "")
            parent.insert(0,ET.Element("child",widget=w.get("name")))
            role(w,"detail" if n=="help_panel" else "heading")
        for n,x,y,width,text in [("section",32,82,332,heading),("prompts",28,450,584," "),("status",28,387,336,"")]:
            w=add("widget",chrome_base+"/"+n,type="text",left=x,top=y,width=width,height=22,controller=1,text=text,font="ui\\small_ui" if n!="section" else "ui\\large_ui",color="#FF80A2C0")
            ET.SubElement(parent,"child",widget=w.get("name"))
            role(w,"status" if n=="prompts" else "heading")
    # Remove declarations whose children were deliberately replaced, so legacy
    # decorative bitmaps and unused captions do not accumulate in the new set.
    generated={w.get("name"):w for w in doc.findall("widget") if w.get("name").startswith("modern/network/")}
    reachable=set()
    def visit(name):
        if name in reachable:return
        reachable.add(name)
        w=generated[name]
        for c in w.findall("child"):visit(c.get("widget"))
        if w.get("description"):visit(w.get("description"))
    for _,name in routes:visit(name)
    for name,w in generated.items():
        if name not in reachable:doc.remove(w)
    return [(n,r) for n,r in roles if n in reachable], routes


def write_metadata(roles, routes):
    # Geometry roles are read-only; no pointers or scrolling state survive a map.
    unique = dict(roles)
    types = list(dict.fromkeys(unique.values()))
    text = "/* Generated by tools/frontend_menu.py. Modern multiplayer views only. */\n"
    text += "enum { " + ", ".join("FRONTEND_NET_"+v.upper() for v in types) + " };\n"
    text += "static struct { char const *name; int role; } const frontend_network_widgets[] = {\n"
    for name,value in unique.items():
        text += "    {"+json.dumps("pc\\"+name.replace("/","\\"))+", FRONTEND_NET_"+value.upper()+"},\n"
    text += "};\n"
    (ROOT/"port/linux/game/frontend_network_layout.inl").write_text(text,encoding="utf-8")
    text = "/* Generated by tools/frontend_menu.py; only explicitly migrated roots. */\n"
    text += "static struct { char const *legacy, *modern; } const frontend_network_routes[] = {\n"
    for old,new in routes:
        text += "    {"+json.dumps("pc\\"+old.replace("/","\\"))+", "+json.dumps("pc\\"+new.replace("/","\\"))+"},\n"
    text += "};\n"
    (ROOT/"port/linux/game/frontend_network_routes.inl").write_text(text,encoding="utf-8")
