/* v0024: exercise Win31 physical INDEX8 Pixel/ROP2 without Win16 SDK.
 * No DDI far-pointer values are dereferenced, and hardware overrides survive.
 */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "pal_screen_ddi.h"

#define W 32
#define H 16
#define TOKEN 0xA3120050UL
static rg_u8 pixels[W * H];
static unsigned pixel_calls;
static void override_pixel(RG_Device *d, int x, int y, rg_pixel value)
{
    ++pixel_calls;
    rg_software_pixel(d, x, y, value);
}
static int op(PAL_ScreenDDI *b, PAL_U32 token, int x, int y,
              PAL_U32 color, unsigned rop2, PAL_U32 *out)
{
    return pal_screen_ddi_pixel(b, token, x, y, color, rop2, out);
}
int main(void)
{
    RG_Device d;
    PAL_PDEVICE pd;
    PAL_ScreenDDI b;
    PAL_U32 out, keep;
    unsigned i, j, src, dst;
    memset(&b, 0, sizeof(b));
    memset(pixels, 0x5A, sizeof(pixels));
    assert(rg_init(&d, pixels, W, H, W, sizeof(pixels), RG_FMT_INDEX8));
    assert(pal_pd_prepare(&pd, W, H, W));
    assert(pal_pd_activate(&pd));
    assert(pal_screen_ddi_attach(&b, &pd, &d, TOKEN));
    d.ops.pixel = override_pixel;
    out = 0xDEADBEEFUL;
    assert(op(&b, TOKEN, 5, 7, 0, 0, &out) && out == 0x5AUL);
    assert(pixel_calls == 0);
    /* GDI 1..16 is the shared engine's truth table 0..15 plus one.
       Check every ROP2 with every source/destination bit combination. */
    for (i = 1; i <= 16; ++i) {
        for (src = 0; src <= 255; src += 255) {
            for (dst = 0; dst <= 255; dst += 255) {
                unsigned table = i - 1u;
                unsigned expect = 0;
                unsigned bit;
                for (bit = 0; bit < 8; ++bit) {
                    unsigned index = (((src >> bit) & 1u) << 1) |
                                     ((dst >> bit) & 1u);
                    if (table & (1u << index)) expect |= 1u << bit;
                }
                pixels[7 * W + 5] = (rg_u8)dst;
                out = 0;
                assert(op(&b, TOKEN, 5, 7, src, i, &out));
                assert(out == 1 && pixels[7 * W + 5] == expect);
            }
        }
    }
    assert(pixel_calls == 16u * 4u);
    /* Test mixed-bit ROP and verify write dispatch through accelerated op. */
    pixels[7 * W + 5] = 0x55;
    j = pixel_calls;
    assert(op(&b, TOKEN, 5, 7, 0xF0, 7, &out)); /* XORPEN */
    assert(pixels[7 * W + 5] == 0xA5 && pixel_calls == j + 1);
    assert(op(&b, TOKEN, 5, 7, 0xFF, 13, &out)); /* COPYPEN */
    assert(pixels[7 * W + 5] == 0xFF);
    keep = 0xC0FFEEUL;
    memcpy(&out, &keep, sizeof(out));
    j = pixel_calls;
    assert(!op(&b, TOKEN+1, 5, 7, 0, 13, &out));
    assert(!op(&b, 0, 5, 7, 0, 13, &out));
    assert(!op(&b, TOKEN, -1, 7, 0, 13, &out));
    assert(!op(&b, TOKEN, 5, H, 0, 13, &out));
    assert(!op(&b, TOKEN, 5, 7, 256, 13, &out));
    assert(!op(&b, TOKEN, 5, 7, 2, 17, &out));
    assert(!op(&b, TOKEN, 5, 7, 2, 13, 0));
    assert(out == keep && pixel_calls == j && pixels[7 * W + 5] == 0xFF);
    assert(pal_pd_stop(&pd));
    assert(!op(&b, TOKEN, 5, 7, 0, 0, &out));
    assert(pal_pd_activate(&pd));
    pal_screen_ddi_detach(&b);
    assert(!op(&b, TOKEN, 5, 7, 0, 0, &out));
    puts("PASS: Win31 Pixel read/write, 16 ROP2s, token, bounds, lifecycle, override");
    return 0;
}
