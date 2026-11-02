"""Modern landing screens; existing services retain their editors and lifecycle."""
import xml.etree.ElementTree as ET

MP = "main_menu/multiplayer_type_select"
HOST = MP + "/connected/connected_map_select_wrapper"
BROWSER = MP + "/join_game/join_game_screen"

# Section, caption, key, destination, callback(s), help.
# Multiplayer mode callbacks use the documented create_inter/create_lan/join_lan/
# join_direct tokens in the instance name. Keep those tokens within 31 characters.
SCREENS = {
    "multiplayer": ("MULTIPLAYER", "PLAY ONLINE", [
        (None, "SERVER BROWSER", "join_internet_item", BROWSER, ["mp type set mode"], "Browse public internet games, view their details, and join a server."),
        (None, "LOCAL NETWORK", "join_lan_item", BROWSER, ["mp type set mode"], "Find and join games on your local network."),
        (None, "JOIN BY LINK", "join_direct_item", BROWSER, ["mp type set mode"], "Join a game using its invite link. Paste the link into the Direct Link screen."),
        ("HOST", "CREATE GAME >", "host", "modern/host/screen", [], "Host over the internet or your local network. Choose a multiplayer map or a single-player level for online campaign co-op."),
        (None, "SPLIT-SCREEN CO-OP >", "coop", MP + "/coop/player_2_profile_screen", ["port coop begin"], "Play the campaign with another player on this machine. Player two chooses a profile with their own controller. For online co-op, use Create Game."),
        ("GAME RULES", "EDIT GAMETYPES >", "gametypes", "main_menu/settings_select/multiplayer_setup/playlist_select/gametype_select_screen", [], "Create or edit saved multiplayer gametypes and their rules."),
    ]),
    "host": ("CREATE GAME", "HOST", [
        (None, "INTERNET", "create_internet_item", HOST, ["join controller to mp game", "mp type set mode"], "Host a game online. Choose a map and gametype, then set visibility, invite links and bots before entering the lobby."),
        (None, "LOCAL NETWORK", "create_lan_item", HOST, ["join controller to mp game", "mp type set mode"], "Host on your local network only. Choose a multiplayer map or a single-player level for campaign co-op."),
    ]),
}


def append(doc):
    def node(kind, **attrs):
        return ET.SubElement(doc, kind, **{k: str(v) for k, v in attrs.items() if v is not None})
    for page, (title, heading, rows) in SCREENS.items():
        base = "modern/" + page
        def widget(name, **attrs):
            return node("widget", name=base + "/" + name, **attrs)
        def child(parent, name, x=0, y=0):
            return ET.SubElement(parent, "child", widget=base + "/" + name, x=str(x), y=str(y))
        def text(name, **attrs):
            return widget(name, type="text", font="ui\\large_ui", color="#FF2896FF", **attrs)
        screen = widget("screen", width=640, height=480, bitmap="modern/panel", flags="pass_unhandled_to_focused_child main_menu_if_no_history")
        # Returning from a cancelled host/browser abandons that session. Use
        # the native shell cleanup before the guarded navigation initializer.
        ET.SubElement(screen, "on", event="created", run="main menu intialize")
        ET.SubElement(screen, "on", event="created", run="port frontend navigation init")
        for key in ("b", "back"):
            ET.SubElement(screen, "on", event=key, back="true")
        child(screen, "title", 70, 40)
        child(screen, "help_panel", 310, 82)
        child(screen, "options")
        child(screen, "help_title", 326, 96)
        child(screen, "status", 28, 389)
        child(screen, "prompts", 28, 450)
        widget("title", width=300, height=28, bitmap="modern/title_" + page, controller=1)
        widget("help_panel", width=302, height=318, bitmap="modern/help_panel", controller=1, flags="nifty_fx")
        text("help_title", width=270, height=24, controller=1, string_list=base+"/labels").set("color", "#FF80A2C0")
        text("help", left=326, top=135, width=270, height=250, controller=1, string_list=base+"/help_strings")
        text("status", width=272, height=20, controller=1).set("font", "ui\\small_ui")
        text("prompts", width=584, height=22, text=" ").set("font", "ui\\small_ui")
        options = widget("options", type="column_list", width=640, height=480, flags="pass_unhandled_to_focused_child up_down_tabs_children", description=base+"/help")
        ET.SubElement(options, "data", input="port frontend navigation update")
        labels = node("strings", name=base+"/labels")
        helps = node("strings", name=base+"/help_strings")
        ET.SubElement(helps, "string", text="Choose an option. Up and Down navigate; Back returns to the previous screen.")
        y = 82
        for index, (section, caption, key, target, callbacks, help_text) in enumerate(rows):
            if index == 0 or section:
                child(options, f"section_{index}", 32, y)
                text(f"section_{index}", width=268, height=24, controller=1, text=heading if index==0 else section).set("color", "#FF80A2C0")
                child(options, f"rule_{index}", 28, y+22)
                widget(f"rule_{index}", width=272, height=1, bitmap="modern/rule", controller=1)
                y += 28
            child(options, "row_"+key, 28, y)
            row = widget("row_"+key, width=272, height=26, bitmap="modern/row", flags="nifty_fx")
            child(row, "label_"+key, 12, 3)
            text("label_"+key, width=248, height=24, controller=1, string_list=base+"/labels", string_index=index)
            ET.SubElement(labels, "string", text=caption)
            ET.SubElement(helps, "string", text=help_text)
            for event in ("a", "start"):
                for callback in callbacks[:-1]:
                    ET.SubElement(row, "on", event=event, run=callback)
                ET.SubElement(row, "on", event=event, open=target, **({"run": callbacks[-1], "branch": "true"} if callbacks else {}))
            ET.SubElement(row, "on", event="left_mouse", run="mouse emit accept event")
            y += 30
        child(options, "button_bar", 26, 414)
        bar = widget("button_bar", type="column_list", width=588, height=26, flags="pass_unhandled_to_focused_child left_right_tabs_items")
        child(bar, "button_back", 462)
        button = text("button_back", text="BACK", width=124, height=26, text_y=3, align="center", bitmap="modern/button", flags="nifty_fx")
        for event in ("a", "start"):
            ET.SubElement(button,"on",event=event,back="true")
        ET.SubElement(button,"on",event="left_mouse",run="mouse emit accept event")

