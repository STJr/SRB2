/****************************************************************************

		THIS SOFTWARE IS NOT COPYRIGHTED

   HP offers the following for use in the public domain.  HP makes no
   warranty with regard to the software or it's performance and the
   user accepts the software "AS IS" with all faults.

   HP DISCLAIMS ANY WARRANTIES, EXPRESS OR IMPLIED, WITH REGARD
   TO THIS SOFTWARE INCLUDING BUT NOT LIMITED TO THE WARRANTIES
   OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.

****************************************************************************/

/****************************************************************************
 *  Header: remcom.c,v 1.34 91/03/09 12:29:49 glenne Exp $
 *
 *  Module name: remcom.c $
 *  Revision: 1.34 $
 *  Date: 91/03/09 12:29:49 $
 *  Contributor:     Lake Stevens Instrument Division$
 *
 *  Description:     low level support for gdb debugger. $
 *
 *  Considerations:  only works on target hardware $
 *
 *  Written by:      Glenn Engel $
 *  ModuleState:     Experimental $
 *
 *  NOTES:           See Below $
 *
 *  Modified for 386 by Jim Kingdon, Cygnus Support.
 *
 *  To enable debugger support, two things need to happen.  One, a
 *  call to set_debug_traps() is necessary in order to allow any breakpoints
 *  or error conditions to be properly intercepted and reported to gdb.
 *  Two, a breakpoint needs to be generated to begin communication.  This
 *  is most easily accomplished by a call to breakpoint().  Breakpoint()
 *  simulates a breakpoint by executing a trap #1.
 *
 *  The external function exceptionHandler() is
 *  used to attach a specific handler to a specific 386 vector number.
 *  It should use the same privilege level it runs at.  It should
 *  install it as an interrupt gate so that interrupts are masked
 *  while the handler runs.
 *
 *  Because gdb will sometimes write to the stack area to execute function
 *  calls, this program cannot rely on using the supervisor stack so it
 *  uses it's own stack area reserved in the int array remcomStack.
 *
 *************
 *
 *    The following gdb commands are supported:
 *
 * command          function                               Return value
 *
 *    g             return the value of the CPU registers  hex data or ENN
 *    G             set the value of the CPU registers     OK or ENN
 *
 *    mAA..AA,LLLL  Read LLLL bytes at address AA..AA      hex data or ENN
 *    MAA..AA,LLLL: Write LLLL bytes at address AA.AA      OK or ENN
 *
 *    c             Resume at current address              SNN   ( signal NN)
 *    cAA..AA       Continue at address AA..AA             SNN
 *
 *    s             Step one instruction                   SNN
 *    sAA..AA       Step one instruction from AA..AA       SNN
 *
 *    k             kill
 *
 *    ?             What was the last sigval ?             SNN   (signal NN)
 *
 * All commands and responses are sent with a packet which includes a
 * checksum.  A packet consists of
 *
 * $<packet info>#<checksum>.
 *
 * where
 * <packet info> :: <characters representing the command or response>
 * <checksum>    :: < two hex digits computed as modulo 256 sum of <packetinfo>>
 *
 * When a packet is received, it is first acknowledged with either '+' or '-'.
 * '+' indicates a successful transfer.  '-' indicates a failed transfer.
 *
 * Example:
 *
 * Host:                  Reply:
 * $m0,10#2a               +$00010203040506070809101112131415#42
 *
 ****************************************************************************/

#include <stdio.h>
#include <string.h>
#ifdef DJGPP
#include <dpmi.h>
#include <setjmp.h>
#include <signal.h>
#include <sys/exceptn.h>
#endif //#ifdef DJGPP


/*
 *
 * external low-level support routines
 */

extern void putDebugChar();	/* write a single character      */
extern int getDebugChar();	/* read and return a single char */
#ifndef DJGPP
extern void exceptionHandler();	/* assign an exception handler   */
#endif

/*
 * BUFMAX defines the maximum number of characters in inbound/outbound buffers
 * at least NUMREGBYTES*2 are needed for register packets
 */
#define BUFMAX 400

/*
 * boolean flag. != 0 means we've been initialized
 */
static char gdb_initialized;

/*
 *  debug >  0 prints ill-formed commands in valid packets & checksum errors
 */
int     remote_debug;

/*
 *  Hexadecimal character string.
 */
#ifndef DJGPP
static const char hexchars[]="0123456789abcdef";
#else
static char hexchars[]="0123456789abcdef";
#endif

/*
 * Number of registers.
*/
#define NUMREGS	16

/*
 * Number of bytes of registers.
*/
#define NUMREGBYTES (NUMREGS * 4)

/*
 * i386 Registers
 */
enum regnames {EAX, ECX, EDX, EBX, ESP, EBP, ESI, EDI,
	       PC /* also known as eip */,
	       PS /* also known as eflags */,
	CS, SS, DS, ES, FS, GS
};

/*
 * Register storage buffer.
 */
static int registers[NUMREGS];

/*
 * Address of a routine to RTE to if we get a memory fault.
*/
#ifndef DJGPP
static void (*volatile mem_fault_routine) () = NULL;
#else
static void (*mem_fault_routine)() = NULL;
#endif

/* I/O buffers */
static char remcomInBuffer[BUFMAX];
static char remcomOutBuffer[BUFMAX];

#if !(defined(DJGPP) || defined(_WIN32)) //MF
#define STACKSIZE 10000
int remcomStack[STACKSIZE/sizeof(int)];
static int* stackPtr = &remcomStack[STACKSIZE/sizeof(int) - 1];
#endif

#ifdef DJGPP
static void lock_handler_data(void);
#endif //#ifdef DJGPP

void _returnFromException ();

/***************************  ASSEMBLY CODE MACROS *************************/
/* 									   */

extern void
return_to_prog ();

/* Restore the program's registers (including the stack pointer, which
   means we get the right stack and don't have to worry about popping our
   return address and any stack frames and so on) and return.  */
