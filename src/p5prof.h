/*********************************************************
 *
 * File:  p5prof.h
 * By:    Kevin Baca
 *
 * MODIFIED BY Fab SO THAT RDMSR(...) WRITES EDX : EAX TO A LONG LONG
 * (WHICH MEANS WRITE THE LOW DWORD FIRST)
 *
 * Now in yer code do:
 *   INT64 count,total;
 *
 *   ...
 *   RDMSR(0x10,&count);        //inner loop count
 *   total += count;
 *   ...
 *
 *   printf("0x%x %x", (INT32)total, *((INT32 *)&total+1));
 *   //                  HIGH        LOW
 *
 *********************************************************/
/**\file
	\brief  This file provides macros to profile your code.

 Here's how they work...

 As you may or may not know, the Pentium class of
 processors provides extremely fine grained profiling
 capabilities through the use of what are called
 Machine Specific Registers (MSRs). These registers
 can provide information about almost any aspect of
 CPU performance down to a single cycle.

 The MSRs of interest for profiling are specified by
 indices 0x10, 0x11, 0x12, and 0x13.  Here is a brief
 description of each of these registers:

 MSR 0x10
    This register is simple a cycle counter.

 MSR 0x11
    This register controls what type of profiling data
 will be gathered.

 MSRs 0x12 and 0x13
    These registers gather the profiling data specified in
 MSR 0x11.

 Each MSR is 64 bits wide.  For the Pentium processor,
 only the lower 32 bits of MSR 0x11 are valid.  Bits 0-15
 specify what data will be gathered in MSR 0x12.  Bits 16-31
 specify what data will be gathered in MSR 0x13.  Both sets
 of bits have the same format:

 Bits 0-5 specify which hardware event will be tracked.
 Bit 6, if set, indicates events will be tracked in
 rings 0-2.
 Bit 7, if set, indicates events will be tracked in
 ring 3.
 Bit 8, if set, indicates cycles should be counted for
 the specified event.  If clear, it indicates the
 number of events should be counted.

 Two instructions are provided for manupulating the MSRs.
 RDMSR (Read Machine Specific Register) and WRMSR
 (Write Machine Specific Register).  These opcodes were
 originally undocumented and therefore most assemblers don't
 recognize them.  Their byte codes are provided in the
 macros below.

 RDMSR takes the MSR index in ecx and the profiling criteria
 in edx : eax.

 WRMSR takes the MSR index in ecx and returns the profile data
 in edx : eax.

 Two profiling registers limits profiling capability to
 gathering only two types of information.  The register
 usage can, however, be combined in interesting ways.
 For example, you can set one register to gather the
 number of a specific type of event while the other gathers
 the number of cycles for the same event.  Or you can
 gather the number of two separate events while using
 MSR 0x10 to gather the number of cycles.

 The enumerated list provides somewhat readable labels for
 the types of events that can be tracked.

 For more information, get ahold of appendix H from the
 Intel Pentium programmer's manual (I don't remember the
 order number) or go to
 http://green.kaist.ac.kr/jwhahn/art3.htm.
 That's an article by Terje Mathisen where I got most of
 my information.

 You may use this code however you wish.  I hope it's
 useful and I hope I got everything right.

 -Kevin

 kbaca@skygames.com

*/

#ifdef __GNUC__

