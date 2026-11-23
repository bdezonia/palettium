#include "rg_gdi_rop.h"

rg_u8 rg_gdi_rop3_byte(unsigned rop3, rg_u8 pattern,
                      rg_u8 source, rg_u8 dest)
{
    unsigned p = pattern;
    unsigned s = source;
    unsigned d = dest;
    unsigned out = 0;
    unsigned b;
    rop3 &= 255U;
    for (b = 0; b < 8; ++b) {
        unsigned index = (((p >> b) & 1U) << 2) |
                         (((s >> b) & 1U) << 1) |
                         ((d >> b) & 1U);
        out |= ((rop3 >> index) & 1U) << b;
    }
    return (rg_u8)out;
}

unsigned rg_gdi_rop3_code(unsigned long win_rop)
{
    return (unsigned)((win_rop >> 16) & 255UL);
}

int rg_gdi_rop3_to_rop2(unsigned rop3)
{
    unsigned lo = rop3 & 15U;
    if (((rop3 >> 4) & 15U) != lo) return -1;
    return (int)lo;
}

rg_u8 rg_gdi_rop2_byte(unsigned rop2, rg_u8 pen, rg_u8 dest)
{
    unsigned truth, b, out = 0;
    if (rop2 < 1u || rop2 > 16u) return dest;
    truth = rop2 - 1u;
    for (b=0; b<8u; ++b) {
        unsigned bit = (((unsigned)pen >> b) & 1u) * 2u +
                       (((unsigned)dest >> b) & 1u);
        out |= ((truth >> bit) & 1u) << b;
    }
    return (rg_u8)out;
}
