/* HOST SYNTAX-ONLY compatibility shims for palddi.c.
 * NOT an emulation of segmented memory or the Windows 3.1 ABI.
 * Real Win16 compiler/linker validation must be performed with Open Watcom.
 */
#include <windows.h>
typedef void *LPVOID;
typedef int BOOL;
#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif
#define FP_OFF(pointer) 0u
#define FP_SEG(pointer) 0u
