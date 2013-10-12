FS_DIR = /home/8168/reach_enc2000/rfs/rfs
FS_DIR = /home/reach/workdir/rfs
include $(dvr_rdk_PATH)/makerules/common_header_a8.mk

INCLUDE+= $(MCFW_INC) $(OSA_INC) $(SYSLINK_INC) $(CODEC_INC) $(LINUXDEVKIT_INC)\
	-I.\
	-I./text\
	-I./track\
	-I./text/freetype2\
	-I../log\
	-I../msg\
    	-I../reach_share\
    	-I../reach_share/libmedia\
	-I../reach_share/xml\
	-I../reach_share/xml/libxml\
	-I./middle\
	-I./stream_output\
	-I./xml_protocol\
	-I./modules/include\
	-I./modules/porting
#	-I./enc/rtsp	
	
SOURCE_DIR = $(shell pwd)

LIBS= -L$(FS_DIR)/usr/lib/ -lpng -lfreetype -lz  -lm -lrt 

INCLUDE += -I./middle -I./text
EXTERN_FILE :=	$(wildcard *.c)\
		$(wildcard middle/*.c)\
		$(wildcard text/*.c)\
		$(wildcard ../log/*.c)\
		$(wildcard track/*.c)\
		$(wildcard ../reach_share/xml/*.c)\
		$(wildcard ../reach_share/*.c)\
		$(wildcard stream_output/*.c)\
		$(wildcard xml_protocol/*.c)\
		$(wildcard modules/porting/*.c)
			
		
#EXTERN_DEFINE := -DHAVE_RTSP_MODULE
EXTERN_DEFINE += -DHAVE_OSD_MODULE
EXTERN_DEFINE += -DHAVE_JPEG

LIBS +=  '-Wl,--start-group' 
LIBS += $(MCFW_LIBS) $(AUDIO_LIBS) $(RPE_LIBS) 
LIBS += $(LIB_DIR)/reach_edukit.a
LIBS += ../../lib/libxml2.a 
LIBS += ../../lib/libz.a
#LIBS += ../../lib/*.a
#LIBS += ./rtsp/rtsp.a
#LIBS += ./modules/libs/*.a

LIBS += ./modules/libs/libcurlarm.a
LIBS += ./modules/libs/libcurl_portarm.a
LIBS += ./modules/libs/liblog.a
#LIBS += ./modules/libs/librtp_build.a
#LIBS += ./modules/libs/librtsp.a
#LIBS += ./modules/libs/libts_build.a

LIBS += '-Wl,--end-group'

LIBS += $(LIBS_OLD)

include $(dvr_rdk_PATH)/makerules/common_footer_a8.mk


