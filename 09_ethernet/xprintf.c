/*-------------------------------------------------------------------------
   xprintf.c - Tiny formatted output routine for use with sdcc/mcs51

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

/* Formatted string output:
  -------------------------
  xprintf("%c", 'a');             "a"
  xprintf("%s", "String");        "String"
  xprintf("%-4s", "abc");         "abc "
  xprintf("%4s", "abc");          " abc"
  xprintf("%04x", 0xA3);          "00a3"
  xprintf("%08LX", 0x123ABC);     "00123ABC"
  xprintf("%016b", 0x550F);       "0101010100001111"
  xprintf("%d", 1234);            "1234"
  xprintf("%6d,%3d%%", -200, 5);  "  -200,  5%"
  xprintf("%-6u", 100);           "100   "
  xprintf("%ld", 12345678L);      "12345678"
  xprintf("%p", xdata_pointer);   "X:0x1234"
  xprintf("%p", code_pointer);    "C:0x1234"
  xprintf("%p", data_pointer);    "I:0x1234"
*/

void xprintf (__code const char *fmt, ...) __reentrant __naked
{
     fmt; // r0-pointer, r1-radix, r2-width, r3-numctr, r4..r7-number(low..high)
     #define zero_padded    b.0
     #define left_justified b.1
     #define long_size      b.2
     #define upper_case     b.3
     #define signed_num     b.4
     #define negative_num   b.5
     __asm
          mov       a,sp
          add       a,#-2
          xch       a,r0
          push      a
          mov       a,r1
          push      a
          mov       a,r2
          push      a
          mov       a,r3
          push      a
          mov       a,r4
          push      a
          mov       a,r5
          push      a
          mov       a,r6
          push      a
          mov       a,r7
          push      a
          mov       dph,@r0
          dec       r0
          mov       dpl,@r0
          dec       r0
_p_loop:  clr       a
          movc      a,@a+dptr
          inc       dptr
          jnz       _p_fmt
_p_end:   pop       a
          mov       r7,a
          pop       a
          mov       r6,a
          pop       a
          mov       r5,a
          pop       a
          mov       r4,a
          pop       a
          mov       r3,a
          pop       a
          mov       r2,a
          pop       a
          mov       r1,a
          pop       a
          mov       r0,a
          ret
_p_fmt:   cjne      a,#'%',_p_out
          clr       a
          mov       b,a
          mov       r2,a
          mov       r3,a
          mov       r4,a
          mov       r5,a
          lcall     _p_get
          cjne      a,#'0',1$
          setb      zero_padded
          sjmp      2$
1$:       cjne      a,#'-',3$
          setb      left_justified
2$:       lcall     _p_get
3$:       lcall     _p_dec // width: 0..99
          jnc       4$
          mov       r2,a
          lcall     _p_get
          lcall     _p_dec
          jnc       4$
          xch       a,r2
          add       a,acc
          mov       r1,a
          add       a,acc
          add       a,acc
          add       a,r1
          add       a,r2
          mov       r2,a
          lcall     _p_get
4$:       jz        _p_end
          cjne      a,#'L',_p_sym
          setb      long_size
          lcall     _p_get
          jz        _p_end
_p_sym:   cjne      a,#'C',_p_str
          dec       r0
          mov       a,@r0
          dec       r0
_p_out:   lcall     _putc
          sjmp      _p_loop
_p_str:   push      dpl
          push      dph
          cjne      a,#'S',_p_nxt
          clr       zero_padded
          mov       a,@r0
          orl       b,a
          dec       r0
          mov       dph,@r0
          dec       r0
          mov       dpl,@r0
          mov       a,r2
          jz        3$
1$:       lcall     __gptrget
          jz        2$
          inc       dptr
          djnz      r2,1$
2$:       lcall     _p_space
          inc       r0
          mov       dph,@r0
          dec       r0
          mov       dpl,@r0
3$:       dec       r0
          lcall     _putstr
5$:       lcall     _p_space
          sjmp      _p_jmp
_p_nxt:   mov       r1,#10
          cjne      a,#'D',1$
          setb      signed_num
          sjmp      _p_num
1$:       cjne      a,#'U',2$
          sjmp      _p_num
2$:       mov       r1,#16
          cjne      a,#'X',3$
          sjmp      _p_num
3$:       cjne      a,#'P',4$
          sjmp      _p_ptr
4$:       cjne      a,#'B',5$
          mov       r1,#2
          sjmp      _p_num
5$:       cjne      a,#'O',6$
          mov       r1,#8
          sjmp      _p_num
6$:       lcall     _putc
_p_jmp:   pop       dph
          pop       dpl
          ljmp      _p_loop
_p_ptr:   mov       a,@r0
          dec       r0
          orl       b,a
          mov       r2,#4
          mov       a,#'C'
          jb        b.7,1$
          mov       a,#'X'
          jnb       b.6,1$
          mov       r2,#2
          mov       a,#'I'
1$:       lcall     _putc
          mov       a,#':'
          lcall     _putc
          mov       a,#'0'
          lcall     _putc
          mov       a,#'x'
          lcall     _putc
          anl       b,#1 << (upper_case - b)
          orl       b,#1 << (zero_padded - b)
_p_num:   jnb       long_size,0$
          mov       a,@r0
          mov       r4,a
          dec       r0
          mov       a,@r0
          mov       r5,a
          dec       r0
0$:       mov       a,@r0
          mov       r6,a
          dec       r0
          mov       a,@r0
          mov       r7,a
          dec       r0
          jnb       signed_num,2$
          jb        long_size,1$
          mov       a,r6
          jnb       a.7,2$
          mov       r4,#0xFF
          mov       r5,#0xFF
1$:       mov       a,r4
          jnb       a.7,2$
          mov       a,r7
          cpl       a
          add       a,#1
          mov       r7,a
          mov       a,r6
          cpl       a
          addc      a,#0
          mov       r6,a
          mov       a,r5
          cpl       a
          addc      a,#0
          mov       r5,a
          mov       a,r4
          cpl       a
          addc      a,#0
          mov       r4,a
          setb      negative_num
2$:       clr       a
          mov       dpl,#32
3$:       .macro    _xprn_shift
          xch       a,r7
          rlc       a
          xch       a,r7
          xch       a,r6
          rlc       a
          xch       a,r6
          xch       a,r5
          rlc       a
          xch       a,r5
          xch       a,r4
          rlc       a
          xch       a,r4
          .endm
          _xprn_shift
          rlc       a
          clr       c
          subb      a,r1
          jnc       .+3
          add       a,r1
          cpl       c
          djnz      dpl,3$
          _xprn_shift
          cjne      a,#10,.+3
          jc        .+9
          add       a,#'A'-10
          jb        upper_case,.+7
          add       a,#'a'-'A'-'0'
          add       a,#'0'
          push      a
          inc       r3
          mov       a,r7
          orl       a,r6
          orl       a,r5
          orl       a,r4
          jnz       2$
          jnb       negative_num,4$
          mov       a,#'-'
          push      a
          inc       r3
4$:       mov       a,r2
          subb      a,r3
          mov       r2,a
          jnc       .+4
          mov       r2,#0
          lcall     _p_space
5$:       pop       a
          lcall     _putc
          djnz      r3,5$
          lcall     _p_space
          ljmp      _p_jmp
_p_space: mov       a,r2
          jz        2$
          jbc       left_justified,2$
1$:       mov       c,zero_padded
          mov       a,#' '
          mov       a.4,c
          lcall     _putc
          djnz      r2,1$
2$:       ret
_p_get:   clr       a
          movc      a,@a+dptr
          inc       dptr
          cjne      a,#'a',.+3
          jc        1$
          subb      a,#'a'-'A'
1$:       mov       upper_case,c
          ret
_p_dec:   add       a,#-'0'
          cjne      a,#10,.+3
          jc        1$
          add       a,#'0'
          clr       c
1$:       ret
_putc:    push      dpl
          push      dph
          mov       dpl,a
          mov       dph,#0
          lcall     _putchar
          pop       dph
          pop       dpl
          ret
_putstr:  lcall     __gptrget
          jz        _putstr - 1
          inc       dptr
          lcall     _putc
          sjmp      _putstr
     __endasm;
}

