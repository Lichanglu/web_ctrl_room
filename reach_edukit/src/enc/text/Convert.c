/*
 * Convert.c
 *
 * ============================================================================
 * Copyright (c) Texas Instruments Inc 2009
 *
 * Use of this software is controlled by the terms and conditions found in the
 * license agreement under which this software has been supplied or provided.
 * ============================================================================
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>


#include "ti_media_std.h"
#include "Convert.h"
//#include "log_common.h"

static UInt8 _8to5bit[256] = { 0 };
static UInt8 _8to6bit[256] = { 0 };
static UInt8 _8to2bit[256] = { 0 };

Void Convert_init(Void)
{
	Int i;

	if(_8to5bit[128] == 0) {
		/* Initialize the conversion arrays */
		for(i = 0; i < 256; i++) {
			_8to5bit[i] = i * (1 << 5) / (1 << 8);
		}

		for(i = 0; i < 256; i++) {
			_8to6bit[i] = i * (1 << 6) / (1 << 8);
		}

		for(i = 0; i < 256; i++) {
			_8to2bit[i] = i * (1 << 2) / (1 << 8);
		}
	}
}

Int32 Convert_execute(Convert_Mode mode, Int32 srcVal, Bool odd)
{
	UInt32 r, g, b;
	UInt32 y, cb, cr;
	Int32 val;

	b = srcVal & 0xff;
	g = (srcVal >> 8) & 0xff;
	r = (srcVal >> 16) & 0xff;

	switch(mode) {
		case Convert_Mode_RGB888_RGB565:
			val = _8to5bit[ r ] << 11 |
			      _8to6bit[ g ] << 5 |
			      _8to5bit[ b ];
			break;

		case Convert_Mode_RGB888_2BIT:
			val = _8to2bit[ srcVal & 0xff ];
			break;

		case Convert_Mode_RGB888_YUV422SEMI:
			if(odd) {
				y  = ((66 * r + 129 * g + 25 * b + 128) >> 8) +  16;
				cb = ((112 * r -  94 * g - 18 * b + 128) >> 8) + 128;
				val = (y & 0xff) << 8 | (cb & 0xff);
			} else {
				y  = ((66  * r + 129 * g +  25 * b + 128) >> 8) +  16;
				cr = ((-38 * r -  74 * g + 112 * b + 128) >> 8) + 128;
				val = (y & 0xff) << 8 | (cr & 0xff);
			}

			break;

		default:
			printf("Conversion not implemented yet\n");
			val = -1;
			break;
	}

	return val;
}
