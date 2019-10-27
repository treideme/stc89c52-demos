/**
 * @file hardware.h Project hardware configuration.
 * @author Thomas Reidemeister <treideme@gmail.com>
 */
#ifndef _hardware_h_
#define _hardware_h_

#include <mcs51/8051.h>
#include "stc89xx.h"

/******************************************************************************\
 * Public type definitions
\******************************************************************************/

/******************************************************************************\
 * Public macros
\******************************************************************************/

#define FREQ_SYS   12000000 // FSys for MCS-51 system
#define MODE_6T    1        // Using 6T (double frequency mode)

/*******************************************************************************
**  Binary constant macros
*/
#define B8(d)                   ((uint8_t)  0b##d)                    ///<  8-bit shorthand. @note Mask needs exactly 8 digits
#define B16(dmsb,dlsb)          ((uint16_t) 0b##dmsb##dlsb)           ///< 16-bit shorthand. @note Mask needs exactly 8,8 digits
#define B32(dmsb,db2,db3,dlsb)  ((uint32_t) 0b##dmsb##db2##db3##dlsb) ///< 32-bit shorthand. @note Mask needs exactly 8,8,8,8 digits

#include "pin.h"

#define INIT_PCON \
  B8(00000000) /**< PCON  Power Control Register. @verbatim
     00010000 = reset
     || |||||
     || ||||+-- (RW)  IDL  : Idle Mode flag, set to 1 for idle
     || |||+--- (RW)  PD   : Set to 1 to sleep, cleared on wake-up (reset)
     || ||+---- (RW)  GF0  : General flag 0
     || |+----- (RW)  GF1  : General flag 1
     || +------ (W0C) POF  : Power on flag (clear after boot)
     ||
     |+-------- (RW)  SMOD0: SM0/FE bit select for SCON.7
     +--------- (RW)  SMOD : UART0 slow (0) or fast mode (1) when T1 is used

  @endverbatim
*/

#define INIT_SCON \
  B8(01010000) /**< SCON  Serial Control. @verbatim
     00000000 = reset
     ||||||||
     |||||||+-- (RW)  RI   : Receive Interrupt Flag
     ||||||+--- (RW)  TI   : Transmit Interrupt Flag
     |||||+---- (RO)  RB8  : 9-th received bit in 9-bit mode
     ||||+----- (RW)  TB8  : 9-th transmitted bit in 9-bit mode
     |||+------ (RW)  REN  : Enable receiver
     ||+------- (RW)  SM2  : Serial Port Mode Bit 2 (Auto Address recognition)
     |+-------- (RW)  SM1  : Serial Port Mode Bit 1
     +--------- (RW)  SM0  : Serial Port Mode Bit 0

  | SM0  | SM1  | Bandwith | Baud Source
  |------|------|----------|------------
  |  0   |  0   | 8-bit    | SysCLK/12
  |  0   |  1   | 8-bit    | Variable
  |  1   |  0   | 9-bit    | SysCLK/64 or SysCLK/32 (SMOD)
  |  1   |  1   | 9-bit    | Variable

  @endverbatim
*/

/******************************************************************************
 * Timers
 */

#define INIT_TCON \
  B8(00000000) /**< TCON  Timer Control. @verbatim
     00000000 = reset
     ||||||||
     |||||||+-- (RW)  IT0 : Interrupt 0 type control bit (Low level)
     ||||||+--- (RO)  IE0 : Interrupt 0 edge flag
     |||||+---- (RO)  IT1 : Interrupt 1 type control bit (Low level)
     ||||+----- (RW)  IE1 : Interrupt 1 edge flag
     |||+------ (RW)  TR0 : Timer run 0 control bit (off)
     ||+------- (RO)  TF0 : Timer 0 overflow flag
     |+-------- (RW)  TR1 : Timer run 1 control bit (off)
     +--------- (RO)  TF1 : Timer 1 overflow flag

  @endverbatim
*/

#define INIT_TMOD \
  B8(00010000) /**< TMOD  Timer Mode. @verbatim
     00000000 = reset
     ||||||||
     |||||||+-- (RW)  M0   : Timer 0 Mode bit 0
     ||||||+--- (RW)  M1   : Timer 0 Mode bit 1 (16-bit counter)
     |||||+---- (RW)  C/~T : Timer 0 Timer or counter (timer)
     ||||+----- (RW)  GATE : Timer 0 Timer enabled whenever TR bit is set
     |||+------ (RW)  M0   : Timer 1 Mode bit 0
     ||+------- (RW)  M1   : Timer 1 Mode bit 1
     |+-------- (RW)  C/~T : Timer 1 Timer or counter (timer)
     +--------- (RW)  GATE : Timer 1 Timer enabled whenever TR bit is set

  | M1 | M0 | Operating Mode
  |----|----|---------------
  | 0  | 0  | 8-bit Timer/Counter "THx" with "TLx" as 5-bit prescaler.
  | 0  | 1  | 16-bit Timer/Counter"THx"and"TLx"are cascaded;there is no prescaler
  | 1  | 0  | 8-bit auto-reload Timer/Counter “THx” holds a value which is to be reloaded into “TLx” each time it overflows
  | 1  | 1  | (T0) TL0 is an 8-bit Timer/Counter controlled by the standard Timer 0 control bits TH0 is an 8-bit timer only controlled by Timer 1 control bits. (T1) is stopped

  @endverbatim
*/

