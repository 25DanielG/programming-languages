/*
** ATCS.H include file
**
** This header file is used in Advanced Topics in Computer Science
** It defines some useful constants and types
**
** May 19, 2020 - Added the QWORD type
**
** Dr. Eric R. Nelson
** November 18, 2013
**
*/
#ifndef ATCS_H
#define ATCS_H

#ifndef NULL
#define NULL  0
#endif

#ifndef O_TEXT
#define O_TEXT 0
#endif

#ifndef O_BINARY
#define O_BINARY 0
#endif

#ifndef O_RDONLY
#define O_RDONLY 0
#endif


#ifndef S_IREAD
#define S_IREAD S_IRUSR
#endif

#ifndef S_IWRITE
#define S_IWRITE S_IWUSR
#endif

#define  NUL   '\0'
#define  NIL   ((void *)0)

#define LONG int   // this type needs to be 4 bytes. On 32-bit machines change int to long.

typedef unsigned char      BYTE;
typedef unsigned short     WORD;
typedef unsigned LONG      DWORD;
typedef unsigned long long QWORD;

typedef short  BOOL;

typedef WORD*  PWORD;
typedef DWORD* PDWORD;
typedef LONG*  PLONG;
typedef char*  PSTR;
typedef BYTE*  PBYTE;
typedef BOOL*  PBOOL;
typedef int*   PINT;

#define MAKELONG(x,y) ((LONG)(((WORD)(x))|((DWORD)((WORD)(y)))<<16))
#define LOWORD(x) ((WORD)(x))
#define HIWORD(x) ((WORD)(((DWORD)(x)>>16) & 0xFFFF))
#define LOBYTE(x) ((BYTE)(x))
#define HIBYTE(x) ((BYTE)(((WORD)(x)>>8) & 0xFF))


#define SWAPDWORD(v) ((((((DWORD)(v))>>24) & (DWORD)0xFF) | (((DWORD)(v))<<24) | ((((DWORD)(v))<<8) & (DWORD)0xFF0000) | ((((DWORD)(v))>>8) & (DWORD)0xFF00)))

#define SWAPWORD(v) (((((WORD)(v))<<8) & (WORD)0xFF00) | ((((WORD)(v))>>8) & (WORD)0xFF))


#ifndef FALSE
#define  FALSE ((BOOL)0)
#define  TRUE  (!FALSE)
#endif

#endif