asm(".text");
asm(".globl _return_to_prog");
asm("_return_to_prog:");
asm("        movw _registers+44, %ss");
asm("        movl _registers+16, %esp");
asm("        movl _registers+4, %ecx");
asm("        movl _registers+8, %edx");
asm("        movl _registers+12, %ebx");
asm("        movl _registers+20, %ebp");
asm("        movl _registers+24, %esi");
asm("        movl _registers+28, %edi");
asm("        movw _registers+48, %ds");
asm("        movw _registers+52, %es");
asm("        movw _registers+56, %fs");
asm("        movw _registers+60, %gs");
asm("        movl _registers+36, %eax");
asm("        pushl %eax");  /* saved eflags */
asm("        movl _registers+40, %eax");
asm("        pushl %eax");  /* saved cs */
asm("        movl _registers+32, %eax");
asm("        pushl %eax");  /* saved eip */
asm("        movl _registers, %eax");
/* use iret to restore pc and flags together so
   that trace flag works right.  */
asm("        iret");

/*
 * BREAKPOINT macro
 */
#ifdef _MSC_VER //MF
#define BREAKPOINT() __asm int 3;
#else
#define BREAKPOINT() asm("   int $3");
#endif

/*
 * Store the error code here just in case the user cares.
*/
int gdb_i386errcode;

/*
 * Store the vector number here (since GDB only gets the signal
 * number through the usual means, and that's not very specific).
 */
#ifndef _WIN32 //MF
int gdb_i386vector = -1;
#endif	// !_WIN32

#if defined(DJGPP) || defined(_WIN32)
static void handle_exception(int);
#endif

#ifdef DJGPP

/***********************************************************************
 *  save_regs
 *
 *  Description:  Retreives the i386 registers as they were when the
 *                exception occurred.  Registers are stored in the 
 *                local static buffer.
 *
 *  Inputs:   None.
 *  Outputs:  None.
 *  Returns:  None.
 *
 ***********************************************************************/
static void save_regs(void)
{
	registers[EAX] = (int) __djgpp_exception_state->__eax;
	registers[ECX] = (int) __djgpp_exception_state->__ecx;
	registers[EDX] = (int) __djgpp_exception_state->__edx;
	registers[EBX] = (int) __djgpp_exception_state->__ebx;
	registers[ESP] = (int) __djgpp_exception_state->__esp;
	registers[EBP] = (int) __djgpp_exception_state->__ebp;
	registers[ESI] = (int) __djgpp_exception_state->__esi;
	registers[EDI] = (int) __djgpp_exception_state->__edi;
	registers[PC]  = (int) __djgpp_exception_state->__eip;
	registers[PS]  = (int) __djgpp_exception_state->__eflags;
	registers[CS]  = (int) __djgpp_exception_state->__cs;
	registers[SS]  = (int) __djgpp_exception_state->__ss;
	registers[DS]  = (int) __djgpp_exception_state->__ds;
	registers[ES]  = (int) __djgpp_exception_state->__es;
	registers[FS]  = (int) __djgpp_exception_state->__fs;
	registers[GS]  = (int) __djgpp_exception_state->__gs;
}
static void end_save_regs(void){}

/***********************************************************************
 *  set_regs
 *
 *  Description:  Restores i386 registers to the DJGPP register buffer.
 *                DJGPP exception handler will restore registers from
 *                it's buffer on exit from the handler.
 *
 *  Inputs:   None.
 *  Outputs:  None.
 *  Returns:  None.
 *
 ***********************************************************************/
static void set_regs(void)
{
	__djgpp_exception_state->__eax    = (unsigned long) registers[EAX];
	__djgpp_exception_state->__ecx    = (unsigned long) registers[ECX]; 
	__djgpp_exception_state->__edx    = (unsigned long) registers[EDX]; 
	__djgpp_exception_state->__ebx    = (unsigned long) registers[EBX]; 
	__djgpp_exception_state->__esp    = (unsigned long) registers[ESP]; 
	__djgpp_exception_state->__ebp    = (unsigned long) registers[EBP]; 
	__djgpp_exception_state->__esi    = (unsigned long) registers[ESI]; 
	__djgpp_exception_state->__edi    = (unsigned long) registers[EDI]; 
	__djgpp_exception_state->__eip    = (unsigned long) registers[PC];  
	__djgpp_exception_state->__eflags = (unsigned long) registers[PS];  
	__djgpp_exception_state->__cs     = (unsigned long) registers[CS];  
	__djgpp_exception_state->__ss	    = (unsigned long) registers[SS];  
	__djgpp_exception_state->__ds	    = (unsigned long) registers[DS];  
	__djgpp_exception_state->__es	    = (unsigned long) registers[ES];  
	__djgpp_exception_state->__fs	    = (unsigned long) registers[FS];  
	__djgpp_exception_state->__gs	    = (unsigned long) registers[GS];  
}									 
static void end_set_regs(void){}

/***********************************************************************
 *  sigsegv_handler
 *
 *  Description:  Handles SIGSEGV signal
 *
 *  Inputs:   None.
 *  Outputs:  None.
 *  Returns:  None.
 *
 ***********************************************************************/
static void sigsegv_handler(int except_num){

	/* Save general purpose registers */
	save_regs();

	/* Dispatch memory fault handling routine if one is registered. */
	if(mem_fault_routine != 0)
	{
		(*mem_fault_routine)();
		mem_fault_routine = NULL;
	}
	else
	{
		/* Save error code */
		gdb_i386errcode = __djgpp_exception_state->__sigmask & 0xffff;

		/* Call the general exception handler */
		handle_exception(except_num);
	}
	/* Write back registers */
	set_regs();

	/* Return from handler */
	longjmp(__djgpp_exception_state,__djgpp_exception_state->__eax);
}
static void end_sigsegv_handler(void){}

