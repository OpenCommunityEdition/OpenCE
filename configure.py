#!/usr/bin/env python3

# root build script

import argparse
import json
from pathlib import Path
from typing import (
    IO,
    Any,
    Callable,
    Dict,
    Iterable,
    List,
    Optional,
    Set,
    Tuple,
    TypedDict,
    Union,
    cast,
)

from tools.project_x86 import (
    Object,
    ObjectStatus,
    ProjectConfig,
    SolutionConfig,
    calculate_progress,
    generate_build,
    is_windows,
)
import os
import subprocess
import sys
# arguments
parser = argparse.ArgumentParser()
parser.add_argument(
    "mode",
    choices=["configure", "progress"],
    default="configure",
    help="script mode (default: configure)",
    nargs="?",
)
parser.add_argument(
    "--objdiff",
    metavar="BINARY",
    type=Path,
    help="path to objdiff-cli binary (optional)",
)
parser.add_argument(
    "--csplit",
    metavar="BINARY",
    type=Path,
    help="path to csplit binary (optional)",
)
parser.add_argument(
    "--ninja",
    metavar="BINARY",
    type=Path,
    help="path to ninja binary (optional)",
)
parser.add_argument(
    "--ml",
    metavar="BINARY",
    type=Path,
    help="path to Microsoft Macro Assembler for authentic CRT .asm units",
)
parser.add_argument(
    "--linux-cc",
    metavar="BINARY",
    help="compiler for the native Linux build, `ninja linux` (default: clang)",
)
parser.add_argument(
    "--compiler-launcher",
    metavar="BINARY",
    help="native ports: a program that runs each compile, such as ccache "
    "(links and the Android host's few units run the compiler directly)",
)
parser.add_argument(
    "--release",
    action="store_true",
    help="release builds of the native ports (Linux, Windows, Android): assertions are not checked",
)
parser.add_argument(
    "--lto",
    choices=["full", "thin", "off"],
    default="full",
    help="link-time optimisation of the native ports (Linux, Windows): full (the default: the whole game "
    "optimised as one module, the fastest code and the slowest links), thin (parallel and incremental) or off",
)
parser.add_argument(
    "--portable",
    action="store_true",
    help="native x86 ports (Linux, Windows): code for any x86-64 processor (SSE2) rather than for this "
    "machine's (-march=native, the default); use it for builds that run on other computers",
)
parser.add_argument(
    "--pgo",
    nargs="?",
    const="train",
    default="use",
    choices=["use", "train", "off"],
    help="profile-guided optimisation of the native ports: use (the default) optimises with the profiles in pgo/, "
    "train (or plain --pgo) first records this platform's profile if it is missing, by letting an instrumented "
    "build play every campaign level (needs the game data in assets/ and a display; Linux and Windows), off does "
    "without",
)
parser.add_argument(
    "--pgo-profile",
    metavar="PROFDATA",
    type=Path,
    help="native ports: profile-guided optimisation from this profile instead",
)
parser.add_argument(
    "--android-ndk",
    type=str,
    help="Android NDK for `ninja android` (default: ANDROID_NDK_HOME, or the newest under the Android SDK)",
)
parser.add_argument(
    "--android-guest-cc",
    type=str,
    help="clang with the arm64_32 target for the Android guest (default: clang)",
)
parser.add_argument(
    "--web-emcc",
    type=str,
    help="Emscripten's emcc for `ninja web` (default: emcc on the PATH, or ~/emsdk)",
)
if not is_windows():
    parser.add_argument(
        "--wrapper",
        metavar="BINARY",
        type=Path,
        help="path to wibo or wine (optional)",
    )

args = parser.parse_args()

# path setup

build_dir = Path("build")
config_dir = Path("config")
config_json_path = config_dir / "config.json"
project_dir = Path("projects")
tools_dir = Path("tools")

# build config json

class BuildConfigObject(TypedDict):
    name: str
    index: int
    status: str
    options: Dict[str, Any]

class BuildConfigProject(TypedDict):
    name: str
    guid: str
    objects: List[BuildConfigObject]
    options: Dict[str, Any]

class BuildConfig(TypedDict):
    name: str
    guid: str
    baserom: str
    projects: List[BuildConfigProject]

build_config: BuildConfig = json.load(open(config_json_path, "r", encoding="utf-8"))

# solution configuration

sln = SolutionConfig()
sln.build_dir = build_dir
sln.config_dir = config_dir
sln.project_dir = project_dir
sln.tools_dir = tools_dir

sln.name = build_config["name"]
sln.guid = build_config["guid"]
sln.baserom = build_config["baserom"]

# Apply arguments
sln.objdiff_path = args.objdiff
sln.csplit_path = args.csplit
sln.ninja_path = args.ninja
sln.ml_path = args.ml
sln.linux_cc = args.linux_cc
sln.compiler_launcher = args.compiler_launcher
sln.port_release = args.release
sln.port_lto = args.lto
sln.port_portable = args.portable
sln.port_pgo = args.pgo
sln.port_pgo_profile = args.pgo_profile
sln.android_ndk = args.android_ndk
sln.android_guest_cc = args.android_guest_cc
sln.web_emcc = args.web_emcc
if not is_windows():
    sln.wrapper = args.wrapper

# Tool versions
sln.objdiff_tag = "v3.6.0"
sln.csplit_tag = "v0.0.2"
# v0.0.2 plus upstream's "Fix uninit phony section"; built from source on Linux
sln.csplit_source_commit = "db510e7ef9f60bb09b13a3df88c7419e9b215cde"
sln.wibo_tag = "1.0.0"
sln.uasm_tag = "v2.57r"

sln.projects = []
for build_project in build_config["projects"]:
    project = ProjectConfig(**build_project.get("options", {}))
    project.name = build_project["name"]
    project.guid = build_project["guid"]
    project.objects = []
    for build_object in build_project["objects"]:
        def get_object_completed(status: str) -> ObjectStatus:
            if status == "MISSING":
                return ObjectStatus.Missing
            elif status == "Matching":
                return ObjectStatus.Matching
            elif status == "NonMatching":
                return ObjectStatus.NonMatching
            assert False, f"Invalid object status {status}"
        project.objects.append(Object(get_object_completed(build_object["status"]), Path(build_object["name"]), **build_object.get("options", {})))
    sln.projects.append(project)

# build file generation

if args.mode == "configure":
    if sln.matching and any(
        obj["status"] != "MISSING" and obj["name"].startswith("libs/d3d8/")
        for project in build_config["projects"]
        for obj in project["objects"]
    ):
        subprocess.run([sys.executable, "libs/d3d8/generate_sdk_overlay.py"], check=True)
    # Write build.ninja and objdiff.json
    generate_build(sln)
elif args.mode == "progress":
    # Print progress information
    calculate_progress(sln)
else:
    sys.exit("Unknown mode: " + args.mode)
