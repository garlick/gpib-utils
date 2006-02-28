/* This file is part of gpib-utils.
   For details, see http://sourceforge.net/projects/gpib-utils.

   Copyright (C) 2005 Jim Garlick <garlick@speakeasy.net>

   gpib-utils is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   gpib-utils is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with gpib-utils; if not, write to the Free Software Foundation, 
   Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA */

/* Convert Hewlett Packard instrument screen dump bitmap to Portable BitMap
 * (PBM) file format.
 */
#include <stdio.h>
#include <stdlib.h>

#define INVERSE_VIDEO

/* "safe" malloc */
char *xmalloc(int size)
{
	char *new = malloc(size);
	if (!new) {
		fprintf(stderr, "out of memory\n");
		exit(1);
	}
	return new;
}

/* "safe" realloc */
char *xrealloc(char *ptr, int size)
{
	char *new = realloc(ptr, size);
	if (!new) {
		fprintf(stderr, "out of memory\n");
		exit(1);
	}
	return new;
}

/* flip all the bits in a row */
void inverse(unsigned char *ptr, int nbytes)
{
	while (--nbytes >= 0)
		ptr[nbytes] = ~ptr[nbytes];
}

#define CHUNK_SIZE 64 	/* number of rows to allocate in one go */

int
main(int argc, char *argv[])
{
	unsigned char **row = NULL;
	int cur_size = 0;
	int cur_row = 0;
	int width = -1;
	int i, w;

	/* input: HP bitmap */

	/* Each row has a "header" ocnsisting of "<ESC>*b<N>W", where <N> is
	 * the number of bytes to follow.
	 */
	while (fscanf(stdin, "\033*b%dW", &w) == 1) {
		if (cur_row >= cur_size) {
			cur_size += CHUNK_SIZE;
			row = (unsigned char **)xrealloc((char *)row, 
					cur_size * sizeof(unsigned char *));
		}
		if (w <= 0) {
			fprintf(stderr, "hptopbm: bogus row width: %d\n", w);
			exit(1);
		}
		if (width == -1)
			width = w;
		if (width != w) {
			fprintf(stderr, "hptopbm: inconsistant width\n");
			exit(1);
		}

		row[cur_row] = xmalloc(w);
		if (fread(row[cur_row], w, 1, stdin) != 1) {
			fprintf(stderr, "hptopbm: short read\n");
			exit(1);
		}
		cur_row++;
	}
	if (cur_row == 0) {
		fprintf(stderr, "hptopbm: input is not an HP bitmap\n");
		exit(1);
	}
	fprintf(stderr, "hptopbm: bitmap geometry %dx%d\n", width*8, cur_row);


	/* output: PBM bitmap */

	/* Ref: pbm(5) */
	printf("P4 %d %d\n", width*8, cur_row);
	for (i = 0; i < cur_row; i++) {
#ifndef INVERSE_VIDEO
		inverse(row[i], width);
#endif
		if (fwrite(row[i], width, 1, stdout) != 1) {
			fprintf(stderr, "hptopbm: short write\n");
			exit(1);
		}
		free(row[i]);
	}
	free(row);

	exit(0);
}