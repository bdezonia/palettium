/* Palettium v0.7: Win16 CORE INTEGRATION TEST, not a GDI driver.
 * Link the real cross-platform raster and hardware sources into this DLL.
 * Keep test memory in one small segment: never use a linear 307200-byte array.
 */
#include <windows.h>
#include "rgfx.h"

#define TEST_WIDTH 64
#define TEST_HEIGHT 48
#define TEST_COUNT (TEST_WIDTH * TEST_HEIGHT)

static rg_u8 test_buffer[TEST_COUNT];

static rg_u8 test_read(void *cookie, rg_offset offset)
{
    rg_u8 *p = (rg_u8 *)cookie;
    if (offset >= (rg_offset)TEST_COUNT) return 0;
    return p[(unsigned)offset];
}

static void test_write(void *cookie, rg_offset offset, rg_u8 color)
{
    rg_u8 *p = (rg_u8 *)cookie;
    if (offset < (rg_offset)TEST_COUNT) p[(unsigned)offset] = color;
}

/* Bitmask of runtime test failures; 0 means every test passed.
 * The DLL export is useful to a future small Win16 probe application.
 */
WORD FAR PASCAL __export PalettiumCoreSelfTest(void)
{
    RG_Device dev;
    RG_VramOps ops;
    unsigned i;
    WORD failures = 0;

    if (sizeof(rg_u8) != 1 || sizeof(rg_offset) < 4 ||
        sizeof(int) != 2) failures |= 1;
    for (i = 0; i < TEST_COUNT; ++i) test_buffer[i] = 0;
    ops.read_byte = test_read;
    ops.write_byte = test_write;
    if (!rg_init_vram(&dev, TEST_WIDTH, TEST_HEIGHT,
                      TEST_WIDTH, TEST_COUNT, RG_FMT_INDEX8, ops, test_buffer)) return 0x8000;

    dev.ops.pixel(&dev, 1, 2, 17);
    if (test_buffer[2 * TEST_WIDTH + 1] != 17) failures |= 2;
    dev.ops.line(&dev, 0, 0, 10, 0, 23);
    if (test_buffer[0] != 23 || test_buffer[10] != 23 ||
        test_buffer[11] != 0) failures |= 4;
    dev.ops.fill(&dev, 5, 5, 4, 3, 29);
    if (test_buffer[5 * TEST_WIDTH + 5] != 29 ||
        test_buffer[7 * TEST_WIDTH + 8] != 29 ||
        test_buffer[8 * TEST_WIDTH + 5] != 0) failures |= 8;
    dev.ops.blit(&dev, &dev, 20, 20, 5, 5, 4, 3, RG_ROP_COPY);
    if (test_buffer[20 * TEST_WIDTH + 20] != 29 ||
        test_buffer[22 * TEST_WIDTH + 23] != 29) failures |= 16;
    return failures;
}

BOOL FAR PASCAL LibMain(HINSTANCE instance, WORD ds, WORD heap, LPSTR cmd)
{
    (void)instance; (void)ds; (void)heap; (void)cmd;
    return TRUE;
}