/***********************************************************************
 *  sigfpe_handler
 *
 *  Description:  Handles SIGFPE signal
 *
 *  Inputs:   None.
 *  Outputs:  None.
 *  Returns:  None.
 *
 ***********************************************************************/
static void sigfpe_handler(int except_num){

	/* Save general purpose registers */
	save_regs();

	/* Call the general purpose exception handler */
	handle_exception(except_num);

	/* Write back registers */
	set_regs();

	/* Return from handler */
	longjmp(__djgpp_exception_state,__djgpp_exception_state->__eax);
}
static void end_sigfpe_handler(void){}

/***********************************************************************
 *  sigtrap_handler
 *
 *  Description:  Handles SIGTRAP signal
 *
 *  Inputs:   None.
 *  Outputs:  None.
 *  Returns:  None.
 *
 ***********************************************************************/
static void sigtrap_handler(int except_num) {

	/* Save general purpose registers */
	save_regs();

	/* Call the general purpose exception handler */
	handle_exception(except_num);

	/* Write back registers */
	set_regs();

	/* Return from handler */
	longjmp(__djgpp_exception_state,__djgpp_exception_state->__eax);
}
static void end_sigtrap_handler(int except_num){}

/***********************************************************************
 *  sigill_handler
 *
 *  Description:  Handles SIGILL signal
 *
 *  Inputs:   None.
 *  Outputs:  None.
 *  Returns:  None.
 *
 ***********************************************************************/
static void sigill_handler(int except_num) {

	/* Save general purpose registers */
	save_regs();

	/* Call the general purpose exception handler */
	handle_exception(except_num);

	/* Write back registers */
	set_regs();

	/* Return from handler */
	longjmp(__djgpp_exception_state,__djgpp_exception_state->__eax);
}
static void end_sigill_handler(int except_num){}
#endif

#ifdef _WIN32 //MF

void win32_exception_handler(EXCEPTION_POINTERS* exc_info)
{
  PCONTEXT ctx = exc_info->ContextRecord;

  registers[EAX] = ctx->Eax;
  registers[ECX] = ctx->Ecx;
  registers[EDX] = ctx->Edx;
  registers[EBX] = ctx->Ebx;
  registers[ESP] = ctx->Esp;
  registers[EBP] = ctx->Ebp;
  registers[ESI] = ctx->Esi;
  registers[EDI] = ctx->Edi;
   registers[PC] = ctx->Eip;
   registers[PS] = ctx->EFlags;
  registers[CS] = ctx->SegCs;
  registers[SS] = ctx->SegSs;
  registers[DS] = ctx->SegDs;
  registers[ES] = ctx->SegEs;
  registers[FS] = ctx->SegFs;
  registers[GS] = ctx->SegGs;

  handle_exception((int)(exc_info->ExceptionRecord->ExceptionCode & 0xFFFF));

  ctx->Eax = registers[EAX];
  ctx->Ecx = registers[ECX];
  ctx->Edx = registers[EDX];
  ctx->Ebx = registers[EBX];
  ctx->Esp = registers[ESP];
  ctx->Ebp = registers[EBP];
  ctx->Esi = registers[ESI];
  ctx->Edi = registers[EDI];
   ctx->Eip = registers[PC];
   ctx->EFlags = registers[PS];
  ctx->SegCs = registers[CS];
  ctx->SegSs = registers[SS];
  ctx->SegDs = registers[DS];
  ctx->SegEs = registers[ES];
  ctx->SegFs = registers[FS];
  ctx->SegGs = registers[GS];
}

#endif // _WIN32

/***********************************************************************
 *  hex
 *
 *  Description:  Convert ASCII character values, representing hex
 *                digits, to the integer value.
 *
 *  Inputs:
 *    ch      - data character
 *  Outputs:  None.
 *  Returns:  integer value represented by the input character.
 *
 ***********************************************************************/
static int hex (char ch)
{
  if ((ch >= 'a') && (ch <= 'f'))
    return (ch - 'a' + 10);
  if ((ch >= '0') && (ch <= '9'))
    return (ch - '0');
  if ((ch >= 'A') && (ch <= 'F'))
    return (ch - 'A' + 10);
  return (-1);
}
#ifdef DJGPP
static void end_hex(void) {}
#endif

/***********************************************************************
 *  getpacket
 *
 *  Description:  Retrieve GDB data packet.
 *                Scan for the sequence $<data>#<checksum>
 *  Inputs:   None.
 *  Outputs:  None.
 *  Returns:  Beginning of packet buffer.
 *
 ***********************************************************************/
static unsigned char *getpacket (void)
{
  unsigned char *buffer = &remcomInBuffer[0];
  unsigned char checksum;
  unsigned char xmitcsum;
  int count;
  char ch;

  while (1)
    {
      /* wait around for the start character, ignore all other characters */
      while ((ch = getDebugChar ()) != '$')
	;

    retry:
      checksum = 0;
      xmitcsum = -1;
      count = 0;

      /* now, read until a # or end of buffer is found */
      while (count < BUFMAX)
	{
	  ch = getDebugChar ();
	  if (ch == '$')
	    goto retry;
	  if (ch == '#')
	    break;
	  checksum = checksum + ch;
	  buffer[count] = ch;
	  count = count + 1;
	}
      buffer[count] = 0;

      if (ch == '#')
	{
	  ch = getDebugChar ();
	  xmitcsum = hex (ch) << 4;
	  ch = getDebugChar ();
	  xmitcsum += hex (ch);

	  if (checksum != xmitcsum)
	    {
	      if (remote_debug)
		{
		  fprintf (stderr,
			   "bad checksum.  My count = 0x%x, sent=0x%x. buf=%s\n",
			   checksum, xmitcsum, buffer);
		}
	      putDebugChar ('-');	/* failed checksum */
	    }
	  else
	    {
	      putDebugChar ('+');	/* successful transfer */

	      /* if a sequence char is present, reply the sequence ID */
	      if (buffer[2] == ':')
		{
		  putDebugChar (buffer[0]);
		  putDebugChar (buffer[1]);

		  return &buffer[3];
		}

	      return &buffer[0];
	    }
	}
    }
}
#ifdef DJGPP
static void end_getpacket(void) {}
#endif

