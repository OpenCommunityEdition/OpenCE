#!/usr/bin/env python3
"""Keep the retail cache helpers while letting the native cache own map paths."""

import sys
import re


SHADOWED_RETAIL_PROVIDERS = (
    "cache_files_map_directory", "cache_file_tag_cache_contains",
    "tag_groups_checksum", "tag_get", "tag_get_group_tag", "tag_get_name",
    "tag_index_is_group", "tag_iterator_new", "tag_iterator_next",
    "tag_loaded", "tag_reference_set",
    # (the menus' tags join the native registry: port/mac/engine_menu.c)
    "cache_files_tag_instances", "cache_files_set_tag_instances",
    # (the native world holds each BSP: port/mac/engine_world.c; a switch
    # of structure bsp, scenario.c, loads and unloads them through these)
    "scenario_structure_bsp_load", "scenario_structure_bsp_unload",
)


def rename_definition(text, name):
    pattern = re.compile(r"\b" + re.escape(name) + r"\s*\(")
    definitions = []
    for match in pattern.finditer(text):
        opening = text.find("(", match.start(), match.end())
        depth = 1
        pos = opening + 1
        while pos < len(text) and depth:
            if text[pos] == "(":
                depth += 1
            elif text[pos] == ")":
                depth -= 1
            pos += 1
        if depth:
            continue
        while pos < len(text) and text[pos].isspace():
            pos += 1
        if pos < len(text) and text[pos] == "{":
            definitions.append(match.start())
    if len(definitions) != 1:
        raise SystemExit("expected one definition for provider " + name)
    pos = definitions[0]
    return text[:pos] + name + "_retail_legacy" + text[pos + len(name):]


def main():
    if len(sys.argv) != 3:
        raise SystemExit("usage: mac_cache_files_map_directory_view.py source.c output.c")
    source_path, output_path = sys.argv[1:]
    with open(source_path, "r") as source_file:
        text = source_file.read()
    for name in SHADOWED_RETAIL_PROVIDERS:
        text = rename_definition(text, name)
    text += """
/* port: the native world's BSPs (port/mac/engine_world.c) */
extern boolean mac_engine_structure_bsp_load(const struct scenario_structure_bsp_reference *reference);
extern void mac_engine_structure_bsp_unload(const struct scenario_structure_bsp_reference *reference);
boolean scenario_structure_bsp_load(
	struct scenario_structure_bsp_reference *reference)
{
	return mac_engine_structure_bsp_load(reference);
}
void scenario_structure_bsp_unload(
	struct scenario_structure_bsp_reference *reference)
{
	mac_engine_structure_bsp_unload(reference);
}
"""
    with open(output_path, "w") as output_file:
        output_file.write(text)


if __name__ == "__main__":
    main()
