#ifndef RGFX_H
#define RGFX_H

/* C89-compatible API. All pixel data is 8-bit indexed color. */
typedef unsigned char rg_u8;

typedef struct RG_Surface {
    rg_u8 *pixels;
    unsigned width;
    unsigned height;
    unsigned stride; /* bytes per scanline, at least width */
} RG_Surface;

struct RG_Device;
typedef void (*RG_PixelFn)(struct RG_Device *, int, int, rg_u8);
typedef void (*RG_LineFn)(struct RG_Device *, int, int, int, int, rg_u8);
typedef void (*RG_FillFn)(struct RG_Device *, int, int, int, int, rg_u8);

typedef struct RG_Ops {
    RG_PixelFn pixel;
    RG_LineFn line;
    RG_FillFn fill; /* x,y,width,height; nonpositive dimensions do nothing */
} RG_Ops;

typedef struct RG_Device {
    RG_Surface surface;
    RG_Ops ops;     /* card backends can override individual functions */
    void *card_data;
} RG_Device;

int rg_init(RG_Device *dev, rg_u8 *pixels,
            unsigned width, unsigned height, unsigned stride);
void rg_software_pixel(RG_Device *dev, int x, int y, rg_u8 color);
void rg_software_line(RG_Device *dev, int x0, int y0,
                      int x1, int y1, rg_u8 color);
void rg_software_fill(RG_Device *dev, int x, int y,
                      int width, int height, rg_u8 color);

#endif