/***********************************************************************
 *  putpacket
 *
 *  Description:  Send GDB data packet.
 *
 *  Inputs:   Buffer of data to send.
 *  Outputs:  None.
 *  Returns:  None.
 *
 ***********************************************************************/
static void putpacket (unsigned char *buffer)
{
  unsigned char checksum;
  int count;
  char ch;

  /*  $<packet info>#<checksum>. */
  do
    {
      putDebugChar ('$');
      checksum = 0;
      count = 0;

      while ((ch = buffer[count]))
	{
	  putDebugChar (ch);
	  checksum += ch;
	  count += 1;
	}

      putDebugChar ('#');
      putDebugChar (hexchars[checksum >> 4]);
      putDebugChar (hexchars[checksum % 16]);

    }
  while (getDebugChar () != '+');
}
#ifdef DJGPP
static void end_putpacket(void) {}
#endif


/***********************************************************************
 *  debug_error
 *
 *  Description:  Log errors
 *
 *  Inputs:
 *     format   - Format string.
 *     parm     - parameter string
 *  Outputs:  None.
 *  Returns:  None.
 *
 ***********************************************************************/
static void debug_error (const char *format, char *parm)
{
  if (remote_debug)
    fprintf (stderr, format, parm);
}
#ifdef DJGPP
static void end_debug_error(void) {}
#endif

/*
 * Indicate to caller of mem2hex or hex2mem that there has been an error.
 */
#ifndef DJGPP
static volatile int mem_err = 0;
#else
static int mem_err = 0;
#endif

/***********************************************************************
 *  set_mem_err
 *
 *  Description:  set memory error flag
 *
 *  Inputs:   None.
 *  Outputs:  None.
 *  Returns:  None.
 *
 ***********************************************************************/
static void set_mem_err (void)
{
  mem_err = 1;
}
#ifdef DJGPP
static void end_set_mem_err(void) {}
#endif

/***********************************************************************
 *  get_char
 *
 *  Description:  Retreive a character from the specified address.
 *                These are separate functions so that they are so
 *                short and sweet that the compiler won't save any
 *                registers (if there is a fault to mem_fault, they
 *                won't get restored, so there better not be any saved).
 *
 *  Inputs:   addr  -  The address to read from.
 *  Outputs:  None.
 *  Returns:  data read from address.
 *
 ***********************************************************************/
static int get_char (char *addr)
{
  return *addr;
}
#ifdef DJGPP
static void end_get_char(void) {}
#endif

/***********************************************************************
 *  set_char
 *
 *  Description:  Write a value to the specified address.
 *                These are separate functions so that they are so
 *                short and sweet that the compiler won't save any
 *                registers (if there is a fault to mem_fault, they
 *                won't get restored, so there better not be any saved).
 *
 *  Inputs:
 *     addr  -  The address to read from.
 *     val   -  value to write.
 *  Outputs:  None.
 *  Returns:  None.
 *
 ***********************************************************************/
static void set_char (char *addr, int val)
{
  *addr = val;
}
#ifdef DJGPP
static void end_set_char(void) {}
#endif


/***********************************************************************
 *  mem2hex
 *
 *  Description:  Convert the memory pointed to by mem into hex, placing
 *                result in buf.  Return a pointer to the last char put
 *                in buf (null).  If MAY_FAULT is non-zero, then we should
 *                set mem_err in response to a fault; if zero treat a
 *                fault like any other fault in the stub.
 *
 *  Inputs:
 *     mem        -  Memory address
 *     buf        -  data buffer
 *     count      -  number of bytes
 *     may_fault  -  flag indicating that the operation may cause a mem fault.
 *  Outputs:  None.
 *  Returns:  Pointer to last character.
 *
 ***********************************************************************/
static char *mem2hex (char *mem,char *buf,int count,int  may_fault)
{
  int i;
  unsigned char ch;

#ifdef _WIN32 //MF
  if (IsBadReadPtr(mem, (DWORD)count))
	return mem;
#else
  if (may_fault)
    mem_fault_routine = set_mem_err;
#endif
  for (i = 0; i < count; i++)
    {
      ch = get_char (mem++);
      if (may_fault && mem_err)
	return (buf);
      *buf++ = hexchars[ch >> 4];
      *buf++ = hexchars[ch % 16];
    }
  *buf = 0;
#ifndef _WIN32 //MF
  if (may_fault)
    mem_fault_routine = NULL;
#endif
  return (buf);
}
#ifdef DJGPP
static void end_mem2hex(void) {}
#endif

/***********************************************************************
 *  hex2mem
 *
 *  Description:  Convert the hex array pointed to by buf into binary to
 *                be placed in mem. Return a pointer to the character
 *                AFTER the last byte written
 *  Inputs:
 *     buf
 *     mem
 *     count
 *     may_fault
 *  Outputs:  None.
 *  Returns:  Pointer to buffer after last byte.
 *
 ***********************************************************************/
static char *hex2mem (char *buf,char *mem,int count,int may_fault)
{
  int i;
  unsigned char ch;

#ifdef _WIN32 //MF
   // MinGW does not support structured exception handling, so let's
   // go safe and make memory writable by default
  DWORD old_protect;

  VirtualProtect(mem, (DWORD)count, PAGE_EXECUTE_READWRITE, &old_protect);
#else
  if (may_fault)
    mem_fault_routine = set_mem_err;
#endif
  for (i = 0; i < count; i++)
    {
      ch = hex (*buf++) << 4;
      ch = ch + hex (*buf++);
      set_char (mem++, ch);
      if (may_fault && mem_err)
	return (mem);
    }
#ifndef _WIN32 //MF
  if (may_fault)
    mem_fault_routine = NULL;
#endif
  return (mem);
}
#ifdef DJGPP
static void end_hex2mem(void) {}
#endif

