#include <stdio.h>
#include <stdlib.h>
#include <openssl/aes.h>
#include <openssl/sha.h>
#include <elf.h>
#include "cmac.h"
#include "kirk_header.h"
#include "psp_header.h"

typedef unsigned char byte;

typedef struct {
	byte key[16];
	byte ckey[16];
	byte head_hash[16];
	byte data_hash[16];
	byte unused[32];
	int unk1; // 1
	int unk2; // 0
	int unk3[2];
	int datasize;
	int dataoffset;
	int unk4[6];
} kirk1head_t;

// secret kirk command 1 key
byte kirk_key[] = {
	0x98, 0xc9, 0x40, 0x97, 0x5c, 0x1d, 0x10, 0xe8, 0x7f, 0xe6, 0x0e, 0xa3, 0xfd, 0x03, 0xa8, 0xba
};

int main(int argc, char **argv)
{
	int i, relrem, size, fullsize, blocks, datasize;
	size_t j;
	kirk1head_t kirk;
	byte iv[16];
	byte cmac[32];
	byte subk[32];
	byte psph[0x150];
	byte *datablob;
	byte *filebuff;
	FILE *f;
	AES_KEY aesKey;
	Elf32_Ehdr *ehdr;
	Elf32_Shdr *shdr;
	Elf32_Rel *relo;

	if(argc < 3) {
		printf("Usage: %s unsigned.prx signed.prx\n", argv[0]);
		return 1;
	}

	// clean kirk header, use modified PRXdecrypter to get it
	/*f = fopen(argv[1], "rb");
	if(!f) {
		printf("failed to open %s\n", argv[1]);
		return 1;
	}
	fread(&kirk, 1, sizeof(kirk1head_t), f);
	fclose(f);*/
	//memcpy(&kirk, kirk_header, size_kirk_header);
	memcpy(&kirk, kirk_header, sizeof(kirk1head_t));

	datasize = kirk.datasize;
	if(datasize % 16) datasize += 16 - (datasize % 16);

	// original ~PSP header
	/*f = fopen(argv[2], "rb");
	if(!f) {
		free(datablob);
		printf("failed to open %s\n", argv[2]);
		return 1;
	}
	fread(psph, 1, 0x150, f);
	fclose(f);*/
	memcpy(&psph, psp_header, size_psp_header);

	// file to encrypt
	f = fopen(argv[1], "rb");
	if(!f) {
		printf("psp-prxsign: Unable to open PRX\n");
		return 1;
	}
	fseek(f, 0, SEEK_END);
	size = ftell(f);
	if(size > datasize - 16) {
		fclose(f);
		printf("psp-prxsign: PRX is too large\n");
		return 1;
	}
	printf("%s : %i\n", argv[1], size);
	fseek(f, 0, SEEK_SET);

	fullsize = datasize + 0x30 + kirk.dataoffset;

	// datablob holds everything needed to calculate data HASH
	datablob = malloc(fullsize);
	if(!datablob) {
		fclose(f);
		printf("psp-prxsign: Failed to allocate memory for blob\n");
		return 1;
	}
	memset(datablob, 0, fullsize);
	memcpy(datablob, &kirk.unk1, 0x30);
	memcpy(datablob + 0x30, psph, kirk.dataoffset);
	filebuff = datablob + 0x30 + kirk.dataoffset;

	int whocares = fread(filebuff, 1, size, f);
	(void)whocares;
	fclose(f);

	// remove relocations type 7
	relrem = 0;
	ehdr = (void *)filebuff;
	if(!memcmp(ehdr->e_ident, ELFMAG, 4) && ehdr->e_shnum) {
		shdr = (void *)(filebuff + ehdr->e_shoff);
		for(i = 0; i < ehdr->e_shnum; i++) {
			if(shdr[i].sh_type == 0x700000A0) {
				relo = (void *)(filebuff + shdr[i].sh_offset);
				for(j = 0; j < shdr[i].sh_size / sizeof(Elf32_Rel); j++) {
					if((relo[j].r_info & 0xFF) == 7) {
						relo[j].r_info = 0;
						relrem++;
					}
				}
			}
		}
	}
	//printf("%i relocations type 7 removed\ncalculating ...\n", relrem);

	// get AES/CMAC key
	AES_set_decrypt_key(kirk_key, 128, &aesKey);
	memset(iv, 0, 16);
	AES_cbc_encrypt(kirk.key, kirk.key, 32, &aesKey, iv, AES_DECRYPT);

	// check header hash, optional
	// if you take correct kirk header, hash is always correct
/*	AES_CMAC(kirk.ckey, datablob, 0x30, cmac);
	if(memcmp(cmac, kirk.head_hash, 16)) {
		free(datablob);
		printf("header hash invalid\n");
		return 1;
	}
*/

	// encrypt input file
	AES_set_encrypt_key(kirk.key, 128, &aesKey);
	memset(iv, 0, 16);
	AES_cbc_encrypt(filebuff, filebuff, datasize, &aesKey, iv, AES_ENCRYPT);

	// make CMAC correct
	generate_subkey(kirk.ckey, subk, subk + 16);
	AES_set_encrypt_key(kirk.ckey, 128, &aesKey);
	blocks = fullsize / 16;
	memset(cmac, 0, 16);
	for(i = 0; i < blocks - 1; i++) {
		xor_128(cmac, &datablob[16 * i], cmac + 16);
		AES_encrypt(cmac + 16, cmac, &aesKey);
	}

	AES_set_decrypt_key(kirk.ckey, 128, &aesKey);
	AES_decrypt(kirk.data_hash, iv, &aesKey);
	xor_128(cmac, iv, iv);
	xor_128(iv, subk, &datablob[16 * (blocks-1)]);
	// check it, optional
	// it works, this is only if you want to change something
/*	AES_CMAC(kirk.ckey, datablob, fullsize, cmac);
	if(memcmp(cmac, kirk.data_hash, 16)) {
		fclose(f);
		free(datablob);
		printf("data hash calculation error\n");
		return 1;
	}
*/
	f = fopen(argv[2], "wb");
	if(!f) {
		free(datablob);
		printf("psp-prxsign: Failed to write signed PRX\n");
		return 1;
	}
	//printf("saving ...\n");
	// save ~PSP header
	fwrite(psph, 1, 0x150, f);
	// save encrypted file
	fwrite(filebuff, 1, fullsize - 0x30 - kirk.dataoffset, f);
	fclose(f);
	free(datablob);
	//printf("everything done\n");
	return 0;
}
