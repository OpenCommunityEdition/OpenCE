#!/usr/bin/env python3
"""Writes the Pterodactyl egg of the dedicated server again, with its
installation script, and checks it:

    python tools/egg_build.py            # deploy/pterodactyl/install.sh into the egg
    python tools/egg_build.py --check    # exit 1 if the egg is not what this would write

The egg (deploy/pterodactyl/egg-opence-dedicated-server.json) is edited by
hand except for scripts.installation.script, which is install.sh's text.
The checks follow what the panel's importer and Wings take (PTDL_v2):
the fields and their types, the variables' environment names, their rules
in Laravel's syntax (the rules eggs use), each default against its rules,
and the startup command's {{...}} placeholders. tools/test_egg.py runs them.
"""

import argparse
import json
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
EGG = ROOT / "deploy/pterodactyl/egg-opence-dedicated-server.json"
INSTALL = ROOT / "deploy/pterodactyl/install.sh"

# what Wings sets itself, which a variable may not be (the panel's
# EggVariable::RESERVED_ENV_NAMES)
RESERVED = {"SERVER_MEMORY", "SERVER_IP", "SERVER_PORT", "ENV", "HOME", "USER", "STARTUP", "SERVER_UUID", "UUID"}
# placeholders the startup command may hold besides the variables'
STARTUP_BUILTINS = {"SERVER_MEMORY", "SERVER_IP", "SERVER_PORT"}
# Laravel rules with no argument, and with one (the ones eggs use)
RULES_PLAIN = {"required", "nullable", "sometimes", "string", "integer", "numeric", "boolean", "alpha_dash",
               "alpha_num", "url", "present"}
RULES_WITH_ARGUMENT = {"max", "min", "between", "in", "not_in", "regex", "not_regex", "digits_between", "size"}


def egg_text(egg: dict) -> str:
    """The file's text for egg: four-space indents, as the panel exports."""
    return json.dumps(egg, indent=4, ensure_ascii=False) + "\n"


def build(egg: dict, script: str) -> dict:
    """egg with script as its installation script."""
    egg = json.loads(json.dumps(egg))
    egg["scripts"]["installation"]["script"] = script.replace("\r\n", "\n")
    return egg


def parse_rules(rules: str) -> list:
    """[(name, argument or None)] of a Laravel rules string; ValueError if malformed."""
    if not isinstance(rules, str) or not rules:
        raise ValueError("rules must be a non-empty string")
    parsed = []
    for part in rules.split("|"):
        name, colon, argument = part.partition(":")
        if name in RULES_PLAIN:
            if colon:
                raise ValueError(f"{name} takes no argument")
            parsed.append((name, None))
        elif name in RULES_WITH_ARGUMENT:
            if not argument:
                raise ValueError(f"{name} needs an argument")
            if name in ("max", "min", "size") and not re.fullmatch(r"\d+", argument):
                raise ValueError(f"{name}:{argument} is not a number")
            if name in ("between", "digits_between") and not re.fullmatch(r"\d+,\d+", argument):
                raise ValueError(f"{name}:{argument} is not two numbers")
            if name in ("between", "digits_between"):
                low, high = (int(value) for value in argument.split(","))
                if low > high:
                    raise ValueError(f"{name}:{argument} is empty")
            if name in ("regex", "not_regex"):
                match = re.fullmatch(r"/(.*)/([a-z]*)", argument, re.S)
                if not match:
                    raise ValueError(f"{name}:{argument} is not /pattern/")
                re.compile(match.group(1))
            parsed.append((name, argument))
        else:
            raise ValueError(f"unknown rule {name!r}")
    names = [name for name, _ in parsed]
    if "required" in names and "nullable" in names:
        raise ValueError("both required and nullable")
    if len(set(names)) != len(names):
        raise ValueError("a rule given twice")
    return parsed


