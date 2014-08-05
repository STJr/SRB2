/*
 * Wad compressor/decompressor by Graue <graue@oceanbase.org>
 * Written January 23, 2007
 *
 * This file is in the public domain; use it without any restrictions.
 */

#include <stdio.h>
#include <stdlib.h>
#ifndef _MSC_VER
#include <stdint.h>
#else
typedef unsigned int uint32_t;
typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
#endif
#include <errno.h>
#include <string.h>
#include "err.h"
#include "lzf.h"
#include "xm.h"

/* Note: Define WORDS_BIGENDIAN if that is the case on the target CPU. */

#define MINCOMPRESS 1024 // Don't bother compressing lumps smaller than this.

#define SWAP16(n) ((((n)&0xff)<<8) | ((n)>>8))
#define SWAP32(n)                                                       \
(                                                                       \
        ((((n)&0xff) <<24)                                              \
        | ((((n)>>8)&0xff)<<16)                                         \
        | ((((n)>>16)&0xff)<<8)                                         \
        | ((n)>>24)                                                     \
)
#ifdef WORDS_BIGENDIAN
#define conv16le SWAP16
#define conv32le SWAP32
#else
#define conv16le(n) (n)
#define conv32le(n) (n)
#endif
#define READFUNC(name, type, conv)					\
static type name(FILE *fp)						\
{									\
	type val;							\
	if (fread(&val, sizeof val, 1, fp) < 1)				\
	{								\
		if (ferror(fp))						\
			err(1, "error reading input file");		\
		errx(1, "premature end of input file");			\
	}								\
	val = conv(val);						\
	return val;							\
}
READFUNC(read32le, uint32_t, conv32le)

#define WRITEFUNC(name, type, conv)					\
static void name(FILE *fp, type val)					\
{									\
	val = conv(val);						\
	if (fwrite(&val, sizeof (type), 1, fp) < 1)			\
		err(1, "error writing output file");			\
}
WRITEFUNC(write32le, uint32_t, conv32le)

#define ID_IWAD 0x44415749
#define ID_PWAD 0x44415750
#define ID_ZWAD 0x4441575a // 'ZWAD' for lzf-compressed wad

typedef struct
{
	char name[8];
	uint32_t len;
	void *data;
} lump_t;

typedef struct
{
	uint32_t id;
	uint32_t numlumps;
	lump_t *lumps;
} wad_t;

extern char *__progname;

static void usage(void)
{
	fprintf(stderr, "usage: %s [cd] infile outfile\n", __progname);
	exit(EXIT_FAILURE);
}

#define fileoffset_t long
#define myseek fseek
#define mytell ftell

static void readlumpdata(lump_t *lump, uint32_t csize, int compressed, FILE *fp)
{
	uint8_t *cbuf;
	uint8_t *ubuf;
	uint32_t usize;
	unsigned int retval;

	cbuf = xm(1, csize);
	if (fread(cbuf, csize, 1, fp) < 1)
		err(1, "cannot read lump from input file");

	if (!compressed)
	{
		lump->len = csize;
		lump->data = cbuf;
		return;
	}

	usize = conv32le(*(uint32_t *)cbuf);
	if (usize == 0)
	{
		lump->len = csize - 4;
		lump->data = cbuf + 4; // XXX cannot be freed later
		return;
	}

	ubuf = xm(1, usize);
	retval = lzf_decompress(cbuf + 4, csize - 4, ubuf, usize);
	if (retval == 0)
	{
		if (errno == E2BIG)
			errx(1, "decompressed data bigger than advertised");
		if (errno == EINVAL)
			errx(1, "invalid compressed (lzf) data");
		else
			err(1, "unknown error from lzf");
	}
	else if (retval < usize)
		errx(1, "decompressed data smaller than advertised");
	lump->len = usize;
	lump->data = ubuf;
	free(cbuf);
	return;
}

static wad_t *readwad(const char *fname)
{
	wad_t *wad;
	FILE *fp;
	int inputcompressed;
	fileoffset_t dirstart;
	uint32_t ix;

	fp = fopen(fname, "rb");
	if (fp == NULL) err(1, "%s", fname);

	wad = xm(sizeof *wad, 1);

	// Read wad id. If it is ZWAD, prepare to read compressed lumps.
	wad->id = read32le(fp);
	inputcompressed = wad->id == ID_ZWAD;

	// Read number of lumps, and prepare space for that many.
	wad->numlumps = read32le(fp);
	wad->lumps = xm(sizeof wad->lumps[0], wad->numlumps);

	// Read offset to directory.
	dirstart = (fileoffset_t)read32le(fp);

	for (ix = 0; ix < wad->numlumps; ix++)
	{
		fileoffset_t lumpofs;
		uint32_t lumpsize; // The compressed size, if it is compressed.

		if (myseek(fp, dirstart + (16*ix), SEEK_SET) == -1)
		{
			err(1, "cannot seek input file to dir entry %lu",
				(unsigned long)ix);
		}
		lumpofs = (fileoffset_t)read32le(fp);
		lumpsize = read32le(fp);
		if (fread(wad->lumps[ix].name, 1, 8, fp) < 8)
			err(1, "cannot read lump %lu name", (unsigned long)ix);

		// Don't try to seek to zero-length lumps.
		// Their offset is meaningless.
		if (lumpsize == 0)
		{
			wad->lumps[ix].len = 0;
			continue;
		}

		if (myseek(fp, lumpofs, SEEK_SET) == -1)
			err(1, "cannot seek to lump %lu", (unsigned long)ix);
		readlumpdata(&wad->lumps[ix], lumpsize, inputcompressed, fp);
	}

	if (fclose(fp) == EOF)
		warn("error closing input file %s", fname);
	return wad;
}

void writewad(const wad_t *wad, const char *fname, int compress)
{
	uint32_t *lumpofs; // Each lump's offset in the file.
	uint32_t *disksize; // Each lump's disk size (compressed if applicable).
	uint32_t dirstart;
	uint32_t ix;
	FILE *fp;

	lumpofs = xm(sizeof lumpofs[0], wad->numlumps);
	disksize = xm(sizeof disksize[0], wad->numlumps);

	fp = fopen(fname, "wb");
	if (fp == NULL) err(1, "%s", fname);

	// Write header.
	write32le(fp, compress ? ID_ZWAD : ID_PWAD);
	write32le(fp, wad->numlumps);
	write32le(fp, 0); // Directory table comes later.

	for (ix = 0; ix < wad->numlumps; ix++)
	{
		uint8_t *cbuf;
		uint32_t csize;

		lumpofs[ix] = (uint32_t)mytell(fp);
		if (!compress)
		{
			cbuf = wad->lumps[ix].data;
			csize = wad->lumps[ix].len;
		}
		else
		{
			unsigned int retval;

			csize = wad->lumps[ix].len + 4;
			cbuf = xm(1, csize);
			if (wad->lumps[ix].len < MINCOMPRESS
				|| (retval = lzf_compress(wad->lumps[ix].data,
					wad->lumps[ix].len, cbuf + 4,
					csize - 5)) == 0)
			{
				// Store uncompressed.
				memcpy(cbuf + 4, wad->lumps[ix].data,
					wad->lumps[ix].len);
				*(uint32_t *)cbuf = 0;
			}
			else
			{
				// Compression succeeded.
				// Store uncompressed size, and set compressed
				// size properly.
				*(uint32_t *)cbuf =
					conv32le(wad->lumps[ix].len);
				csize = retval + 4;
			}
		}

		if (!csize)
			; // inu: 0 length markers aren't to be written
		else if (fwrite(cbuf, csize, 1, fp) < 1)
		{
			err(1, "cannot write lump %lu to %s", (unsigned long)ix,
				fname);
		}

		if (compress)
			free(cbuf);
		disksize[ix] = csize;
	}

	dirstart = (uint32_t)mytell(fp);
	if (myseek(fp, 8L, SEEK_SET) == -1)
		err(1, "cannot reseek %s to write directory start", fname);
	write32le(fp, dirstart);
	if (myseek(fp, dirstart, SEEK_SET) == -1)
		err(1, "cannot reseek %s to write actual directory", fname);

	for (ix = 0; ix < wad->numlumps; ix++)
	{
		// Write the lump's directory entry.
		write32le(fp, lumpofs[ix]);
		write32le(fp, disksize[ix]);
		if (fwrite(wad->lumps[ix].name, 1, 8, fp) < 8)
		{
			err(1, "cannot write lump %lu's name",
				(unsigned long)ix);
		}
	}

	if (fclose(fp) == EOF)
		warn("error closing output file %s", fname);
}

int main(int argc, char *argv[])
{
	const char *infile, *outfile;
	wad_t *wad;
	int compress;

	if (argc != 4 || strlen(argv[1]) != 1)
		usage();

	/* c to compress, d to decompress */
	if (argv[1][0] == 'c') compress = 1;
	else if (argv[1][0] == 'd') compress = 0;
	else usage();

	infile = argv[2];
	outfile = argv[3];

	wad = readwad(infile);
	writewad(wad, outfile, compress);
	return 0;
}
