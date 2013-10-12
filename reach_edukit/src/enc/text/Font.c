/*
 * Font.c
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
#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_GLYPH_H

//#include <xdc/std.h>
//#include <ti/sdo/dmai/BufferGfx.h>

#include "dm6467_struct.h"
#include "Font.h"

Font_Handle Font_create(char *fileName)
{
	Font_Handle hFont;

	hFont = calloc(1, sizeof(Font_Object));

	if(hFont == NULL) {
		printf("Failed to allocate space for Font Object\n");
		return NULL;
	}

	if(FT_Init_FreeType((FT_Library *)(void *) &hFont->library)) {
		printf("Failed to intialize freetype library\n");
		free(hFont);
		return NULL;
	}

	if(FT_New_Face((FT_Library)(void *) hFont->library, fileName, 0,
	               (FT_Face *)(void *) &hFont->face)) {
		printf("Failed to load font %s\n", fileName);
		free(hFont);
		return NULL;
	}

	if(FT_Select_Charmap((FT_Face) hFont->face, FT_ENCODING_UNICODE)) {
		printf("Invalid charmap [%d]\n", FT_ENCODING_UNICODE);
		free(hFont);
		return NULL;
	}

	printf("++++++++++++++Font_create++++++++++++++++++\n");
	return hFont;
}

int Font_delete(Font_Handle hFont)
{
	if(hFont) {
		free(hFont);
	}

	return SW_EOK;
}

void test_hahah()
{
	return ;
}