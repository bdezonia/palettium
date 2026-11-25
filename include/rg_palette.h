#ifndef RG_PALETTE_H
#define RG_PALETTE_H
/* Hardware-independent INDEX8 palette and color-index translation.
 * All RGB entries are ordered red, green, blue, flags on the Win3.x wire.
 * Mapping is explicitly logical->physical; reverse uses a deterministic
 * inverse even when several logical indexes select one physical index.
 * C89 / Win16; OS-specific far pointers never enter this interface.
 */
#include "rgfx.h"

#define RG_PALETTE_SIZE 256u
#define RG_STATIC_COLORS 20u
#define RG_COLOR_INDEX_TAG 0xFF000000UL

typedef struct RG_Color {
    rg_u8 red, green, blue, flags;
} RG_Color;

typedef struct RG_Palette {
    RG_Color entries[RG_PALETTE_SIZE];
    unsigned short logical_to_physical[RG_PALETTE_SIZE];
    unsigned short physical_to_logical[RG_PALETTE_SIZE];
    unsigned initialized;
} RG_Palette;

void rg_palette_init(RG_Palette *p);
int rg_palette_is_static(unsigned index);
int rg_palette_set_entry(RG_Palette *p, unsigned index, RG_Color color);
int rg_palette_get_entry(const RG_Palette *p, unsigned index, RG_Color *out);
int rg_palette_set_translation(RG_Palette *p, const unsigned short *map);
void rg_palette_rebuild_inverse(RG_Palette *p);
/* Return 1 on success; on failure leave the output unchanged. */
int rg_palette_translate(const RG_Palette *p, unsigned logical_index,
                         unsigned *physical_index);
int rg_palette_untranslate(const RG_Palette *p, unsigned physical_index,
                           unsigned *logical_index);
/* Win3.x ColorInfo: 0x00BBGGRR or 0xFF00iiii; index and physical
 * input are 0..255. Returns zero on invalid arguments. A successful
 * forward conversion returns both the physical index and logical result.
 */
int rg_palette_color_info(const RG_Palette *p, rg_pixel input,
                          int to_physical, rg_pixel *logical,
                          rg_pixel *physical);
/* The translation is phys->phys; validate before touching pixels.
 * Rectangle is [x,x+w) x [y,y+h), entirely inside a live INDEX8 device.
 */
int rg_palette_remap_rect(RG_Device *dev, unsigned x, unsigned y,
                          unsigned w, unsigned h,
                          const unsigned short *translate);
#endif