/***********************************************************************
 *  computeSignal
 *
 *  Description:  This function takes the 386 exception vector and
 *                attempts to translate this number into a unix 
 *                compatible signal value.
 *  Inputs:
 *     exceptionVector
 *  Outputs:  None.
 *  Returns:  
 *
 ***********************************************************************/
static int computeSignal (int exceptionVector)
{
  int sigval;
  switch (exceptionVector)
    {
    case 0:
      sigval = 8;
      break;			/* divide by zero */
    case 1:
      sigval = 5;
      break;			/* debug exception */
    case 3:
      sigval = 5;
      break;			/* breakpoint */
    case 4:
      sigval = 16;
      break;			/* into instruction (overflow) */
    case 5:
      sigval = 16;
      break;			/* bound instruction */
    case 6:
      sigval = 4;
      break;			/* Invalid opcode */
    case 7:
      sigval = 8;
      break;			/* coprocessor not available */
    case 8:
      sigval = 7;
      break;			/* double fault */
    case 9:
      sigval = 11;
      break;			/* coprocessor segment overrun */
    case 10:
      sigval = 11;
      break;			/* Invalid TSS */
    case 11:
      sigval = 11;
      break;			/* Segment not present */
    case 12:
      sigval = 11;
      break;			/* stack exception */
    case 13:
      sigval = 11;
      break;			/* general protection */
    case 14:
      sigval = 11;
      break;			/* page fault */
    case 16:
      sigval = 7;
      break;			/* coprocessor error */
    default:
      sigval = 7;		/* "software generated" */
    }
  return (sigval);
}
#ifdef DJGPP
static void end_computeSignal(void) {}
#endif

/***********************************************************************
 *  hexToInt
 *
 *  Description:  Convert an ASCII string to an integer.
 *
 *  Inputs:
 *  Outputs:  None.
 *  Returns: Number of chars processed
 *
 ***********************************************************************/
int hexToInt (char **ptr, int *intValue)
{
  int numChars = 0;
  int hexValue;

  *intValue = 0;

  while (**ptr)
    {
      hexValue = hex (**ptr);
      if (hexValue >= 0)
	{
	  *intValue = (*intValue << 4) | hexValue;
	  numChars++;
	}
      else
	break;

      (*ptr)++;
    }

  return (numChars);
}
#ifdef DJGPP
static void end_hexToInt(void) {}
#endif


/***********************************************************************
 *  handle_exception
 *
 *  Description:  This function does all command procesing for interfacing
 *                to GDB.
 *  Inputs:
 *    exceptionVector  - number of the vector.
 *  Outputs:  None.
 *  Returns:  None.
 *
 ***********************************************************************/
static void handle_exception (int exceptionVector)
{
  int sigval, stepping;
  int addr, length;
  char *ptr;
  int newPC;

#ifndef _WIN32 //MF
  gdb_i386vector = exceptionVector;
#endif

  if (remote_debug)
    {
      printf ("vector=%d, sr=0x%x, pc=0x%x\n",
	      exceptionVector, registers[PS], registers[PC]);
    }

  /* reply to host that an exception has occurred */
  sigval = computeSignal (exceptionVector);

  ptr = remcomOutBuffer;

  *ptr++ = 'T';			/* notify gdb with signo, PC, FP and SP */
  *ptr++ = hexchars[sigval >> 4];
  *ptr++ = hexchars[sigval & 0xf];

  *ptr++ = hexchars[ESP]; 
  *ptr++ = ':';
  ptr = mem2hex((char *)&registers[ESP], ptr, 4, 0);	/* SP */
  *ptr++ = ';';

  *ptr++ = hexchars[EBP]; 
  *ptr++ = ':';
  ptr = mem2hex((char *)&registers[EBP], ptr, 4, 0); 	/* FP */
  *ptr++ = ';';

  *ptr++ = hexchars[PC]; 
  *ptr++ = ':';
  ptr = mem2hex((char *)&registers[PC], ptr, 4, 0); 	/* PC */
  *ptr++ = ';';

  *ptr = '\0';

  putpacket (remcomOutBuffer);

  stepping = 0;

  while (1 == 1)
    {
      remcomOutBuffer[0] = 0;
      ptr = getpacket ();

      switch (*ptr++)
	{
	case '?':
	  remcomOutBuffer[0] = 'S';
	  remcomOutBuffer[1] = hexchars[sigval >> 4];
	  remcomOutBuffer[2] = hexchars[sigval % 16];
	  remcomOutBuffer[3] = 0;
	  break;
	case 'd':
	  remote_debug = !(remote_debug);	/* toggle debug flag */
	  break;
	case 'g':		/* return the value of the CPU registers */
	  mem2hex ((char *) registers, remcomOutBuffer, NUMREGBYTES, 0);
	  break;
	case 'G':		/* set the value of the CPU registers - return OK */
	  hex2mem (ptr, (char *) registers, NUMREGBYTES, 0);
	  strcpy (remcomOutBuffer, "OK");
	  break;
	case 'P':		/* set the value of a single CPU register - return OK */
	  {
	    int regno;

	    if (hexToInt (&ptr, &regno) && *ptr++ == '=')
	      if (regno >= 0 && regno < NUMREGS)
		{
		  hex2mem (ptr, (char *) &registers[regno], 4, 0);
		  strcpy (remcomOutBuffer, "OK");
		  break;
		}

	    strcpy (remcomOutBuffer, "E01");
	    break;
	  }

	  /* mAA..AA,LLLL  Read LLLL bytes at address AA..AA */
	case 'm':
	  /* TRY TO READ %x,%x.  IF SUCCEED, SET PTR = 0 */
	  if (hexToInt (&ptr, &addr))
	    if (*(ptr++) == ',')
	      if (hexToInt (&ptr, &length))
		{
		  ptr = 0;
		  mem_err = 0;
		  mem2hex ((char *) addr, remcomOutBuffer, length, 1);
		  if (mem_err)
		    {
		      strcpy (remcomOutBuffer, "E03");
		      debug_error ("%s","memory fault");
		    }
		}

	  if (ptr)
	    {
	      strcpy (remcomOutBuffer, "E01");
	    }
	  break;

	  /* MAA..AA,LLLL: Write LLLL bytes at address AA.AA return OK */
	case 'M':
	  /* TRY TO READ '%x,%x:'.  IF SUCCEED, SET PTR = 0 */
	  if (hexToInt (&ptr, &addr))
	    if (*(ptr++) == ',')
	      if (hexToInt (&ptr, &length))
		if (*(ptr++) == ':')
		  {
		    mem_err = 0;
		    hex2mem (ptr, (char *) addr, length, 1);

		    if (mem_err)
		      {
			strcpy (remcomOutBuffer, "E03");
			debug_error ("%s","memory fault");
		      }
		    else
		      {
			strcpy (remcomOutBuffer, "OK");
		      }

		    ptr = 0;
		  }
	  if (ptr)
	    {
	      strcpy (remcomOutBuffer, "E02");
	    }
	  break;

	  /* cAA..AA    Continue at address AA..AA(optional) */
	  /* sAA..AA   Step one instruction from AA..AA(optional) */
	case 's':
	  stepping = 1;
	case 'c':
	  /* try to read optional parameter, pc unchanged if no parm */
	  if (hexToInt (&ptr, &addr))
	    registers[PC] = addr;

	  newPC = registers[PC];

	  /* clear the trace bit */
	  registers[PS] &= 0xfffffeff;

	  /* set the trace bit if we're stepping */
	  if (stepping)
	    registers[PS] |= 0x100;

#ifdef _WIN32 //MF
		  return;
#else
	  _returnFromException ();	/* this is a jump */

	  break;

	  /* kill the program */
	case 'k':		/* do nothing */
#if 0
	  /* Huh? This doesn't look like "nothing".
	     m68k-stub.c and sparc-stub.c don't have it.  */
	  BREAKPOINT ();
#endif
	  break;
#endif
	}			/* switch */

      /* reply to the request */
      putpacket (remcomOutBuffer);
    }
}
#ifdef DJGPP
static void end_handle_exception(void) {}
#endif

