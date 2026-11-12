"""Regenerate MCC navigation hooks in existing PC widgets without retail tags.

Full generation reads these same WIDGET_PATCHES. This limited pass updates only
the named map-navigation widgets' handlers and attributes, preserving artwork,
children and all other screens.
"""
from pathlib import Path
import re
from xml.sax.saxutils import quoteattr

import port_settings

MAP_WIDGETS = {
    "main_menu.solo_level_select.xml": (
        "main_menu/solo_level_select/solo_level_select_list",
        "main_menu/solo_level_select/solo_level_select_screen"),
    "main_menu.multiplayer_type_select.mp_map_select.xml": (
        port_settings.MT + "/mp_map_select/mp_map_select_list_2",
        port_settings.MT + "/mp_map_select/mp_map_select_screen"),
    "main_menu.multiplayer_type_select.connected.xml": (
        port_settings.MT + "/connected/connected_map_select_wrapper",),
}


def regenerate(folder: Path) -> None:
    for filename, names in MAP_WIDGETS.items():
        path = folder / filename
        text = path.read_text(encoding="utf-8")
        for name in names:
            patch = port_settings.WIDGET_PATCHES[name]
            pattern = r'(\t<widget name="' + re.escape(name) + r'"[^>]*>\n)(.*?)(\t</widget>)'
            match = re.search(pattern, text, re.S)
            if not match:
                raise ValueError(f"Missing map-navigation widget {name} in {path}")
            opening, body, closing = match.groups()
            for key, value in patch.get("set", {}).items():
                opening = re.sub(r' ' + re.escape(key) + r'="[^"]*"', '', opening)
                opening = opening[:-2] + ' ' + key + '=' + quoteattr(str(value)) + '>\n'
            body = re.sub(r'\t\t<on\b[^\n]*\n', '', body)
            handlers = ''.join('\t\t' + line + '\n' for line in patch["handlers"])
            text = text[:match.start()] + opening + handlers + body + closing + text[match.end():]
        path.write_text(text, encoding="utf-8", newline="\n")
