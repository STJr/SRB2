/*
 *  CRC.H - header file for CRC functions
 */

#ifndef _CRC_H_
#define _CRC_H_

#include <stdlib.h>           /* For size_t                 */

typedef unsigned char BYTE;
typedef unsigned short WORD;
typedef unsigned long DWORD;

/*
**  File: CRC-16.C
*/

WORD crc16(char *data_p, WORD length);

#endif /* _CRC_H_ */