#ifndef DJGPP
/* GDB stores segment registers in 32-bit words (that's just the way
   m-i386v.h is written).  So zero the appropriate areas in registers.  */
#define SAVE_REGISTERS1() \
  asm ("movl %eax, _registers");                                   	  \
  asm ("movl %ecx, _registers+4");			  		     \
  asm ("movl %edx, _registers+8");			  		     \
  asm ("movl %ebx, _registers+12");			  		     \
  asm ("movl %ebp, _registers+20");			  		     \
  asm ("movl %esi, _registers+24");			  		     \
  asm ("movl %edi, _registers+28");			  		     \
  asm ("movw $0, %ax");							     \
  asm ("movw %ds, _registers+48");			  		     \
  asm ("movw %ax, _registers+50");					     \
  asm ("movw %es, _registers+52");			  		     \
  asm ("movw %ax, _registers+54");					     \
  asm ("movw %fs, _registers+56");			  		     \
  asm ("movw %ax, _registers+58");					     \
  asm ("movw %gs, _registers+60");			  		     \
  asm ("movw %ax, _registers+62");
#define SAVE_ERRCODE() \
  asm ("popl %ebx");                                  \
  asm ("movl %ebx, _gdb_i386errcode");
#define SAVE_REGISTERS2() \
  asm ("popl %ebx"); /* old eip */			  		     \
  asm ("movl %ebx, _registers+32");			  		     \
  asm ("popl %ebx");	 /* old cs */			  		     \
  asm ("movl %ebx, _registers+40");			  		     \
  asm ("movw %ax, _registers+42");                                           \
  asm ("popl %ebx");	 /* old eflags */		  		     \
  asm ("movl %ebx, _registers+36");			 		     \
  /* Now that we've done the pops, we can save the stack pointer.");  */   \
  asm ("movw %ss, _registers+44");					     \
  asm ("movw %ax, _registers+46");     	       	       	       	       	     \
  asm ("movl %esp, _registers+16");

/* See if mem_fault_routine is set, if so just IRET to that address.  */
#define CHECK_FAULT() \
  asm ("cmpl $0, _mem_fault_routine");					   \
  asm ("jne mem_fault");

asm (".text");
asm ("mem_fault:");
/* OK to clobber temp registers; we're just going to end up in set_mem_err.  */
/* Pop error code from the stack and save it.  */
asm ("     popl %eax");
asm ("     movl %eax, _gdb_i386errcode");

asm ("     popl %eax"); /* eip */
/* We don't want to return there, we want to return to the function
   pointed to by mem_fault_routine instead.  */
asm ("     movl _mem_fault_routine, %eax");
asm ("     popl %ecx"); /* cs (low 16 bits; junk in hi 16 bits).  */
asm ("     popl %edx"); /* eflags */

/* Remove this stack frame; when we do the iret, we will be going to
   the start of a function, so we want the stack to look just like it
   would after a "call" instruction.  */
asm ("     leave");

/* Push the stuff that iret wants.  */
asm ("     pushl %edx"); /* eflags */
asm ("     pushl %ecx"); /* cs */
asm ("     pushl %eax"); /* eip */

/* Zero mem_fault_routine.  */
asm ("     movl $0, %eax");
asm ("     movl %eax, _mem_fault_routine");

asm ("iret");


#define CALL_HOOK() asm("call _remcomHandler");

