#!/usr/bin/env python
"""Create the checked native errors.c view with visible fatal assertions."""
import os
import sys

if len(sys.argv) != 3:
    raise SystemExit("usage: mac_errors_view.py source/cseries/errors.c output.c")
source_path, output_path = sys.argv[1:]
source = open(source_path, "r").read()
needle = '\t\t\twrite_to_error_file(string, TRUE);\n'
if source.count(needle) != 1:
    raise SystemExit("errors.c logging call changed; review native assertion view")
replacement = needle + '''\t\t\t/* port: Keep fatal native assertions visible when the Xbox debug.txt
\t\t\tpath is unavailable; ordinary engine chatter remains in its log. */
\t\t\tif (!strncmp(string, "EXCEPTION halt", 14) ||
\t\t\t\t!strncmp(string, "EXCEPTION assert", 16))
\t\t\t{
\t\t\t\tfputs(string, stderr);
\t\t\t\tfflush(stderr);
\t\t\t}
\t\t\t/* port: HALO_MAC_ENGINE_LOG=1 puts the rest of it on stderr too. The
\t\t\tengine says why it refused something through error(), and
\t\t\tnetwork_event's lines are all of that kind, so without this the
\t\t\tnetcode cannot say what went wrong: the Xbox's debug.txt that
\t\t\twrite_to_error_file wants is not written here. (getenv is declared
\t\t\there because errors.c does not include stdlib.h and an include at
\t\t\tthe top of this view would move every __LINE__ in the file.) */
\t\t\telse
\t\t\t{
\t\t\t\tstatic int engine_log = -1;

\t\t\t\tif (engine_log < 0)
\t\t\t\t{
\t\t\t\t\tchar *getenv(const char *name);
\t\t\t\t\tconst char *setting = getenv("HALO_MAC_ENGINE_LOG");

\t\t\t\t\tengine_log = setting && setting[0] == '1' && !setting[1];
\t\t\t\t}
\t\t\t\tif (engine_log)
\t\t\t\t{
\t\t\t\t\tfputs(string, stderr);
\t\t\t\t\tfflush(stderr);
\t\t\t\t}
\t\t\t}
'''
directory = os.path.dirname(output_path)
if directory and not os.path.isdir(directory):
    os.makedirs(directory)
open(output_path, "w").write(source.replace(needle, replacement))
