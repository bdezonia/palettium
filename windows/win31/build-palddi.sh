#!/bin/sh
# Build the Windows 3.1 staging display-driver module.
# This is NOT an installable display driver: Enable(style=0) fails.
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
common/rg_gdi_rop.c
common/rg_gdi_blit.c
common/rg_win31_screen.c
windows/win31/pal_gdiinfo.c
windows/win31/pal_pdevice.c
windows/win31/pal_objects.c
windows/win31/pal_enable.c
windows/win31/pal_screen_ddi.c
windows/win31/pal_screen_host.c
windows/win31/pal_ddl_blit.c
windows/win31/palddi.c
windows/win31/pal_driver_entry.c'

LINKFILE=tests/exes/PALSTAGE.generated.lnk
OUTPUT=tests/exes/PALSTAGE.DRV
rm -f "$LINKFILE" "$OUTPUT" tests/exes/PALSTAGE.exports.txt

# Generate ONE complete linker recipe.  Do not feed wlink a file generated
# through wcl -fd: the previous script duplicated its SYSTEM block.
{
    printf '%s\n' 'option quiet' 'system windows_dll' \
        'name tests/exes/PALSTAGE.DRV' 'option modname=PALSTAGE'
} > "$LINKFILE"

for source in $SOURCES; do
    base=$(basename "$source" .c)
    obj="tests/exes/$base.obj"
    echo "Win16 C: $source"
    wcc -bt=windows -mc -bd -zu -zq -i=include -i=windows/win31 \
        -fo="$obj" "$source"
    test -s "$obj"
    printf 'file %s\n' "$obj" >> "$LINKFILE"
done

# These ordinals follow the documented Win16 display DDI.
cat >> "$LINKFILE" <<'EOF_EXPORTS'
export BitBlt.1
export ColorInfo.2
export Disable.4
export Enable.5
export Output.8
export Pixel.9
export RealizeObject.10
export ScanLR.12
export SetPalette.22
export GetPalette.23
export SetPalTrans.24
export GetPalTrans.25
export UpdateColors.26
EOF_EXPORTS

# Fail early if an edit duplicates the executable-format selection.
[ "$(grep -ic '^system ' "$LINKFILE")" -eq 1 ] || {
    echo 'FAIL: linker file must contain exactly one SYSTEM directive' >&2
    exit 1
}

wlink @"$LINKFILE"
test -s "$OUTPUT"
wdump -x "$OUTPUT" > tests/exes/PALSTAGE.exports.txt
python3 tools/check_win31_exports.py tests/exes/PALSTAGE.exports.txt
printf '%s\n' 'PASS: linked Win16 PALSTAGE.DRV with required staging exports.' \
    'NOT INSTALLABLE: Enable(style=0) still fails; patterned brushes, text, bitmap, cursor and mode initialization remain missing.'
