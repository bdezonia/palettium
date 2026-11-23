#ifndef PALDIAG_TEST_DOS_H
#define PALDIAG_TEST_DOS_H
union REGS {
    struct { unsigned short ax; } x;
    struct { unsigned char al, ah, bl, bh; } h;
};
int int86(int interrupt_number, union REGS *in,union REGS *out);
#endif
