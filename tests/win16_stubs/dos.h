#ifndef PALDIAG_TEST_DOS_H
#define PALDIAG_TEST_DOS_H
union REGS {
    struct { unsigned short ax; } x;
    struct { unsigned char al, ah, bl, bh; } h;
};
int int86(int interrupt_number, union REGS *in,union REGS *out);
/* Unlike the real Win16 MK_FP, this maps to a strictly bounded fake byte. */
void *paldiag_test_mk_fp(unsigned selector, unsigned offset);
#define MK_FP(selector,offset) paldiag_test_mk_fp((selector),(offset))
#endif
