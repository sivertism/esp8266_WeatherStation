// ROM and blob calls without official headers available

#if !defined(__ESP8266_UNDOCUMENTED_H) && !(defined(_ASMLANGUAGE) || defined(__ASSEMBLER__))
#define __ESP8266_UNDOCUMENTED_H

#ifndef XCHAL_EXCCAUSE_NUM
// from tools/xtensa-lx106-elf/include/xtensa/config/core.h:629:#define XCHAL_EXCCAUSE_NUM  		64
#define XCHAL_EXCCAUSE_NUM  		64
#endif

// ROM

extern void rom_i2c_writeReg_Mask(int, int, int, int, int, int);
extern int rom_i2c_readReg_Mask(int, int, int, int, int);

extern int uart_baudrate_detect(int, int);

/*
ROM function, uart_buff_switch(), is used to switch printing between UART0 and
UART1. It updates a structure that only controls a select group of print
functions. ets_putc() and ets_uart_printf() are examples and are not affected by
calls to ets_install_putc1().

 Use:
   0 for UART0, also clears RX FIFO
   1 for UART1
 */
extern void uart_buff_switch(uint8_t);

/*
 ROM function, ets_uart_printf(), prints on the UART selected by
 uart_buff_switch(). Supported format options are the same as vprintf(). Also
 has cooked newline behavior. No flash format/string support; however, ISR safe.
 Also, uses a static function in ROM to print characters which is only
 controlled by uart_buff_switch().
 */
extern int ets_uart_printf(const char *format, ...) __attribute__ ((format (printf, 1, 2)));

extern void ets_delay_us(uint16_t us);

#ifndef GDBSTUB_H
/*
  GDBSTUB duplicates these with some variances that are not compatible with our
  references (offsets), which are synced with those used by the BootROM.
  Specifically, the BootROM does not have register "a1" in the structure where
  GDBSTUB does.
*/

/*
  This structure is used in the argument list of "C" callable exception handlers.
  See `_xtos_set_exception_handler` details below.
*/
struct __exception_frame
{
  uint32_t epc;
  uint32_t ps;
  uint32_t sar;
  uint32_t unused;
  union {
    struct {
      uint32_t a0;
      // note: no a1 here!
      uint32_t a2;
      uint32_t a3;
      uint32_t a4;
      uint32_t a5;
      uint32_t a6;
      uint32_t a7;
      uint32_t a8;
      uint32_t a9;
      uint32_t a10;
      uint32_t a11;
      uint32_t a12;
      uint32_t a13;
      uint32_t a14;
      uint32_t a15;
    };
    uint32_t a_reg[15];
  };
  uint32_t cause;
};
#endif

/*
 Most of the comments here are gleamed from the xtensa files found at the site
 listed below and are mostly unverified:
 https://github.com/qca/open-ath9k-htc-firmware/tree/master/sboot/magpie_1_1/sboot/athos/src/xtos
  * exc-c-wrapper-handler.S
  * exc-sethandler.c
*/

/*
  The Boot ROM sets up a table of dispatch handlers at 0x3FFFC000. This table
  has an entry for each of the EXCCAUSE values, 0 through 63. The exception
  handler at the `User Exception Vector` uses EXCCAUSE with the base address
  0x3FFFC000 to build a jump address to the respective cause handler. Of the
  cause handle functions, `_xtos_c_wrapper_handler` and `_xtos_unhandled_exception`
  are of interest.

  Exception handler entries that do not have a specific handler are set to
  `_xtos_unhandled_exception`. This handler will execute a `break 1, 1`
  (0x4000DC4Bu) before doing a `rfe` (return from exception).  Since the PC has
  not changed, the event that caused the 1st exception will likely keep
  repeating until the HWDT kicks in.

  These exception handling functions are in assembly, and do not conform to the
  typical "C" function conventions. However, some form of prototype/typedef is
  needed to reference these function addresses in "C" code. In
  `RTOS_SDK/components/esp8266/include/xtensa/xtruntime.h`, it uses a compounded
  definition that equates to `void (*)(...)` for .cpp modules to use. I have
  noticed this creates sufficient confusion at compilation to get your attention
  when used in the wrong place. I have copied that definition here.

  Added to eagle.rom.addr.v6.ld:
    PROVIDE ( _xtos_exc_handler_table = 0x3fffc000 );
    PROVIDE ( _xtos_c_handler_table = 0x3fffc100 );
*/
#ifndef XTRUNTIME_H
// This is copy/paste from RTOS_SDK/components/esp8266/include/xtensa/xtruntime.h
#ifdef __cplusplus
typedef void (_xtos_handler_func)(...);
#else
typedef void (_xtos_handler_func)();
#endif
typedef _xtos_handler_func *_xtos_handler;

extern _xtos_handler _xtos_exc_handler_table[XCHAL_EXCCAUSE_NUM];

/*
  Assembly-level handler, used in the _xtos_exc_handler_table[]. It is a wrapper
  for calling registered "C" exception handlers.
*/
_xtos_handler_func _xtos_c_wrapper_handler;

