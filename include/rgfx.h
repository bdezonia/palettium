#ifndef RGFX_H
#define RGFX_H
/* Palettium unified raw-pixel API. C89 / Win16 compatible.
 * Pixel values are raw framebuffer bits; palette interpretation is separate.
 * 32-bit byte offsets; OS adapters own segmented VRAM access.
 */
typedef unsigned char rg_u8;
typedef unsigned long rg_offset;
typedef unsigned long rg_pixel;

typedef enum RG_PixelFormat {
    RG_FMT_INDEX1 = 1, RG_FMT_INDEX4, RG_FMT_INDEX8,
    RG_FMT_RGB555, RG_FMT_RGB565, RG_FMT_RGB888,
    RG_FMT_XRGB8888, RG_FMT_ARGB8888
} RG_PixelFormat;

struct RG_Device;
typedef rg_u8 (*RG_ReadByteFn)(void *, rg_offset);
typedef void (*RG_WriteByteFn)(void *, rg_offset, rg_u8);
typedef struct RG_VramOps {
    RG_ReadByteFn read_byte;
    RG_WriteByteFn write_byte;
} RG_VramOps;

typedef struct RG_Surface {
    rg_u8 *pixels; /* host/short buffers only; Win16 screen uses callbacks */
    unsigned width, height;
    rg_offset stride;    /* bytes per row, NOT pixels */
    rg_offset capacity;  /* bytes available from offset zero */
    RG_PixelFormat format;
} RG_Surface;

typedef void (*RG_PixelFn)(struct RG_Device *, int, int, rg_pixel);
typedef void (*RG_LineFn)(struct RG_Device *, int, int, int, int, rg_pixel);
typedef void (*RG_FillFn)(struct RG_Device *, int, int, int, int, rg_pixel);
/* 4-bit truth table with index (source_bit<<1)|destination_bit. */
#define RG_ROP_CLEAR 0x0
#define RG_ROP_AND 0x8
#define RG_ROP_COPY 0xC
#define RG_ROP_XOR 0x6
#define RG_ROP_OR 0xE
#define RG_ROP_SET 0xF

typedef void (*RG_BlitFn)(struct RG_Device *, struct RG_Device *,
                          int, int, int, int, int, int, unsigned);
typedef struct RG_Ops {
    RG_PixelFn pixel;
    RG_LineFn line;
    RG_FillFn fill;
    RG_BlitFn blit;
} RG_Ops;
typedef struct RG_Device {
    RG_Surface surface;
    RG_Ops ops;
    void *card_data;
    RG_VramOps vram;
    void *vram_context;
} RG_Device;

unsigned rg_pixel_bits(RG_PixelFormat format);
rg_offset rg_min_stride(RG_PixelFormat format, unsigned width);
rg_pixel rg_rgb_pack(RG_PixelFormat format, rg_u8 r, rg_u8 g, rg_u8 b, rg_u8 a);
/* Memory-backed initialization requires addressable entire buffer.
 * On Win16 screen VRAM, always use rg_init_vram with far-access callbacks.
 */
int rg_init(RG_Device *, rg_u8 *, unsigned, unsigned, rg_offset,
            rg_offset, RG_PixelFormat);
int rg_init_vram(RG_Device *, unsigned, unsigned, rg_offset,
                 rg_offset, RG_PixelFormat, RG_VramOps, void *);
rg_u8 rg_read_byte(RG_Device *, rg_offset);
void rg_write_byte(RG_Device *, rg_offset, rg_u8);
int rg_get_pixel(RG_Device *, int, int, rg_pixel *);
int rg_put_pixel(RG_Device *, int, int, rg_pixel);
void rg_software_pixel(RG_Device *, int, int, rg_pixel);
void rg_software_line(RG_Device *, int, int, int, int, rg_pixel);
void rg_software_fill(RG_Device *, int, int, int, int, rg_pixel);
void rg_software_blit(RG_Device *, RG_Device *,
                      int, int, int, int, int, int, unsigned);
#endif
