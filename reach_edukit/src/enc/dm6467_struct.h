#ifndef _DM6467_COMMON_H__
#define _DM6467_COMMON_H__

typedef enum {
    ColorSpace_NOTSET = -1,

    /**
      * @brief YUV 420 semi planar. The dm6467 VDCE outputs this format after a
      *        color conversion from #ColorSpace_YUV422PSEMI. The format
      *        consists of two planes: one with the Y component and one
      *        with the CbCr components interleaved (hence semi) See the LSP
      *        VDCE documentation for a thorough description of this format.
      */
    ColorSpace_YUV420PSEMI = 0,

    /**
      * @brief YUV 422 semi planar corresponding to V4L2_PIX_FMT_YUV422UVP. This
      *        format was added to v4l2 by TI because the dm6467 VDCE and VPSS
      *        peripherals use this format. The format consists of two planes:
      *        one with the Y component and one with the CbCr components
      *        interleaved (hence semi) See the LSP VDCE documentation for a
      *        thorough description of this format.
      */
    ColorSpace_YUV422PSEMI,

    /** @brief YUV 422 interleaved corresponding to V4L2_PIX_FMT_UYVY in v4l2 */
    ColorSpace_UYVY,
    
    /** @brief RGB 888 packed corresponding to V4L2_PIX_FMT_RGB24 in v4l2 */
    ColorSpace_RGB888,

    /** @brief RGB 565 packed corresponding to V4L2_PIX_FMT_RGB565 in v4l2 */
    ColorSpace_RGB565,

    /**
      * @brief 2 bits per pixel. This is the format used by the VDCE for the
      *        bitmap while blending and is documented in the VDCE peripheral
      *        guide.
      */
    ColorSpace_2BIT,
    
    /**
      * @brief YUV 420 planar. The format consists of three planes:
      *        one with the Y component, one Cb, and one Cr component.
      */
    ColorSpace_YUV420P,
    
    /**
      * @brief YUV 422 planar. The format consists of three planes:
      *        one with the Y component, one Cb, and one Cr component.
      */
    ColorSpace_YUV422P,

    /**
      * @brief YUV 444 planar. The format consists of three planes:
      *        one with the Y component, one Cb, and one Cr component.
      */
    ColorSpace_YUV444P,
    
    /**
      * @brief Gray Scale. The format consist of single Luma plane
      *        ignoring the color plane components.      
      *             
      */
    ColorSpace_GRAY,

    ColorSpace_COUNT
} ColorSpace_Type;

#define GET_2BPP_BYTE(x) ((x) >> 2)
#define GET_2BPP_SHIFT(x) (((x) % 4) * 2)

typedef struct BufferGfx_Dimensions_t{
	int x;
	int y;
	unsigned int width;
	unsigned int height;
	unsigned int	lineLength;
}BufferGfx_Dimensions;


typedef struct BufferGfx_Attrs {
    /** @brief The basic #Buffer_Attrs to use for creating the buffer. */
 //   Buffer_Attrs            bAttrs;

    /** @brief The #ColorSpace_Type of the buffer. */
    ColorSpace_Type         colorSpace;

    /** @brief The original dimensions of the buffer. */
    BufferGfx_Dimensions    dim;
} BufferGfx_Attrs;


typedef struct Buffer_Handle_t
{
	unsigned int bufsize;
	ColorSpace_Type colortype;
	BufferGfx_Dimensions dim;
	char *buf;
	
}Buffer_strcut;

typedef enum {
    SW_EFAIL       = -1,   //!< General failure code.
    SW_EOK         = 0,    //!< General success code.
} SW_ErrorCode;


typedef Buffer_strcut * Buffer_Handle;



int BufferGfx_getDimensions(Buffer_Handle hBuf, BufferGfx_Dimensions *dim);

int BufferGfx_getColorSpace(Buffer_Handle hBuf);

char *Buffer_getUserPtr(Buffer_Handle hBuf);

int Buffer_getSize(Buffer_Handle hBuf);

int write2_yuv_422(Buffer_Handle hCapBuf, int height, int width);

Int ColorSpace_getBpp(ColorSpace_Type colorSpace);

Buffer_Handle Buffer_create(Int32 size, BufferGfx_Attrs *attrs);






#endif

