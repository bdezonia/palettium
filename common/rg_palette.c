#include <limits.h>
#include <string.h>
#include "rg_palette.h"

/* The Windows 3.x 20-entry default system palette. First ten entries
 * occupy 0..9; last ten occupy 246..255. Static colors must remain fixed.
 * Values are the logical 8-bit RGB triplets from Microsoft's 3.x DDK.
 */
static const rg_u8 rg_static_rgb[20][3] = {
    {0,0,0}, {128,0,0}, {0,128,0}, {128,128,0},
    {0,0,128}, {128,0,128}, {0,128,128}, {192,192,192},
    {192,220,192}, {166,202,240},
    {255,251,240}, {160,160,164}, {128,128,128}, {255,0,0},
    {0,255,0}, {255,255,0}, {0,0,255}, {255,0,255},
    {0,255,255}, {255,255,255}
};

int rg_palette_is_static(unsigned index)
{
    return index < 10u || (index >= 246u && index < 256u);
}

void rg_palette_init(RG_Palette *p)
{
    unsigned i, pos;
    if (!p) return;
    memset(p, 0, sizeof(*p));
    for (i=0; i<256u; ++i) {
        p->logical_to_physical[i] = (unsigned short)i;
        p->physical_to_logical[i] = (unsigned short)i;
    }
    for (i=0; i<20u; ++i) {
        pos = i < 10u ? i : (i+236u);
        p->entries[pos].red = rg_static_rgb[i][0];
        p->entries[pos].green = rg_static_rgb[i][1];
        p->entries[pos].blue = rg_static_rgb[i][2];
    }
    p->initialized = 1;
}

int rg_palette_set_entry(RG_Palette *p, unsigned index, RG_Color color)
{
    if (!p || !p->initialized || index >= 256u ||
        rg_palette_is_static(index)) return 0;
    color.flags = 0;
    p->entries[index] = color;
    return 1;
}

int rg_palette_get_entry(const RG_Palette *p, unsigned index, RG_Color *out)
{
    if (!p || !p->initialized || !out || index >= 256u) return 0;
    *out = p->entries[index];
    out->flags = 0;
    return 1;
}

/* Resolve duplicate physical targets in favor of their lowest logical
 * source, but keep identity for unreferenced targets. */
void rg_palette_rebuild_inverse(RG_Palette *p)
{
    unsigned i, phy;
    rg_u8 visited[RG_PALETTE_SIZE];
    if (!p || !p->initialized) return;
    memset(visited, 0, sizeof(visited));
    for (i=0; i<256u; ++i) p->physical_to_logical[i] = (unsigned short)i;
    for (i=0; i<256u; ++i) {
        phy = p->logical_to_physical[i];
        if (phy < 256u && !visited[phy]) {
            p->physical_to_logical[phy] = (unsigned short)i;
            visited[phy] = 1;
        }
    }
}

int rg_palette_set_translation(RG_Palette *p, const unsigned short *map)
{
    unsigned i;
    if (!p || !p->initialized) return 0;
    if (map) {
        for (i=0; i<256u; ++i)
            if (map[i] >= 256u ||
                (rg_palette_is_static(i) && map[i] != i)) return 0;
        /* A reset table means identity; map is allowed to alias the field. */
        for (i=0; i<256u; ++i)
            p->logical_to_physical[i] = map[i];
    } else {
        for (i=0; i<256u; ++i)
            p->logical_to_physical[i] = (unsigned short)i;
    }
    rg_palette_rebuild_inverse(p);
    return 1;
}

unsigned rg_palette_translate(const RG_Palette *p, unsigned logical_index)
{
    if (!p || !p->initialized || logical_index >= 256u) return 256u;
    return p->logical_to_physical[logical_index];
}

static rg_pixel rg_colorref(const RG_Color *color)
{
    return ((rg_pixel)color->blue << 16) |
           ((rg_pixel)color->green << 8) | color->red;
}

int rg_palette_color_info(const RG_Palette *p, rg_pixel input,
                          int to_physical, rg_pixel *logical,
                          rg_pixel *physical)
{
    unsigned i, chosen=0;
    rg_pixel best=~0UL, distance;
    int r, g, b, dr, dg, db;
    if (!p || !p->initialized || !logical) return 0;
    if (to_physical) {
        if (!physical) return 0;
        if ((input & 0xFFFF0000UL) == 0xFF000000UL) {
            chosen = (unsigned)(input & 0xFFFFUL);
            if (chosen >= 256u) return 0;
            *physical = (rg_pixel)p->logical_to_physical[chosen];
            *logical = input;
            return 1;
        }
        if (input & 0xFF000000UL) return 0;
        r = (int)(input & 255UL);
        g = (int)((input >> 8) & 255UL);
        b = (int)((input >> 16) & 255UL);
        for (i=0; i<256u; ++i) {
            if (!rg_palette_is_static(i)) continue;
            dr = r - (int)p->entries[i].red;
            dg = g - (int)p->entries[i].green;
            db = b - (int)p->entries[i].blue;
            distance = (rg_pixel)((long)dr * dr) +
                       (rg_pixel)((long)dg * dg) +
                       (rg_pixel)((long)db * db);
            if (distance < best) { best=distance; chosen=i; }
        }
        *physical = (rg_pixel)chosen;
        *logical = rg_colorref(&p->entries[chosen]);
        return 1;
    }
    if ((input & 0xFFFF0000UL) == 0xFF000000UL) {
        if ((input & 0xFFFFUL) >= 256UL) return 0;
        *logical = input;
        return 1;
    }
    if (input >= 256UL) return 0;
    chosen = (unsigned)input;
    /* GDI handles dynamic colors by index, not potentially stale RGB. */
    if (!rg_palette_is_static(chosen)) {
        *logical = RG_COLOR_INDEX_TAG | (rg_pixel)p->physical_to_logical[chosen];
    } else {
        *logical = rg_colorref(&p->entries[chosen]);
    }
    return 1;
}

int rg_palette_remap_rect(RG_Device *dev, unsigned x, unsigned y,
                          unsigned w, unsigned h,
                          const unsigned short *translate)
{
    unsigned i, j;
    rg_pixel old;
    if (!dev || !translate || dev->surface.format != RG_FMT_INDEX8 ||
        !dev->ops.pixel || x > dev->surface.width ||
        y > dev->surface.height || w > dev->surface.width-x ||
        h > dev->surface.height-y ||
        x > (unsigned)INT_MAX || y > (unsigned)INT_MAX ||
        w > (unsigned)INT_MAX-x || h > (unsigned)INT_MAX-y) return 0;
    for (i=0; i<256u; ++i) if (translate[i] >= 256u) return 0;
    for (j=0; j<h; ++j) for (i=0; i<w; ++i) {
        if (!rg_get_pixel(dev, (int)(x+i), (int)(y+j), &old)) return 0;
        if (old >= 256UL) return 0;
        if (old != (rg_pixel)translate[(unsigned)old])
            dev->ops.pixel(dev, (int)(x+i), (int)(y+j),
                           (rg_pixel)translate[(unsigned)old]);
    }
    return 1;
}
