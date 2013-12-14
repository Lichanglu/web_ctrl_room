CC=/opt/CodeSourcery/Sourcery_G++_Lite/bin/arm-none-linux-gnueabi-gcc
INCLUDE := -fpic -I./\
 -I../../log/\
 -I../../msg/\
 -I../../md5/\
 -I../receive/\
 -I../udp_recv/\
 -I../udp_snd/\
 -I../lives/\
 -I../rec/\
 -I../../reach_share/\
 -I../../reach_share/xml/\
 -I../../reach_share/xml/libxml/\
 -I../../reach_share/libmedia/\
 -I../../reach_share/\
 -I../../recourse/
TARGETS=room
SRC := $(wildcard ./*.c)\
		 $(wildcard ../../reach_share/reach_os.c)\
	   $(wildcard ../../reach_share/libmedia/*.c)\
	   $(wildcard ../../reach_share/reach_socket.c)\
	   $(wildcard ../../reach_share/reach_rtp_build.c)\
	   $(wildcard ../../reach_share/reach_udp_snd.c)\
	   $(wildcard ../../reach_share/xml/*.c)\
	   $(wildcard ../../msg/*.c)\
	   $(wildcard ../../md5/*.c)\
	   $(wildcard ../lives/*.c)\
	   $(wildcard ../../recourse/*.c)

SRC += ../receive/media_frame.c\
	../receive/receive_module.c\
	../receive/receive_xml_protocol.c\
	../../reach_share/reach_udp_recv.c
SRC +=  ../rec/record.c \
	../rec/record_xml.c

LIBS := -lpthread
#LIBS += -lrt -lm -lstdc++ 
LIBS += -lrt -lm
LOCAL_LIBS := $(wildcard ../../../lib/*.a) 
LIBS += '-Wl,--start-group' $(LOCAL_LIBS) '-Wl,--end-group'
CFLAGS := -g
OBJS_DIR = objs
$(shell mkdir -p $(OBJS_DIR))

OBJS = $(subst .c,.o, $(SRC) )
EXTERN_OBJ :=  $(foreach n,$(notdir $(OBJS)),$(OBJS_DIR)/$(n))



$(TARGETS): FIRST $(OBJS)
	echo #$(OBJS)#
	echo #$(EXTERN_OBJ)#
	$(CC) -o $(TARGETS) $(EXTERN_OBJ) $(LIBS)
FIRST:
	rm -rf $(OBJS)
.c.o:
	$(CC) $(INCLUDE) $(CFLAGS) -c $<
	mv *.o $(OBJS_DIR)
client:
	$(CC) -o $(CLITARGET) $(CLIDIR)
.PHONY:clean
clean:
	rm -rf $(OBJS_DIR)
	rm  -rf ./*.o
	rm -rf $(TARGETS)