def value_passes(value: str, rules: list) -> str:
    """Why value fails the parsed rules, as Laravel would judge an
    environment value (a string), or "" if it passes."""
    names = {name for name, _ in rules}
    if value == "":
        return "" if "nullable" in names or "required" not in names else "empty, but required"
    numeric = "integer" in names or "numeric" in names
    if "integer" in names and not re.fullmatch(r"-?\d+", value):
        return "not an integer"
    if "numeric" in names and not re.fullmatch(r"-?\d+(\.\d+)?", value):
        return "not a number"
    if "boolean" in names and value not in ("0", "1", "true", "false"):
        return "not a boolean"
    if "alpha_dash" in names and not re.fullmatch(r"[\w-]+", value):
        return "not letters, digits, dashes and underscores"
    if "alpha_num" in names and not re.fullmatch(r"[^\W_]+", value):
        return "not letters and digits"
    size = float(value) if numeric else len(value)
    for name, argument in rules:
        if name == "max" and size > int(argument):
            return f"over max:{argument}"
        if name == "min" and size < int(argument):
            return f"under min:{argument}"
        if name == "between":
            low, high = (int(part) for part in argument.split(","))
            if not low <= size <= high:
                return f"outside between:{argument}"
        if name == "in" and value not in argument.split(","):
            return f"not in {argument}"
        if name == "regex" and not re.search(re.fullmatch(r"/(.*)/[a-z]*", argument, re.S).group(1), value):
            return f"does not match {argument}"
    return ""