/*
  Assembly-level handler, used in the _xtos_exc_handler_table[]. It is the
  default handler, for exceptions without a registered handler.
*/
_xtos_handler_func _xtos_unhandled_exception;
#endif


#ifdef __cplusplus
// For these definitions, try to be more precise for .cpp module usage.

/*
  A detailed typdef for the "C" callable functions found in
  `_xtos_c_handler_table[]` More details in `_xtos_set_exception_handler`
  comments below.
*/
typedef void (*fn_c_exception_handler_t)(struct __exception_frame *ef, int cause);

/*
  TMI maybe? However, it may be useful for a deep debugging session.
  `_xtos_p_none` is the default "C" exception handler that fills the
  _xtos_c_handler_table[]. It is present when an exception handler has not been
  registered. It simply consist of a single instruction, `ret`.
  It is also internally used by `_xtos_set_exception_handler(cause, NULL)` to
  reset a "C" exception handler back to the unhandled state. The coresponding
  `_xtos_exc_handler_table` entry will be set to `_xtos_unhandled_exception`.
  Note, if nesting handlers is desired this must be implemented in the new "C"
  exception handler(s) being registered.
*/
extern void _xtos_p_none(struct __exception_frame *ef, int cause);

/*
  TMI maybe?
  For `extern _xtos_handler _xtos_c_handler_table[XCHAL_EXCCAUSE_NUM];`, defined
  in in `xtensa/xtos/exc-sethandler.c`. _xtos_handler is a generalized
  definition that doesn't match the actual function definition of those
  assigned to `_xtos_c_handler_table` entries.

  At this time we do not require direct access to this table. We perform updates
  by calling the ROM function `_xtos_set_exception_handler`.

  A corrected version for .cpp would look like this:
*/
extern fn_c_exception_handler_t _xtos_c_handler_table[XCHAL_EXCCAUSE_NUM];

/*
  ROM API function `_xtos_set_exception_handler` registers a "C" callable
  exception handler for a specified general exception, (EXCCAUSE value). (source
  in xtensa/xtos/exc-sethandler.c)
  * If `cause`/reason (EXCCAUSE) is out of range, >=64, it returns NULL.
  * If the new exception handler is installed, it returns the previous handler.
  * If the previous handler was `_xtos_unhandled_exception`/`_xtos_p_none`, it
    returns NULL.

  Note, the installed "C" exception handler is noramlly called from the ROM
  function _xtos_c_wrapper_handler with IRQs enabled. This build now includes a
  replacement wrapper that is used with the "C" exception handler for
  EXCCAUSE_LOAD_STORE_ERROR (3), Non 32-bit read/write error.

  This prototype has been corrected (changed from a generalized to specific
  argument list) for the .cpp files in this projects; however, it does not match
  the over generalized version in some Xtensa .h files (not currently part of
  this project)

  To aid against future conflicts, keep these new defines limited to .cpp with
  `#ifdef __cplusplus`.
*/
extern fn_c_exception_handler_t _xtos_set_exception_handler(int cause, fn_c_exception_handler_t fn);
#endif

#endif

#if defined(VERIFY_C_ASM_EXCEPTION_FRAME_STRUCTURE) || defined(_ASMLANGUAGE) || defined(__ASSEMBLER__)
/*
  Extracted from information at
  From https://github.com/fdivitto/ESPWebFramework/blob/master/SDK/xtensa-lx106-elf/xtensa-lx106-elf/lib/libhandlers-null.txt

  The UEXC_... values are create by the macro STRUCT_FIELD in `xtruntime-frames.h`

  These VERIFY_... values are used to confirm that the "C" structure offsets
  match those generated in exc-c-wrapper-handler.S.
*/
#define VERIFY_UEXC_pc             0x0000
#define VERIFY_UEXC_ps             0x0004
#define VERIFY_UEXC_sar            0x0008
#define VERIFY_UEXC_vpri           0x000c
#define VERIFY_UEXC_a0             0x0010
#define VERIFY_UEXC_a2             0x0014
#define VERIFY_UEXC_a3             0x0018
#define VERIFY_UEXC_a4             0x001c
#define VERIFY_UEXC_a5             0x0020
#define VERIFY_UEXC_a6             0x0024
#define VERIFY_UEXC_a7             0x0028
#define VERIFY_UEXC_a8             0x002c
#define VERIFY_UEXC_a9             0x0030
#define VERIFY_UEXC_a10            0x0034
#define VERIFY_UEXC_a11            0x0038
#define VERIFY_UEXC_a12            0x003c
#define VERIFY_UEXC_a13            0x0040
#define VERIFY_UEXC_a14            0x0044
#define VERIFY_UEXC_a15            0x0048
#define VERIFY_UEXC_exccause       0x004c
#define VERIFY_UserFrameSize       0x0050
#define VERIFY_UserFrameTotalSize  0x0100
#endif
