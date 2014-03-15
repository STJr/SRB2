/***********************************************************************
 *  serdbg.c
 *
 *  Description:  Pretty simple demonstration program.  It accomplishes
 *                the following.
 *
 *                1.  Allocate a block of memory Feel free to change
 *                    size (memBlockSize) with debugger.
 *
 *                2.  Writes a word pattern to the entire block.  Feel
 *                    free to change the pattern (memPatternWord) with
 *                    debugger.
 *
 *                3.  Computes the CRC-16 on the block.  Feel free to
 *                    check the size with the debuger.
 *
 *                4.  Free the memory block allocated in step 1.  Repeat
 *                    step 1.  If you wish to exit, set doneFlag to 0 with
 *                    the debugger.
 *
 *  Credits:      Created by Jonathan Brogdon
 *
 *  Terms of use:  Use as you will.
 *
 *  Global Data:  None.
 *  Global Functions:  main
 *
 *  History
 *  Engineer:           Date:              Notes:
 *  ---------           -----              ------
 *  Jonathan Brogdon    070500             Genesis
 *
 ***********************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <i386-stub.h>
#include <crc.h>

#define MEM_BLOCK_SIZE 100      /* Words */
#define MEM_PATTERN_WORD 0x55AA

void write_mem_pattern(unsigned short*, unsigned short, unsigned short);

/************************************************************************
 *
 *  main()
 *
 ************************************************************************/
int main(int argc, char *argv[])
{
  volatile int doneFlag = 0;
  unsigned short crcValue = 0;
  unsigned short * memBlockPtr = NULL;
  short memBlockSize = MEM_BLOCK_SIZE;
  short memPatternWord = MEM_PATTERN_WORD;

#ifdef REMOTE_DEBUGGING
  /* Only setup if demonstrating remote debugging */
  gdb_serial_init(DEBUG_COM_PORT,DEBUG_COM_PORT_SPEED);
  gdb_target_init();
  breakpoint();
#endif

  while(doneFlag != 1)
    {
      memBlockSize   = MEM_BLOCK_SIZE;
      memPatternWord = MEM_PATTERN_WORD;
      memBlockPtr    = (unsigned short *) malloc((int)memBlockSize);

      write_mem_pattern(memBlockPtr, memBlockSize, memPatternWord);

      crcValue = crc16((char *)memBlockPtr,memBlockSize);

      free(memBlockPtr);
    }

  exit(0);
}

/************************************************************************
 *
 *  write_mem_pattern()
 *
 *  Description:  Writes a word pattern to a block of RAM.
 *
 ************************************************************************/
void write_mem_pattern(unsigned short *block, unsigned short blockSize, unsigned short patternWord)
{
  int index = 0;

  for(index = 0; index < blockSize; index++)
    {
      block[index] = patternWord;
    }
}







