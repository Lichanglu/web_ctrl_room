#ifndef _SD_DEMO_OSD_H__
#define _SD_DEMO_OSD_H__


#define SD_DEMO_OSD_NUM_WINDOWS      (4)

#define SD_DEMO_OSD_WIN0_STARTX      (16)
#define SD_DEMO_OSD_WIN0_STARTY      (16)
#define SD_DEMO_OSD_GLOBAL_ALPHA    (0x80)
#define SD_DEMO_OSD_MAX_FILE_NAME_SIZE (128)

#define OSD_MAX_WIDTH  720
#define OSD_MAX_HEIGHT 1080

/**
    \brief Allocated buffer info
*/
typedef struct {

	UInt8  *physAddr;
	/**< Physical address */

	UInt8  *virtAddr;
	/**< Virtual address */

	UInt32  srPtr;
	/**< Shared region Pointer SRPtr */

	UInt32 bufsize;

} SD_Demo_AllocBufInfo;


enum{
	WINDOW_TEXT_OSD,
	WINDOW_TIME_OSD,
	WINDOW_LOGO_OSD,
	WINDOW_MAX_OSD
};

extern int SD_Demo_osdInit(int algid);
extern void SD_Demo_osdDeinit(void);
extern Int32 SD_Demo_allocBuf(UInt32 srRegId, UInt32 bufSize, UInt32 bufAlign, SD_Demo_AllocBufInfo *bufInfo);
extern Int32 SD_Demo_freeBuf(UInt32 srRegId, UInt8 *virtAddr, UInt32 bufSize);
#endif
