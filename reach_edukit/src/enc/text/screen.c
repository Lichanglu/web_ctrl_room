/*
 * Screen.c
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
#include "dm6467_struct.h"

#include "screen.h"

//#include "log_common.h"
/******************************************************************************
 * Screen_fill
 ******************************************************************************/
Void Screen_fill(Buffer_Handle hBuf, Int x, Int y,
                 Int width, Int height, Int32 evenValue,
                 Int32 oddValue)
{
	BufferGfx_Dimensions dim;
	UInt8 *ptr, *rowPtr;
	Int32 bufSize = Buffer_getSize(hBuf);
	ColorSpace_Type colorSpace = BufferGfx_getColorSpace(hBuf);
	Int i, j;
	Int32 dstValue;
	//	printf("sceen2\n");
	BufferGfx_getDimensions(hBuf, &dim);
	//	printf("sceen3\n");
	ptr = (UInt8 *) Buffer_getUserPtr(hBuf) + y * dim.lineLength;
	//	printf("sceen4\n");
	ptr += Screen_getInc(colorSpace, x);
	//	printf("sceen5\n");

	for(j = y; j < height + y; j++) {
		rowPtr = ptr;

		for(i = x; i < width + x; i++) {
			dstValue = (i + x) & 1 ? oddValue : evenValue;
			Screen_setPixelPtr(rowPtr, bufSize, i + x, dstValue,
			                   FALSE, colorSpace);
			//  printf("sceen6\n");
			rowPtr += Screen_getStep(colorSpace, i + x);
			//	  printf("sceen7\n");
		}

		ptr += dim.lineLength;
		//	printf("sceen7 j= %d,height=%d\n", j, height);
	}

	//	printf("sceen8\n");
}

/******************************************************************************
 * Screen_clear
 ******************************************************************************/
Void Screen_clear(Buffer_Handle hBuf, Int x, Int y,
                  Int width, Int height)
{
	Int32 value = BufferGfx_getColorSpace(hBuf) == ColorSpace_YUV422PSEMI ?
	              0x80 : 0;
	//	printf("value =%x\n", value);
	Screen_fill(hBuf, x, y, width, height, value, value);
}


