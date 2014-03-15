// convert corona.raw in a pic_t as corona2.raw

#include <stdio.h>

typedef struct
{
	short width;
	char reserved0; // set to 0
	char mode; // see pic_mode_t above
	short height;
	short reserved1; // set to 0
} pic_t;

void main(int argc, char *argv[])
{
	int i, j, k;
	char buf1[256][256], buf2[256][256][2];
	pic_t pic = {256, 0, 2, 256, 0};
	FILE *g;

	FILE *f = fopen("corona.raw", "rb");
	fread(buf1, 256*256, 1, f);
	fclose(f);

	g = fopen("corona2.raw", "wb");
	for (i = 0; i < 256; i++)
	for (j = 0; j < 256; j++)
	{
		buf2[i][j][0] = buf1[i][j];
		buf2[i][j][1] = buf1[i][j];
	}
	fwrite(&pic, sizeof (pic_t), 1, g);
	fwrite(buf2, sizeof (buf2), 1, g);
	fclose(g);
}