/* This function is called when a i386 exception occurs.  It saves
 * all the cpu regs in the _registers array, munges the stack a bit,
 * and invokes an exception handler (remcom_handler).
 *
 * stack on entry:                       stack on exit:
 *   old eflags                          vector number
 *   old cs (zero-filled to 32 bits)
 *   old eip
 *
 */
extern void _catchException3();
asm(".text");
asm(".globl __catchException3");
asm("__catchException3:");
SAVE_REGISTERS1();
SAVE_REGISTERS2();
asm ("pushl $3");
CALL_HOOK();

/* Same thing for exception 1.  */
extern void _catchException1();
asm(".text");
asm(".globl __catchException1");
asm("__catchException1:");
SAVE_REGISTERS1();
SAVE_REGISTERS2();
asm ("pushl $1");
CALL_HOOK();

/* Same thing for exception 0.  */
extern void _catchException0();
asm(".text");
asm(".globl __catchException0");
asm("__catchException0:");
SAVE_REGISTERS1();
SAVE_REGISTERS2();
asm ("pushl $0");
CALL_HOOK();

/* Same thing for exception 4.  */
extern void _catchException4();
asm(".text");
asm(".globl __catchException4");
asm("__catchException4:");
SAVE_REGISTERS1();
SAVE_REGISTERS2();
asm ("pushl $4");
CALL_HOOK();

/* Same thing for exception 5.  */
extern void _catchException5();
asm(".text");
asm(".globl __catchException5");
asm("__catchException5:");
SAVE_REGISTERS1();
SAVE_REGISTERS2();
asm ("pushl $5");
CALL_HOOK();

/* Same thing for exception 6.  */
extern void _catchException6();
asm(".text");
asm(".globl __catchException6");
asm("__catchException6:");
SAVE_REGISTERS1();
SAVE_REGISTERS2();
asm ("pushl $6");
CALL_HOOK();

/* Same thing for exception 7.  */
extern void _catchException7();
asm(".text");
asm(".globl __catchException7");
asm("__catchException7:");
SAVE_REGISTERS1();
SAVE_REGISTERS2();
asm ("pushl $7");
CALL_HOOK();

/* Same thing for exception 8.  */
extern void _catchException8();
asm(".text");
asm(".globl __catchException8");
asm("__catchException8:");
SAVE_REGISTERS1();
SAVE_ERRCODE();
SAVE_REGISTERS2();
asm ("pushl $8");
CALL_HOOK();

/* Same thing for exception 9.  */
extern void _catchException9();
asm(".text");
asm(".globl __catchException9");
asm("__catchException9:");
SAVE_REGISTERS1();
SAVE_REGISTERS2();
asm ("pushl $9");
CALL_HOOK();

/* Same thing for exception 10.  */
extern void _catchException10();
asm(".text");
asm(".globl __catchException10");
asm("__catchException10:");
SAVE_REGISTERS1();
SAVE_ERRCODE();
SAVE_REGISTERS2();
asm ("pushl $10");
CALL_HOOK();

/* Same thing for exception 12.  */
extern void _catchException12();
asm(".text");
asm(".globl __catchException12");
asm("__catchException12:");
SAVE_REGISTERS1();
SAVE_ERRCODE();
SAVE_REGISTERS2();
asm ("pushl $12");
CALL_HOOK();

/* Same thing for exception 16.  */
extern void _catchException16();
asm(".text");
asm(".globl __catchException16");
asm("__catchException16:");
SAVE_REGISTERS1();
SAVE_REGISTERS2();
asm ("pushl $16");
CALL_HOOK();

/* For 13, 11, and 14 we have to deal with the CHECK_FAULT stuff.  */

/* Same thing for exception 13.  */
extern void _catchException13 ();
asm (".text");
asm (".globl __catchException13");
asm ("__catchException13:");
CHECK_FAULT();
SAVE_REGISTERS1();
SAVE_ERRCODE();
SAVE_REGISTERS2();
asm ("pushl $13");
CALL_HOOK();

/* Same thing for exception 11.  */
extern void _catchException11 ();
asm (".text");
asm (".globl __catchException11");
asm ("__catchException11:");
CHECK_FAULT();
SAVE_REGISTERS1();
SAVE_ERRCODE();
SAVE_REGISTERS2();
asm ("pushl $11");
CALL_HOOK();

/* Same thing for exception 14.  */
extern void _catchException14 ();
asm (".text");
asm (".globl __catchException14");
asm ("__catchException14:");
CHECK_FAULT();
SAVE_REGISTERS1();
SAVE_ERRCODE();
SAVE_REGISTERS2();
asm ("pushl $14");
CALL_HOOK();

/*
 * remcomHandler is a front end for handle_exception.  It moves the
 * stack pointer into an area reserved for debugger use.
 */
asm("_remcomHandler:");
asm("           popl %eax");        /* pop off return address     */
asm("           popl %eax");      /* get the exception number   */
asm("		movl _stackPtr, %esp"); /* move to remcom stack area  */
asm("		pushl %eax");	/* push exception onto stack  */
asm("		call  _handle_exception");    /* this never returns */
#endif

void _returnFromException ()
{
#ifndef DJGPP
	return_to_prog ();
#else
/* Return from handler */
	longjmp(__djgpp_exception_state,__djgpp_exception_state->__eax);
#endif
}

/* this function is used to set up exception handlers for tracing and
   breakpoints */
#ifndef DJGPP
void
set_debug_traps (void)
{
#ifndef _WIN32 //MF
  stackPtr = &remcomStack[STACKSIZE / sizeof (int) - 1];

  exceptionHandler (0, _catchException0);
  exceptionHandler (1, _catchException1);
  exceptionHandler (3, _catchException3);
  exceptionHandler (4, _catchException4);
  exceptionHandler (5, _catchException5);
  exceptionHandler (6, _catchException6);
  exceptionHandler (7, _catchException7);
  exceptionHandler (8, _catchException8);
  exceptionHandler (9, _catchException9);
  exceptionHandler (10, _catchException10);
  exceptionHandler (11, _catchException11);
  exceptionHandler (12, _catchException12);
  exceptionHandler (13, _catchException13);
  exceptionHandler (14, _catchException14);
  exceptionHandler (16, _catchException16);
#endif // _WIN32

  gdb_initialized = 1;
}
#endif //#ifndef DJGPP


