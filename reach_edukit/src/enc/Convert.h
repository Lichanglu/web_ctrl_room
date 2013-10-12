/**
 * @file    Convert.h
 * @brief   
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

#ifndef ti_sdo_simplewidget_Convert_h_
#define ti_sdo_simplewidget_Convert_h_

//#include <std.h>


typedef enum {
    /** @brief From RGB888 to 422Psemi */
    Convert_Mode_RGB888_YUV422SEMI,

    /** @brief From RGB888 to RGB565 */
    Convert_Mode_RGB888_RGB565,

    /** @brief From RGB888 to 2bit */
    Convert_Mode_RGB888_2BIT,

    Convert_Mode_COUNT
} Convert_Mode;

#if defined (__cplusplus)
extern "C" {
#endif

extern Void Convert_init(Void);

extern Int32 Convert_execute(Convert_Mode mode, Int32 srcVal, Bool odd);

#if defined (__cplusplus)
}
#endif

#endif /* ti_sdo_simplewidget_Convert_h_ */
