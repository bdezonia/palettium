#include "rg_gdi_rop.h"
#include <stdio.h>

static unsigned failures;
static void check(int condition, const char *name)
{
    if (!condition) { ++failures; printf("FAIL: %s\n", name); }
}

/* An independently expressed bitplane reference using minterms. */
static unsigned reference(unsigned r, unsigned p, unsigned s, unsigned d)
{
    unsigned result = 0;
    unsigned i;
    for (i = 0; i != 8U; ++i) {
        unsigned mask = ((i & 4U) ? p : (~p)) &
                        ((i & 2U) ? s : (~s)) &
                        ((i & 1U) ? d : (~d));
        if (r & (1U << i)) result |= mask;
    }
    return result & 255U;
}

int main(void)
{
    unsigned r, p, s, d;
    unsigned cases = 0;
    check(rg_gdi_rop3_code(0x00CC0020UL) == 0xCC, "SRCCOPY code");
    check(rg_gdi_rop3_code(0x00F00021UL) == 0xF0, "PATCOPY code");
    check(rg_gdi_rop3_code(0x005A0049UL) == 0x5A, "PATINVERT code");
    check(rg_gdi_rop3_byte(0xCC, 0xF0, 0x69, 0x12) == 0x69, "SRCCOPY");
    check(rg_gdi_rop3_byte(0xAA, 0xF0, 0x69, 0x12) == 0x12, "DSTCOPY");
    check(rg_gdi_rop3_byte(0xF0, 0xF0, 0x69, 0x12) == 0xF0, "PATCOPY");
    check(rg_gdi_rop3_byte(0x66, 0, 0x69, 0x12) == (0x69 ^ 0x12), "SRCINVERT");
    for (r = 0; r != 256U; ++r) {
        for (p = 0; p < 256U; p += 17U) {
            for (s = 0; s < 256U; s += 17U) {
                for (d = 0; d < 256U; d += 17U) {
                    ++cases;
                    if (rg_gdi_rop3_byte(r, (rg_u8)p, (rg_u8)s, (rg_u8)d) !=
                        reference(r, p, s, d)) {
                        check(0, "truth table");
                        return 1;
                    }
                }
            }
        }
        if ((r & 15U) == ((r >> 4) & 15U))
            check(rg_gdi_rop3_to_rop2(r) == (int)(r & 15U), "2-input reduction");
        else
            check(rg_gdi_rop3_to_rop2(r) == -1, "pattern-dependent detection");
    }
    if (failures) return 1;
    printf("PASS: GDI ROP3: %u exhaustive ROP/byte-sample comparisons, 16 pattern-independent reductions\n", cases);
    return 0;
}