// DJGPP stuff
#ifdef DJGPP
/***********************************************************************
 *  restore_traps
 *
 *  Description:  This function restores all used signal handlers to
 *                defaults.
 *
 *  Inputs:   None.
 *  Outputs:  None.
 *  Returns:  None.
 *
 ***********************************************************************/
void restore_traps(void)
{
	/* Restore default signal handlers */
	signal(SIGSEGV,SIG_DFL);
	signal(SIGTRAP,SIG_DFL);
	signal(SIGFPE,SIG_DFL);
	signal(SIGTRAP,SIG_DFL);
	signal(SIGILL,SIG_DFL);

	/* Clear init flag */
	gdb_initialized = 0;
}

/***********************************************************************
 *  lock_handler_data
 *
 *  Description:  This function locks all data that is used by the signal
 *                handlers.
 *
 *  Inputs:   None.
 *  Outputs:  None.
 *  Returns:  None.
 *
 ***********************************************************************/
static void lock_handler_data(void)
{
	_go32_dpmi_lock_data(&gdb_initialized,sizeof(gdb_initialized));
	_go32_dpmi_lock_data(&remote_debug,sizeof(remote_debug));
	_go32_dpmi_lock_data(hexchars,sizeof(hexchars));
	_go32_dpmi_lock_data(registers,sizeof(registers));
	_go32_dpmi_lock_data(&gdb_i386errcode,sizeof(gdb_i386errcode));
	_go32_dpmi_lock_data(&gdb_i386vector,sizeof(gdb_i386vector));

	_go32_dpmi_lock_data(remcomInBuffer,sizeof(remcomInBuffer));
	_go32_dpmi_lock_data(remcomOutBuffer,sizeof(remcomOutBuffer));

	_go32_dpmi_lock_code(getpacket,(unsigned long)end_getpacket-
		(unsigned long)getpacket);
	_go32_dpmi_lock_code(putpacket,(unsigned long)end_putpacket-
		(unsigned long)putpacket);
	_go32_dpmi_lock_code(debug_error,(unsigned long)end_debug_error-
		(unsigned long)debug_error);

	_go32_dpmi_lock_data(&mem_fault_routine,sizeof(mem_fault_routine));
	_go32_dpmi_lock_data(&mem_err,sizeof(mem_err));

	_go32_dpmi_lock_code(set_mem_err,(unsigned long)end_set_mem_err-(unsigned long)set_mem_err);
	_go32_dpmi_lock_code(get_char,(unsigned long)end_get_char-(unsigned long)get_char);
	_go32_dpmi_lock_code(set_char,(unsigned long)end_set_char-(unsigned long)set_char);
	_go32_dpmi_lock_code(mem2hex,(unsigned long)end_hex-(unsigned long)hex);
	_go32_dpmi_lock_code(mem2hex,(unsigned long)end_mem2hex-(unsigned long)mem2hex);
	_go32_dpmi_lock_code(hex2mem,(unsigned long)end_hex2mem-(unsigned long)hex2mem);
	_go32_dpmi_lock_code(computeSignal,(unsigned long)end_computeSignal-
		(unsigned long)computeSignal);
	_go32_dpmi_lock_code(hexToInt,(unsigned long)end_hexToInt-(unsigned long)hexToInt);
	_go32_dpmi_lock_code(handle_exception,(unsigned long)end_handle_exception-
		(unsigned long)handle_exception);

	_go32_dpmi_lock_code(sigsegv_handler,
		(unsigned long)end_sigsegv_handler-(unsigned long)sigsegv_handler);

	_go32_dpmi_lock_code(sigfpe_handler,
		(unsigned long)end_sigfpe_handler-(unsigned long)sigfpe_handler);

	_go32_dpmi_lock_code(sigtrap_handler,
		(unsigned long)end_sigtrap_handler-(unsigned long)sigtrap_handler);

	_go32_dpmi_lock_code(sigill_handler,
		(unsigned long)end_sigill_handler-(unsigned long)sigill_handler);

	_go32_dpmi_lock_code(save_regs,
		(unsigned long)end_save_regs-(unsigned long)save_regs);

	_go32_dpmi_lock_code(set_regs,
		(unsigned long)end_set_regs-(unsigned long)set_regs);

}

/***********************************************************************
 *  set_debug_traps
 *
 *  Description:  This function installs signal handlers.
 *
 *  Inputs:   None.
 *  Outputs:  None.
 *  Returns:  None.
 *
 ***********************************************************************/
void set_debug_traps(void)
{
	/* Lock any data that may be used by the trap handlers */
	lock_handler_data();

	/* Install signal handlers here */
	signal(SIGSEGV,sigsegv_handler);
	signal(SIGFPE,sigfpe_handler);
	signal(SIGTRAP,sigtrap_handler);
	signal(SIGILL,sigill_handler);

	/* Set init flag */
	gdb_initialized = 1;

}
#endif //#ifdef DJGPP

/***********************************************************************
 *  breakpoint
 *
 *  Description:  This function will generate a breakpoint exception.
 *                It is used at the beginning of a program to sync up
 *                with a debugger and can be used otherwise as a quick
 *                means to stop program execution and "break" into the
 *                debugger.
 *
 *  Inputs:   None.
 *  Outputs:  None.
 *  Returns:  None.
 *
 ***********************************************************************/
void breakpoint ()
{
  if (gdb_initialized)
    BREAKPOINT ();
}

