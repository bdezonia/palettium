#ifndef RGFX_H
#define RGFX_H

/* Palettium: C89, 8-bit indexed surfaces. rg_offset is at least 32 bits. */
typedef unsigned char rg_u8;
typedef unsigned long rg_offset;

struct RG_Device;
typedef rg_u8 (*RG_ReadByteFn)(void *context, rg_offset offset);
typedef void (*RG_WriteByteFn)(void *context, rg_offset offset, rg_u8 value);

typedef struct RG_VramOps {
    RG_ReadByteFn read_byte;
    RG_WriteByteFn write_byte;
} RG_VramOps;

typedef struct RG_Surface {
    rg_u8 *pixels;       /* NULL when using callback-based framebuffer */
    unsigned width;
    unsigned height;
    unsigned stride;    /* bytes per row; >= width */
} RG_Surface;

typedef void (*RG_PixelFn)(struct RG_Device *, int, int, rg_u8);
typedef void (*RG_LineFn)(struct RG_Device *, int, int, int, int, rg_u8);
typedef void (*RG_FillFn)(struct RG_Device *, int, int, int, int, rg_u8);

typedef struct RG_Ops {
    RG_PixelFn pixel;
    RG_LineFn line;
    RG_FillFn fill;
} RG_Ops;

typedef struct RG_Device {
    RG_Surface surface;
    RG_Ops ops;
    void *card_data;      /* reserved for hardware backend */
    RG_VramOps vram;
    void *vram_context;
} RG_Device;

/* Use a directly addressable framebuffer (legacy v0.1 API). */
int rg_init(RG_Device *dev, rg_u8 *pixels,
            unsigned width, unsigned height, unsigned stride);
/* Use mapped/banked VRAM through byte callbacks. */
int rg_init_vram(RG_Device *dev, unsigned width, unsigned height,
                 unsigned stride, RG_VramOps ops, void *context);
rg_u8 rg_read_byte(RG_Device *dev, rg_offset offset);
void rg_write_byte(RG_Device *dev, rg_offset offset, rg_u8 value);
void rg_software_pixel(RG_Device *dev, int x, int y, rg_u8 color);
void rg_software_line(RG_Device *dev, int x0, int y0,
                      int x1, int y1, rg_u8 color);
void rg_software_fill(RG_Device *dev, int x, int y,
                      int width, int height, rg_u8 color);

#endif
