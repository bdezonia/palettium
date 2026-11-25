#!/bin/sh
# Build the Windows 3.1 staging display-driver module.
# PALSTAGE: ALWAYS rejects style=0. PALTRIAL: explicit Standard-mode boot
# experiment only; DO NOT deploy without a disposable VM and DOS recovery.
set -eu
cd "$(dirname "$0")/../.."
command -v wcc >/dev/null
command -v wlink >/dev/null
command -v wdump >/dev/null
mkdir -p tests/exes

SOURCES='common/rgfx.c
common/rg_hardware.c
common/rg_palette.c
common/rg_objects.c
common/rg_gdi_output.c
common/rg_gdi_text.c
common/rg_cursor.c
common/rg_gdi_rop.c
common/rg_gdi_blit.c
common/rg_win31_screen.c
hardware/tseng/et4000ax/et4k.c
windows/win31/pal_gdiinfo.c
windows/win31/pal_pdevice.c
windows/win31/pal_objects.c
windows/win31/pal_font16.c
windows/win31/pal_cursor16.c
windows/win31/pal_trial_gate.c
windows/win31/pal_bitmap.c
windows/win31/pal_bitmap_device.c
windows/win31/pal_enable.c
windows/win31/pal_et4k_session.c
windows/win31/pal_vdd.c
windows/win31/pal_et4k_win16.c
windows/win31/pal_screen_ddi.c
windows/win31/pal_screen_host.c
windows/win31/pal_ddl_blit.c
windows/win31/palddi.c
windows/win31/pal_driver_entry.c
windows/win31/pal_boottrace.c'

if [ "${PAL_TRIAL_BUILD:-0}" = 1 ]; then
    NAME=PALTRIAL
    TRIAL_OPTION=-dPAL_WIN31_TRIAL_MODE
    # Trial-only KERNEL file trace is enabled by default; explicitly
    # disable with PAL_TRIAL_TRACE=0 to compare behavior without file I/O.
    if [ "${PAL_TRIAL_TRACE:-1}" = 1 ]; then
        TRACE_OPTION=-dPAL_WIN31_BOOTTRACE
    else
        TRACE_OPTION=
    fi
else
    NAME=PALSTAGE
    TRACE_OPTION=
    TRIAL_OPTION=
fi
LINKFILE="tests/exes/$NAME.generated.lnk"
OUTPUT="tests/exes/$NAME.DRV"
rm -f "$LINKFILE" "$OUTPUT" "tests/exes/$NAME.exports.txt"

# Generate ONE complete linker recipe.  Do not feed wlink a file generated
# through wcl -fd: the previous script duplicated its SYSTEM block.
{
    printf '%s\n' 'option quiet' 'system windows_dll' \
        "name tests/exes/$NAME.DRV" 'option modname=DISPLAY' \
        "option map=tests/exes/$NAME.map"
} > "$LINKFILE"

for source in $SOURCES; do
    base=$(basename "$source" .c)
    obj="tests/exes/$base.obj"
    echo "Win16 C: $source"
    # Session and Win16 platform adapter both include et4k.h.  The
    # ET4000AX implementation also MUST be in SOURCES so it can link.
    # The default Win16 __STK checker links a fatal MessageBox handler
    # from USER, which cannot load before the display driver.  -s omits
    # this runtime check ONLY in this early-boot Win16 display module.
    # This does not make unchecked stack growth safe; keep the binary NE
    # import audit below and test only in a disposable 86Box VM.
    wcc -bt=windows -mc -bd -zu -zq -s $TRIAL_OPTION $TRACE_OPTION -i=include -i=windows/win31 \
        -i=hardware/tseng/et4000ax \
        -fo="$obj" "$source"
    test -s "$obj"
    printf 'file %s\n' "$obj" >> "$LINKFILE"
done

# These ordinals follow the documented Win16 display DDI.
cat >> "$LINKFILE" <<'EOF_EXPORTS'
export BitBlt.1
export ColorInfo.2
export Control.3
export Disable.4
export Enable.5
export EnumDFonts.6
export EnumObj.7
export Output.8
export Pixel.9
export RealizeObject.10
export StrBlt.11
export ScanLR.12
export ExtTextOut.14
export GetCharWidth.15
export DeviceBitmap.16
export FastBorder.17
export SetAttribute.18
export SetPalette.22
export GetPalette.23
export SetPalTrans.24
export GetPalTrans.25
export UpdateColors.26
export Inquire.101
export SetCursor.102
export MoveCursor.103
export CheckCursor.104
EOF_EXPORTS

# Fail early if an edit duplicates the executable-format selection.
[ "$(grep -ic '^system ' "$LINKFILE")" -eq 1 ] || {
    echo 'FAIL: linker file must contain exactly one SYSTEM directive' >&2
    exit 1
}

wlink @"$LINKFILE"
test -s "$OUTPUT"
wdump -x "$OUTPUT" > "tests/exes/$NAME.exports.txt"
# EARLY LOADER RULE: display drivers must not depend on USER, which has
# not finished loading when Windows is bringing up the display subsystem.
python3 tools/check_win31_modname.py "$OUTPUT"
python3 tools/check_win31_imports.py "$OUTPUT"
python3 tools/check_win31_exports.py "tests/exes/$NAME.exports.txt"
if [ "$NAME" = PALTRIAL ]; then
    printf '%s\n' 'PASS: linked PALTRIAL.DRV (experimental 256/512KiB Standard-mode GDI boot ONLY).' \
        'WARNING: unverified Windows runtime. Use snapshot and DOS recovery; no enhanced mode.'
else
    printf '%s\n' 'PASS: linked Win16 PALSTAGE.DRV with required staging exports.' \
        'NOT INSTALLABLE: Enable(style=0) ALWAYS fails in default build.'
fi
