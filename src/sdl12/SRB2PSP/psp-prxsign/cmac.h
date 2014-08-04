/*
 ---------------------------------------------------------------------------
 Copyright (c) 1998-2008, Brian Gladman, Worcester, UK. All rights reserved.

 LICENSE TERMS

 The redistribution and use of this software (with or without changes)
 is allowed without the payment of fees or royalties provided that:

  1. source code distributions include the above copyright notice, this
     list of conditions and the following disclaimer;

  2. binary distributions include the above copyright notice, this list
     of conditions and the following disclaimer in their documentation;

  3. the name of the copyright holder is not used to endorse products
     built using this software without specific written permission.

 DISCLAIMER

 This software is provided 'as is' with no explicit or implied warranties
 in respect of its properties, including, but not limited to, correctness
 and/or fitness for purpose.
 ---------------------------------------------------------------------------
 Issue Date: 6/10/2008
*/

#ifndef CMAC_AES_H
#define CMAC_AES_H

#include <string.h>
#include <openssl/aes.h>

void xor_128(unsigned char *a, unsigned char *b, unsigned char *out);
void generate_subkey(unsigned char *key, unsigned char *K1, unsigned char *K2);
void AES_CMAC(unsigned char *key, unsigned char *input, int length, unsigned char *mac);

#endif
