/**
 * @file    Screen.h
 * @brief   
 *
 * Put the file comments here.
 *
 * @verbatim
 * ============================================================================
 * Copyright (c) Texas Instruments Inc 2009
 *
 * Use of this software is controlled by the terms and conditions found in the
 * license agreement under which this software has been supplied or provided.
 * ============================================================================
 * @endverbatim
 */

#ifndef ti_sdo_simplewidget_Screen_h_
#define ti_sdo_simplewidget_Screen_h_

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include "ti_media_std.h"

#define Uint16 UInt16
#define SCREEN_GET_2BPP_BYTE(x) ((x) >> 2)
#define SCREEN_GET_2BPP_SHIFT(x) (((x) % 4) * 2)

/******************************************************************************
 * Screen_getStep
 ******************************************************************************/
static inline Int Screen_getStep(ColorSpace_Type colorSpace, Int x)
{
    Int step;

    switch (colorSpace) {
        case ColorSpace_YUV422PSEMI:
            step = 1;
            break;
        case ColorSpace_RGB565:
            step = 2;
            break;
        case ColorSpace_2BIT:
            step = (!((x + 1) % 4)) ? 1 : 0;
            break;
        default:
            step = 0;
            break;
    }

    return step;
}

/******************************************************************************
 * Screen_getInc
 ******************************************************************************/
static inline Int Screen_getInc(ColorSpace_Type colorSpace, Int x)
{
    Int inc;

    switch (colorSpace) {
        case ColorSpace_YUV422PSEMI:
            inc = x;
            break;
        case ColorSpace_RGB565:
            inc = x << 1;
            break;
        case ColorSpace_2BIT:
            inc = SCREEN_GET_2BPP_BYTE(x);
            break;
        default:
            inc = 0;
            break;
    }

    return inc;
}
/******************************************************************************
 * Screen_getPixelPtr
 ******************************************************************************/
static inline void Screen_getPixelPtr(UInt8 *ptr, Int bufSize, Int x,UInt8* y,UInt8* cbcr, Bool overlay,
                                      ColorSpace_Type colorSpace)
{
	Int32 dstValue=0;	
	
    if (colorSpace == ColorSpace_YUV422PSEMI) {
        UInt8 *cbcrPtr = ptr + bufSize / 2;
        UInt8 v = 0;		
        if (overlay) {
            //v = (dstValue >> 8) & 0xff;

            if (v > *ptr) {
                *y=*ptr;
            }
        }
        else {
            *y=*ptr; //= (dstValue >> 8) & 0xff;
        }

        if (!overlay) {
            *cbcr=*cbcrPtr; //= dstValue & 0xff;
        }
    }
    else if (colorSpace == ColorSpace_RGB565) {
        UInt16 *bufPtr = (Uint16 *) ptr;
        if (overlay) {
            *bufPtr |= dstValue;
        }
        else {
            *bufPtr = dstValue;
        }
    }
    else if (colorSpace == ColorSpace_2BIT) {
        UInt8 byte;

        if (overlay) {
            *ptr |= dstValue << SCREEN_GET_2BPP_SHIFT(x);
        }
        else {
            byte = *ptr;
            byte &= ~(0x3 << SCREEN_GET_2BPP_SHIFT(x));
            byte |= dstValue << SCREEN_GET_2BPP_SHIFT(x);
            *ptr = byte;
        }
    }
	return;
}

/******************************************************************************
 * Screen_setPixelPtr
 ******************************************************************************/
static inline Void Screen_setPixelPtr(UInt8 *ptr, Int bufSize, Int x,
                                      Int32 dstValue, Bool overlay,
                                      ColorSpace_Type colorSpace)
{
    if (colorSpace == ColorSpace_YUV422PSEMI) {
        UInt8 *cbcrPtr = ptr + bufSize / 2;
        UInt8 v;

        if (overlay) {
            v = (dstValue >> 8) & 0xff;

            if (v > *ptr) {
                *ptr = v;
            }
        }
        else {
            *ptr = (dstValue >> 8) & 0xff;
        }

        if (!overlay) {
            *cbcrPtr = dstValue & 0xff;
        }
    }
    else if (colorSpace == ColorSpace_RGB565) {
        UInt16 *bufPtr = (Uint16 *) ptr;
        if (overlay) {
            *bufPtr |= dstValue;
        }
        else {
            *bufPtr = dstValue;
        }
    }
    else if (colorSpace == ColorSpace_2BIT) {
        UInt8 byte;

        if (overlay) {
            *ptr |= dstValue << SCREEN_GET_2BPP_SHIFT(x);
        }
        else {
            byte = *ptr;
            byte &= ~(0x3 << SCREEN_GET_2BPP_SHIFT(x));
            byte |= dstValue << SCREEN_GET_2BPP_SHIFT(x);
            *ptr = byte;
        }
    }
}

/******************************************************************************
 * Screen_setPixel
 ******************************************************************************/
static inline Void Screen_setPixel(Buffer_Handle hBuf,
                                   Int x, Int y, Int32 dstValue, Bool overlay)
{
    BufferGfx_Dimensions dim;

    BufferGfx_getDimensions(hBuf, &dim);

    if (BufferGfx_getColorSpace(hBuf) == ColorSpace_YUV422PSEMI) {
        UInt8 *yPtr = (UInt8 *) Buffer_getUserPtr(hBuf);
        UInt8 *cbcrPtr = yPtr + Buffer_getSize(hBuf) / 2;
        UInt8 v;

        if (overlay) {
            v = (dstValue >> 8) & 0xff;

            if (v > yPtr[y * dim.lineLength + x]) {
                yPtr[y * dim.lineLength + x] = v;
            }
        }
        else {
            yPtr[y * dim.lineLength + x] = (dstValue >> 8) & 0xff;
        }

        if (!overlay) {
            cbcrPtr[y * dim.lineLength + x] = dstValue & 0xff;
        }
    }
    else if (BufferGfx_getColorSpace(hBuf) == ColorSpace_RGB565) {
        UInt16 *bufPtr = (Uint16 *) Buffer_getUserPtr(hBuf);

        if (overlay) {
            bufPtr[y * (dim.lineLength >> 1) + x] |= dstValue;
        }
        else {
            bufPtr[y * (dim.lineLength >> 1) + x] = dstValue;
        }
    }
    else if (BufferGfx_getColorSpace(hBuf) == ColorSpace_2BIT) {
        UInt8 *bufPtr = (UInt8 *) Buffer_getUserPtr(hBuf);
        UInt8 byte;

        if (overlay) {
            bufPtr[y * dim.lineLength + SCREEN_GET_2BPP_BYTE(x)] |=
                dstValue << SCREEN_GET_2BPP_SHIFT(x);
        }
        else {
            byte = bufPtr[y * dim.lineLength + SCREEN_GET_2BPP_BYTE(x)];
            byte &= ~(0x3 << SCREEN_GET_2BPP_SHIFT(x));
            byte |= dstValue << SCREEN_GET_2BPP_SHIFT(x);

            bufPtr[y * dim.lineLength + SCREEN_GET_2BPP_BYTE(x)] = byte;
        }
    }
}

#if defined (__cplusplus)
extern "C" {
#endif

Void Screen_fill(Buffer_Handle hBuf, Int x, Int y,
                        Int width, Int height, Int32 evenValue,
                        Int32 oddValue);

Void Screen_clear(Buffer_Handle hBuf, Int x, Int y,
                         Int width, Int height);

#if defined (__cplusplus)
}
#endif

#endif /* ti_sdo_simplewidget_Screen_h_ */
