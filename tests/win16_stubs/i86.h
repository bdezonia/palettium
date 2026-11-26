#ifndef PALENH_HOST_I86_H
#define PALENH_HOST_I86_H
/* Host-only shape for PALENH; not a Win16 ABI emulation. */
union REGS { struct { unsigned short ax,bx,cx,dx,si,di,cflag; } w; };
struct SREGS { unsigned short es,cs,ss,ds; };
void segread(struct SREGS *s);
int int86x(int number,const union REGS *in,union REGS *out,struct SREGS *s);
#endif
