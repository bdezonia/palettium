#include <stdio.h>
#include <string.h>
#include "rgfx.h"

#define W 160
#define H 120
static rg_u8 framebuffer[W * H];
static unsigned intercepted_lines = 0;

/* A mock card accelerator: this override delegates to software. */
static void mock_line(RG_Device *dev, int x0, int y0,
                      int x1, int y1, rg_u8 color)
{
    ++intercepted_lines;
    rg_software_line(dev, x0, y0, x1, y1, color);
}

static int write_ppm(const char *path)
{
    FILE *f = fopen(path, "wb");
    unsigned i;
    if (!f) return 0;
    fprintf(f, "P6\n%d %d\n255\n", W, H);
    for (i = 0; i < W * H; ++i) {
        rg_u8 rgb[3];
        rgb[0] = framebuffer[i];
        rgb[1] = (rg_u8)((framebuffer[i] * 5u) / 8u);
        rgb[2] = (rg_u8)(255u - framebuffer[i]);
        if (fwrite(rgb, 1, 3, f) != 3) { fclose(f); return 0; }
    }
    return fclose(f) == 0;
}

int main(void)
{
    RG_Device dev;
    memset(framebuffer, 0, sizeof(framebuffer));
    if (!rg_init(&dev, framebuffer, W, H, W)) return 1;
    dev.ops.fill(&dev, 10, 10, 140, 100, 40);
    dev.ops.fill(&dev, 25, 25, 110, 70, 100);
    dev.ops.fill(&dev, -10, -10, 30, 30, 160); /* clipping */
    dev.ops.line = mock_line; /* example optional override */
    dev.ops.line(&dev, 0, 0, 159, 119, 255);
    dev.ops.line(&dev, 0, 119, 159, 0, 220);
    dev.ops.line(&dev, 80, 0, 80, 119, 200);
    if (intercepted_lines != 3) return 2;
    if (framebuffer[80 * W + 80] != 200) return 3;
    if (!write_ppm("raster-demo.ppm")) return 4;
    puts("PASS: raster demo created raster-demo.ppm; mock override called 3 times");
    return 0;
}