#define RDTSC(_dst) \
__asm__("
     .byte 0x0F,0x31
     movl %%edx,(%%edi)
     movl %%eax,4(%%edi)"\
: : "D" (_dst) : "eax", "edx", "edi")

// the old code... swapped it
//     movl %%edx,(%%edi)
//     movl %%eax,4(%%edi)"
#define RDMSR(_msri, _msrd) \
__asm__("
     .byte 0x0F,0x32
     movl %%eax,(%%edi)
     movl %%edx,4(%%edi)"\
: : "c" (_msri), "D" (_msrd) : "eax", "ecx", "edx", "edi")

#define WRMSR(_msri, _msrd) \
__asm__("
     xorl %%edx,%%edx
     .byte 0x0F,0x30"\
: : "c" (_msri), "a" (_msrd) : "eax", "ecx", "edx")

#define RDMSR_0x12_0x13(_msr12, _msr13) \
__asm__("
     movl $0x12,%%ecx
     .byte 0x0F,0x32
     movl %%edx,(%%edi)
     movl %%eax,4(%%edi)
     movl $0x13,%%ecx
     .byte 0x0F,0x32
     movl %%edx,(%%esi)
     movl %%eax,4(%%esi)"\
: : "D" (_msr12), "S" (_msr13) : "eax", "ecx", "edx", "edi")

#define ZERO_MSR_0x12_0x13() \
__asm__("
     xorl %%edx,%%edx
     xorl %%eax,%%eax
     movl $0x12,%%ecx
     .byte 0x0F,0x30
     movl $0x13,%%ecx
     .byte 0x0F,0x30"\
: : : "eax", "ecx", "edx")

#elif defined (__WATCOMC__)

extern void RDTSC(UINT32 *dst);
#pragma aux RDTSC =\
   "db 0x0F,0x31"\
   "mov [edi],edx"\
   "mov [4+edi],eax"\
   parm [edi]\
   modify [eax edx edi];

extern void RDMSR(UINT32 msri, UINT32 *msrd);
#pragma aux RDMSR =\
   "db 0x0F,0x32"\
   "mov [edi],edx"\
   "mov [4+edi],eax"\
   parm [ecx] [edi]\
   modify [eax ecx edx edi];

extern void WRMSR(UINT32 msri, UINT32 msrd);
#pragma aux WRMSR =\
   "xor edx,edx"\
   "db 0x0F,0x30"\
   parm [ecx] [eax]\
   modify [eax ecx edx];

extern void RDMSR_0x12_0x13(UINT32 *msr12, UINT32 *msr13);
#pragma aux RDMSR_0x12_0x13 =\
   "mov ecx,0x12"\
   "db 0x0F,0x32"\
   "mov [edi],edx"\
   "mov [4+edi],eax"\
   "mov ecx,0x13"\
   "db 0x0F,0x32"\
   "mov [esi],edx"\
   "mov [4+esi],eax"\
   parm [edi] [esi]\
   modify [eax ecx edx edi esi];

extern void ZERO_MSR_0x12_0x13(void);
#pragma aux ZERO_MSR_0x12_0x13 =\
   "xor edx,edx"\
   "xor eax,eax"\
   "mov ecx,0x12"\
   "db 0x0F,0x30"\
   "mov ecx,0x13"\
   "db 0x0F,0x30"\
   modify [eax ecx edx];

#endif

typedef enum
{
   DataRead,
     DataWrite,
     DataTLBMiss,
     DataReadMiss,
     DataWriteMiss,
     WriteHitEM,
     DataCacheLinesWritten,
     DataCacheSnoops,
     DataCacheSnoopHit,
     MemAccessBothPipes,
     BankConflict,
     MisalignedDataRef,
     CodeRead,
     CodeTLBMiss,
     CodeCacheMiss,
     SegRegLoad,
     RESERVED0,
     RESERVED1,
     Branch,
     BTBHit,
     TakenBranchOrBTBHit,
     PipelineFlush,
     InstructionsExeced,
     InstructionsExecedVPipe,
     BusUtilizationClocks,
     PipelineStalledWriteBackup,
     PipelineStalledDateMemRead,
     PipeLineStalledWriteEM,
     LockedBusCycle,
     IOReadOrWriteCycle,
     NonCacheableMemRef,
     AGI,
     RESERVED2,
     RESERVED3,
     FPOperation,
     Breakpoint0Match,
     Breakpoint1Match,
     Breakpoint2Match,
     Breakpoint3Match,
     HWInterrupt,
     DataReadOrWrite,
     DataReadOrWriteMiss
};

#define PROF_CYCLES (0x100)
#define PROF_EVENTS (0x000)
#define RING_012    (0x40)
#define RING_3      (0x80)
#define RING_0123   (RING_012 | RING_3)

/*void ProfSetProfiles(UINT32 msr12, UINT32 msr13);*/
#define ProfSetProfiles(_msr12, _msr13)\
{\
   UINT32 prof;\
\
   prof = (_msr12) | ((_msr13) << 16);\
   WRMSR(0x11, prof);\
}

/*void ProfBeginProfiles(void);*/
#define ProfBeginProfiles()\
   ZERO_MSR_0x12_0x13();

/*void ProfGetProfiles(UINT32 msr12[2], UINT32 msr13[2]);*/
#define ProfGetProfiles(_msr12, _msr13)\
   RDMSR_0x12_0x13(_msr12, _msr13);

/*void ProfZeroTimer(void);*/
#define ProfZeroTimer()\
   WRMSR(0x10, 0);

/*void ProfReadTimer(UINT32 timer[2]);*/
#define ProfReadTimer(timer)\
   RDMSR(0x10, timer);

/*EOF*/
