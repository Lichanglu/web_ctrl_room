CC=arm-none-linux-gnueabi-gcc
INCLUDE := -fpic -I./\
 -I../../log/\
 -I../../msg/\
 -I../../reach_share/\
 -I../../reach_share/xml/\
 -I../../reach_share/xml/libxml/\
 -I../../reach_share/libmedia/\
 -I../../reach_share/reach_udp_snd/
TARGETS=main
SRC := $(wildcard ../../reach_share/reach_os.c)\
	   $(wildcard ../../reach_share/reach_socket.c)\
	   $(wildcard ../../reach_share/reach_list.c)\
	   $(wildcard ../../reach_share/reach_epoll.c)\
	   $(wildcard ../../reach_share/xml/*.c)\
	   $(wildcard ../../reach_share/reach_udp_snd/*.c)\
	   $(wildcard ./*.c)
LIBS := -lpthread -lm
LIBS += -lrt 
LOCAL_LIBS := $(wildcard ../../../lib/*.a) 
LIBS += '-Wl,--start-group' $(LOCAL_LIBS) '-Wl,--end-group'
CFFLAGS := -g
OBJS_DIR = objs
$(shell mkdir -p $(OBJS_DIR))
OBJS_DEST= $(wildcard $(OBJS_DIR)/*.o)
OBJS = $(SRC:.c=.o)
$(TARGETS):$(OBJS)
	$(CC) -o $(TARGETS) $(OBJS_DEST) $(LIBS)
	#cp $(TARGETS) /liuchao3/encode_test/ -f
.c.o:
	$(CC) $(INCLUDE) $(CFLAGS)\
	 -c $<
	mv *.o $(OBJS_DIR)
client:
	$(CC) -o $(CLITARGET) $(CLIDIR)
clean:
	rm -rf $(OBJS_DIR)
	rm -rf $(TARGETS)