def check(egg: dict) -> list:
    """What is wrong with egg, as lines (none: it is fine)."""
    problems = []

    def need(container, key, kind, where):
        if key not in container:
            problems.append(f"{where}{key}: missing")
            return None
        if not isinstance(container[key], kind):
            problems.append(f"{where}{key}: not {kind}")
            return None
        return container[key]

    if not isinstance(egg, dict):
        return ["the egg is not an object"]
    meta = need(egg, "meta", dict, "")
    if meta is not None:
        if meta.get("version") != "PTDL_v2":
            problems.append("meta.version: not PTDL_v2")
        if "update_url" not in meta:
            problems.append("meta.update_url: missing")
    for key in ("_comment", "exported_at", "name", "author", "description", "startup"):
        need(egg, key, str, "")
    author = egg.get("author", "")
    if isinstance(author, str) and not re.fullmatch(r"[^@\s]+@[^@\s]+\.[^@\s]+", author):
        problems.append("author: not an e-mail address")
    if isinstance(egg.get("exported_at"), str) and not re.fullmatch(
            r"\d{4}-\d\d-\d\dT\d\d:\d\d:\d\d\+00:00", egg["exported_at"]):
        problems.append("exported_at: not an ISO 8601 time")
    if "features" not in egg or not isinstance(egg["features"], (list, type(None))):
        problems.append("features: not a list or null")
    images = need(egg, "docker_images", dict, "")
    if images is not None:
        if not images:
            problems.append("docker_images: empty")
        for label, image in images.items():
            if not isinstance(image, str) or not re.fullmatch(r"[a-z0-9./_-]+(:[A-Za-z0-9._-]+)?(@sha256:[0-9a-f]{64})?", image):
                problems.append(f"docker_images[{label!r}]: not an image reference")
    need(egg, "file_denylist", list, "")

    config = need(egg, "config", dict, "")
    if config is not None:
        for key in ("files", "startup", "logs", "stop"):
            if need(config, key, str, "config.") is None:
                continue
            if key == "stop":
                continue
            try:
                parsed = json.loads(config[key])
            except ValueError:
                problems.append(f"config.{key}: not JSON in a string")
                continue
            if not isinstance(parsed, dict):
                problems.append(f"config.{key}: not an object")
            elif key == "startup":
                # (Wings takes a string or a list of them, any of which
                # found in a line of the console marks the server started)
                done = parsed.get("done")
                lines = [done] if isinstance(done, str) else done
                if not isinstance(lines, list) or not lines or not all(
                        isinstance(line, str) and line for line in lines):
                    problems.append("config.startup: no done line")
        if isinstance(config.get("stop"), str) and not config["stop"]:
            problems.append("config.stop: empty")

    scripts = need(egg, "scripts", dict, "")
    installation = need(scripts, "installation", dict, "scripts.") if scripts is not None else None
    if installation is not None:
        for key in ("script", "container", "entrypoint"):
            need(installation, key, str, "scripts.installation.")
        if not str(installation.get("script", "")).startswith("#!/bin/bash"):
            problems.append("scripts.installation.script: does not start with #!/bin/bash")
        if "\r" in str(installation.get("script", "")):
            problems.append("scripts.installation.script: has carriage returns")

    variables = need(egg, "variables", list, "")
    names = set()
    for index, variable in enumerate(variables or []):
        where = f"variables[{index}]."
        if not isinstance(variable, dict):
            problems.append(f"variables[{index}]: not an object")
            continue
        for key in ("name", "description", "env_variable", "default_value", "rules", "field_type"):
            need(variable, key, str, where)
        for key in ("user_viewable", "user_editable"):
            need(variable, key, bool, where)
        env = variable.get("env_variable", "")
        if not isinstance(env, str):
            continue
        where = f"variables[{env}]."
        if not re.fullmatch(r"[A-Z][A-Z0-9_]{0,190}", env):
            problems.append(f"{where}env_variable: not upper-case letters, digits and underscores")
        if env in RESERVED:
            problems.append(f"{where}env_variable: reserved by Wings")
        if env in names:
            problems.append(f"{where}env_variable: given twice")
        names.add(env)
        if variable.get("field_type") not in ("text", None):
            problems.append(f"{where}field_type: not text")
        if variable.get("user_editable") and not variable.get("user_viewable"):
            problems.append(f"{where}user_editable without user_viewable: the owner cannot see it to edit it")
        try:
            rules = parse_rules(variable.get("rules"))
        except (ValueError, re.error) as error:
            problems.append(f"{where}rules: {error}")
            continue
        default = variable.get("default_value")
        if isinstance(default, str):
            failure = value_passes(default, rules)
            if failure:
                problems.append(f"{where}default_value {default!r}: {failure}")

    startup = egg.get("startup", "")
    if isinstance(startup, str):
        placeholders = re.findall(r"{{\s*([^{}]*?)\s*}}", startup)
        if "SERVER_PORT" not in placeholders:
            problems.append("startup: no {{SERVER_PORT}}")
        for placeholder in placeholders:
            if placeholder.startswith(("server.build.", "env.")):
                continue
            if placeholder not in STARTUP_BUILTINS and placeholder not in names:
                problems.append(f"startup: {{{{{placeholder}}}}} is neither Wings' nor a variable's")
            if placeholder != placeholder.upper():
                problems.append(f"startup: {{{{{placeholder}}}}} is not upper case (Wings upper-cases names)")
        if re.search(r"{{|}}", re.sub(r"{{\s*[^{}]*?\s*}}", "", startup)):
            problems.append("startup: unbalanced braces")
    return problems


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--check", action="store_true", help="only check that the egg is up to date and valid")
    args = parser.parse_args()

    egg = json.loads(EGG.read_text(encoding="utf-8"))
    built = build(egg, INSTALL.read_text(encoding="utf-8"))
    problems = check(built)
    for problem in problems:
        print(f"{EGG.relative_to(ROOT)}: {problem}", file=sys.stderr)
    if problems:
        return 1
    text = egg_text(built)
    if args.check:
        if EGG.read_text(encoding="utf-8") != text:
            print(f"{EGG.relative_to(ROOT)} is out of date: run python tools/egg_build.py", file=sys.stderr)
            return 1
        return 0
    if EGG.read_text(encoding="utf-8") != text:
        EGG.write_text(text, encoding="utf-8")
        print(f"wrote {EGG.relative_to(ROOT)}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
