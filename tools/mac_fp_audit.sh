#!/bin/sh
# Floating-point determinism audit of a Mac session build (see
# MATHAUDIT-PROGRESS.md and port/linux/NETCODE.md). Run it in the source
# tree after tools/mac_rendered_session_checks.sh:
#   sh tools/mac_fp_audit.sh <out>/render-graph/engine_objects_<arch>.txt <arch> [binary]
# 1. no object of the simulation references a C library maths function whose
#    results differ between libraries (sin, cos, tan, asin, acos, atan, atan2,
#    exp, log, pow and their float forms; the game calls the vendored halo_
#    versions instead). The few presentation objects that do (audio, voice
#    chat, field of view, Vorbis and Opus) are listed in MAC_FP_ALLOWED.
#    floor, ceil, fmod, sqrt, rint and lrint are exact in IEEE arithmetic.
# 2. with a binary: no fused multiply-add on ppc (-mno-fused-madd), and no
#    x87 arithmetic on i386 (SSE, like the Linux and Windows builds).
set -eu
list=$1
arch=$2
binary=${3:-}
allowed=${MAC_FP_ALLOWED:-"mac_dsound_native mac_voice_audio view_fov opus_ stb_vorbis"}
status=0
inexact='^_(sin|cos|tan|asin|acos|atan|atan2|sinh|cosh|tanh|exp|exp2|log|log2|log10|pow|cbrt|hypot|sinf|cosf|tanf|asinf|acosf|atanf|atan2f|expf|exp2f|logf|log2f|log10f|powf|ldexp|ldexpf)$'
for object in $(cat "$list"); do
    [ -f "$object" ] || continue
    found=$(nm -u -arch "$arch" "$object" 2>/dev/null | awk '{print $NF}' | grep -E "$inexact" | tr '\n' ' ' || true)
    [ -n "$found" ] || continue
    skip=0
    for name in $allowed; do
        case "$(basename "$object")" in *"$name"*) skip=1 ;; esac
    done
    if [ "$skip" = 1 ]; then continue; fi
    echo "FAIL: $object references $found"
    status=1
done
if [ -n "$binary" ]; then
    otool -arch "$arch" -tv "$binary" > "${TMPDIR:-/tmp}/fp_audit_$$.s"
    case "$arch" in
    ppc)
        # (a jump table inside the code reads as instructions: ignore a hit
        # next to a .long)
        hits=$(awk '{ line[NR]=$0 } END { for (i=1;i<=NR;i++) if (line[i] ~ /\t(fmadds?|fmsubs?|fnmadds?|fnmsubs?|fres|frsqrte|fsqrts?)\t/ && line[i-1] !~ /\.long/ && line[i+1] !~ /\.long/) print line[i] }' "${TMPDIR:-/tmp}/fp_audit_$$.s")
        ;;
    *)
        # (data after an indirect jump in the same function, a jump table, reads as
        # instructions: ignore a hit within 40 lines after a jmp *reg, or between
        # the zero bytes of a table)
        hits=$(awk '{ line[NR]=$0 } END { for (i=1;i<=NR;i++) { if (line[i] ~ /:$/) last=-1000; if (line[i] ~ /\tjmp\t\*/) last=i; if (line[i] ~ /\t(f(mul|add|sub|subr|div|divr)[pls]?|fsqrt|fsin|fcos|fptan|fpatan|fyl2x|f2xm1|fprem|rsqrtss|rcpss|rsqrtps|rcpps)\t/ && i-last > 40 && line[i-1] !~ /addb\t%al,\(%eax\)/ && line[i+1] !~ /addb\t%al,\(%eax\)/) print line[i] } }' "${TMPDIR:-/tmp}/fp_audit_$$.s")
        ;;
    esac
    rm -f "${TMPDIR:-/tmp}/fp_audit_$$.s"
    if [ -n "$hits" ]; then
        echo "FAIL: $binary has instructions that change results:"
        echo "$hits" | head -20
        status=1
    fi
fi
if [ "$status" = 0 ]; then echo "$arch: floating-point audit passed"; fi
exit $status
