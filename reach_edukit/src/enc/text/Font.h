/**
 * @file    Font.h
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

#ifndef ti_sdo_simplewidget_Font_h_
#define ti_sdo_simplewidget_Font_h_

#include <stdio.h>
//#include <xdc/std.h>
//#include <ti/sdo/simplewidget/SimpleWidget.h>

typedef struct Font_Object {
    void*        library;
     void*           face;
} Font_Object;

typedef struct Font_Object *Font_Handle;


#if defined (__cplusplus)
extern "C" {
#endif

extern Font_Handle Font_create(char *fileName);
extern int Font_delete(Font_Handle hFont);

#if defined (__cplusplus)
}
#endif

#endif /* ti_sdo_simplewidget_Font_h_ */
