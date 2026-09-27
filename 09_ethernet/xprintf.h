/*-------------------------------------------------------------------------
   xprintf.h - Tiny formatted output header for use with sdcc/mcs51

   Copyright (C) 2022, Eugene Chaban, box4xyz@gmail.com

   This library is free software; you can redistribute it and/or modify it
   under the terms of the GNU General Public License as published by the
   Free Software Foundation; either version 2, or (at your option) any
   later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License 
   along with this library; see the file COPYING. If not, write to the
   Free Software Foundation, 51 Franklin Street, Fifth Floor, Boston,
   MA 02110-1301, USA.

   As a special exception, if you link this library with other files,
   some of which are compiled with SDCC, to produce an executable,
   this library does not by itself cause the resulting executable to
   be covered by the GNU General Public License. This exception does
   not however invalidate any other reasons why the executable file
   might be covered by the GNU General Public License.
-------------------------------------------------------------------------*/

#include <stdio.h>
#undef PUTC
#undef PUTS
#undef PRINTF
#if defined _PRINTF_ || defined _XPRINTF_
void xprintf (__code const char *fmt, ...) __reentrant __naked;
#define PUTC(c)     putchar(c)
#define PUTS(s)     puts(s)
#if defined _PRINTF_
#define PRINTF(...) printf(__VA_ARGS__)
#else
#define PRINTF(...) xprintf(__VA_ARGS__)
#endif
#else
#define PUTC(c)
#define PUTS(s)
#define PRINTF(...)
#endif

