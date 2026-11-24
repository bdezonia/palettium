#!/usr/bin/env python3
"""Regression guard for the Win16 link recipe and project-wide make clean."""
import pathlib
import re
import shutil
import subprocess
import tempfile

ROOT = pathlib.Path(__file__).resolve().parents[1]


def check_win16_recipe():
    script = (ROOT / "windows/win31/build-palddi.sh").read_text()
    source_match = re.search(r"SOURCES='([^']+)'", script, re.S)
    assert source_match, "missing Win16 SOURCES list"
    sources = source_match.group(1).split()
    assert "hardware/tseng/et4000ax/et4k.c" in sources, "ET4000 backend not linked"
    assert "windows/win31/pal_et4k_session.c" in sources, "session missing"
    assert "windows/win31/pal_et4k_win16.c" in sources, "Win16 adapter missing"
    assert "windows/win31/pal_vdd.c" in sources, "Win3.x VDD policy not linked"
    assert "windows/win31/pal_font16.c" in sources
    assert "common/rg_gdi_text.c" in sources
    assert "common/rg_cursor.c" in sources
    assert "windows/win31/pal_cursor16.c" in sources
    for export in ("Inquire.101", "SetCursor.102", "MoveCursor.103", "CheckCursor.104"):
        assert "export " + export in script, "missing cursor DDI " + export
    win16_entry = (ROOT / "windows/win31/palddi.c").read_text()
    assert "pal_win16_et4k_profile_plan(&plan)" in win16_entry, (
        "GDI style-1 must use configured VRAM/VDD plan")
    assert "style != 1u" in win16_entry, "stage-only Enable(style=0) gate lost"
    assert "DWORD FAR PASCAL ExtTextOut" in win16_entry
    assert "DWORD FAR PASCAL StrBlt" in win16_entry
    assert "void FAR PASCAL MoveCursor" in win16_entry
    assert "void FAR PASCAL CheckCursor" in win16_entry
    assert "pal_win16_install_ready()) return 0" in win16_entry, (
        "mandatory DDI installation gate removed")
    assert "pal_et4k_session_start_published(&pal_native_session" in win16_entry, (
        "native transaction no longer integrated into Win16 Enable path")
    assert "pal_et4k_session_stop(&pal_native_session)" in win16_entry, (
        "Win16 Disable no longer releases the native session")
    assert "pal_win16_publish_host" in win16_entry and \
           "pal_screen_host_gdi_snapshot(host,token,&image)" in win16_entry, (           "Win16 GDI-owned PDEVICE must publish through checked snapshot")
    assert "pal_win16_bind_host" not in win16_entry, (           "unchecked device host binding must not be restored")
    # Cursor DDI must protect BOTH the publisher and the consumer of the
    # two-word coordinate pair using the Win3.x DDK critical convention.
    cursor = (ROOT / "windows/win31/pal_cursor16.c").read_text()
    assert "#pragma aux pal_irq_publish" in cursor
    assert "#pragma aux pal_irq_consume" in cursor
    assert cursor.count('"pushf"') == 2 and cursor.count('"cli"') == 2
    assert cursor.count("0xeb 0x01 0xcf 0x0e 0xe8 0xfb 0xff") == 2
    assert '"sti"' not in cursor and "s->writing" not in cursor
    assert "s->composing=1" in cursor and "s->composing=0" in cursor
    loader = (ROOT / "windows/win31/palload.c").read_text()
    assert 'LoadLibrary("C:\\\\PALTEST\\\\PALSTAGE.DRV")' in loader
    assert 'GetProcAddress(module,"ENABLE")' in loader
    assert '(*enable)((LPVOID)&pal_gdi_guard[8],1u,' in loader
    assert '(*enable)((LPVOID)&pal_gdi_guard[8],0u,' not in loader
    assert 'Enable(style=0)' in loader  # explicit refusal in instructions
    assert 'SetCursor(' not in loader and 'int86(' not in loader
    assert (ROOT / "windows/win31/build-palload.sh").is_file()
    adapter = (ROOT / "windows/win31/pal_et4k_win16.c").read_text()
    assert 'GetPrivateProfileInt("Palettium","VRAMKiB"' in adapter
    assert 'GetWinFlags() & 0x0020u' in adapter, (
        "raw CRTC read missing enhanced-mode safety gate")
    assert len(sources) == len(set(sources)), "duplicate linked source"
    stems = [pathlib.Path(x).stem for x in sources]
    assert len(stems) == len(set(stems)), "Win16 object filename collision"

    # Exactly the include switches that the actual wcc invocation receives.
    command = re.search(r"\bwcc -bt=windows .*?\n\s*test -s", script, re.S)
    assert command, "Win16 compiler invocation not found"
    include_dirs = [ROOT / p for p in re.findall(r"-i=([\w./-]+)", command.group())]
    assert ROOT / "hardware/tseng/et4000ax" in include_dirs
    assert ROOT / "windows/win31" in include_dirs
    assert ROOT / "include" in include_dirs

    examined = set()

    def examine(file):
        if file in examined:
            return
        examined.add(file)
        assert file.is_file(), "missing Win16 source/header: " + str(file)
        for header in re.findall(r'^\s*#\s*include\s*"([^"]+)"', file.read_text(), re.M):
            candidates = [file.parent / header] + [d / header for d in include_dirs]
            found = next((x for x in candidates if x.is_file()), None)
            assert found is not None, "unresolved include {} from {}".format(header, file)
            examine(found.resolve())

    for source in sources:
        examine((ROOT / source).resolve())


def check_full_clean():
    with tempfile.TemporaryDirectory(prefix="pal-clean-") as temp:
        base = pathlib.Path(temp)
        shutil.copyfile(ROOT / "Makefile", base / "Makefile")
        generated = [
            "tests/exes/raster-test", "tests/exes/win31-native-test",
            "tests/exes/PALSTAGE.DRV", "tests/exes/PALSTAGE.generated.lnk",
            "tests/exes/PALDIAG.EXE", "tests/exes/paldiag-host-test",
            "tests/exes/PALLOAD.EXE", "tests/exes/palload-host-test",
            "tests/exes/et4k-test", "tests/exes/raster-demo.ppm",
            "build/dos/ET4KTEST.EXE", "build/other/temporary.obj",
            "windows/win31/PALCORE.DLL", "windows/win31/PALDEV.EXPORTS.txt",
            "windows/win31/PALPROBE.OBJ", "windows/win31/pal_bitmap.obj",
            "windows/win31/PALTEST.EXE", "rgfx.obj", "et4k_demo.OBJ",
        ]
        retained = [
            "windows/win31/PALPROBE.LNK", "windows/win31/palddi.c",
            "windows/win31/README.md", "README.md", "tests/test_kept.c",
        ]
        for rel in generated + retained:
            path = base / rel
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_text("test fixture\n")
        for _ in range(2):  # clean must be idempotent
            subprocess.run(["make", "clean"], cwd=base, check=True,
                           stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        for rel in generated:
            assert not (base / rel).exists(), "not cleaned: " + rel
        assert not (base / "tests/exes").exists()
        assert not (base / "build").exists()
        for rel in retained:
            assert (base / rel).is_file(), "source deleted by clean: " + rel


if __name__ == "__main__":
    check_win16_recipe()
    check_full_clean()
    print("PASS: Win16 ET4000 includes/link list; global clean clears all outputs and preserves sources")