#define INIT_T2_CON \
  B8(00110100) /**< T2CON  Timer 2 Control. @verbatim
     00000000 = reset
     ||||||||
     |||||||+-- (RW)  CP/~RL2: Auto reload
     ||||||+--- (RW)  C/~T2  : Timer 2 as internal timer
     |||||+---- (RW)  TR2    : Timer 2 run bit
     ||||+----- (RW)  EXEN2  : External enable flag
     |||+------ (RW)  TCLK   : Transmit clock flag (baud rate generation)
     ||+------- (RW)  RCLK   : Receive clock flag (baud rate generation)
     |+-------- (RW)  EXF2   : Timer 2 external flag
     +--------- (RW)  TF2    : Timer 2 overflow flag

  @endverbatim
*/

#define INIT_T2_MOD \
  B8(00000000) /**< T2CON  Timer 2 Control. @verbatim
     00000000 = reset
     xxxxxx||
           |+-- (RW)  DCEN   : Down count enable bit
           +--- (RW)  T2OE   : Timer 2 output enable bit

  @endverbatim
*/

/*******************************************************************************
 * Auxialiary RAM
 */

#define INIT_AUXR \
  B8(00000001) /**< AUXR  Auxiliary Register 0. @verbatim
     00000000 = reset
           ||
           |+-- (RW)  ALEOFF : Ale only active during MOVX, MOVC
           +--- (RW)  EXTRAM : On-chip RAM enabled at 00H - FFH

  @endverbatim
*/

#define INIT_WDT_CONTR \
  B8(00000000) /**< WDT_CONTR  Watchdog control register. @verbatim
     00000000 = reset
       ||||||
       |||+++-- (RW)  PS   : Prescaler (see below)
       |||
       |||
       ||+----- (RW)  IDLE : When set, WDT enabled in idle mode.
       |+------ (W1S) CLEAR: Clear watchdog timer (reset)
       +------- (RW)  EN   : Enable watchdog timer.

  | PS 0 | PS 1 | PS 2 | Prescale | WDT Overflow time @ 12Mhz
  |------|------|------|----------|--------------------------
  |  0   |  0   |  0   |    2     |   65.5  ms
  |  0   |  0   |  1   |    4     |  131.0  ms
  |  0   |  1   |  0   |    8     |  262.1  ms
  |  0   |  1   |  1   |   16     |  524.2  ms
  |  1   |  0   |  0   |   32     | 1048.5  ms
  |  1   |  0   |  1   |   64     | 2097    ms
  |  1   |  1   |  0   |  128     | 4194.3  ms
  |  1   |  1   |  1   |  256     | 8388.6  ms

  @endverbatim
*/

/*******************************************************************************
** Interrupts FIXME
*/

#define INIT_IE \
  B8(10011000) /**< IE  Interrupt Enable Register.
     00000000 = reset
     ||||||||
     |||||||+-- (RW)  EX0  : External Interrupt 0
     ||||||+--- (RW)  ET0  : Timer Interrupt 0
     |||||+---- (RW)  EX1  : External Interrupt 1
     ||||+----- (RW)  ET1  : Timer 1 Interrupt
     |||+------ (RO)  ES   : UART 0 Interrupt
     ||+------- (RO)  ET2  : Timer 2 Interrupt
     |+-------- (RW)  E_DIS: Global Interrupt Disable
     +--------- (RW)  EA   : Global Interrupt Enable

  | RSTF1 | RSTF0 | Description
  |-------|-------|---------------------------------------------------------------------
  |   0   |   0   | Software reset, source: bSW_RESET=1 and (bBOOT_LOAD=0 or bWDOG_EN=1)
  |   0   |   1   | Power-on reset, source: VDD33 pin voltage is lower than detection level
  |   1   |   0   | Watchdog reset, source: bWDOG_EN=1 and watchdog timeout
  |   1   |   1   | External pin is manually reset, source: En_P5.7_RESET=1 and P5.7 input high

  @endverbatim
*/

/******************************************************************************\
 * Public memory declarations
\******************************************************************************/

/******************************************************************************\
 * Public functions
\******************************************************************************/

void hardware_init();


#endif /* _hardware_h_ */
