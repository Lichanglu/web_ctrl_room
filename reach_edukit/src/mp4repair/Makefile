NAME := mp4repair

BIT:=64
PLATFORM:=arm
ifeq ($(PLATFORM), arm)
    PRE:=/opt/CodeSourcery/Sourcery_G++_Lite/bin/arm-none-linux-gnueabi-
    BIT:=
endif

CC:=$(PRE)gcc
CXX:=$(PRE)g++
AR:=$(PRE)ar
LINK:=$(PRE)gcc
LINKFLAGS := -fpic -shared
#LINKFLAGS := 
CFLAGS := -fpic -g
Q_ := 
.PHONY:	clean

INCDIR += -I ../reach_share/\
	  -I ../reach_share/xml\
	  -I ../reach_share/xml/libxml\
	  -I ../log/\
          -I ../reach_share/libmedia\
	  -I ../md5/
          
SRC = $(filter-out weblib.c mkimage.c,$(wildcard *.c *.cpp))
SRC +=$(wildcard ../reach_share/xml/*.c)
SRC +=$(filter ../reach_share/reach_os.c,$(wildcard ../reach_share/*.c))\
      $(wildcard ../reach_share/libmedia/*.c)\
      $(wildcard ../md5/*.c)

DIR = $(notdir$(SRC))
OBJ += $(patsubst %.c,%.o,$(patsubst %.cpp,%.o,$(SRC)))

CFLAGS += $(INCDIR)
 
#DYN_LINKS_WITH +=  -lpthread -lrt -lm -lstdc++
DYN_LINKS_WITH +=  -lpthread -lrt -lm
STATIC_LINKS_WITH += 

ifeq ($(PLATFORM), arm)
    STATIC_LINKS_WITH += $(wildcard ../../lib/*.a)
endif


ifeq ($(PLATFORM), linux)
    STATIC_LINKS_WITH += $(wildcard ../../lib64/*.a)
endif

%.o: %.c
	echo '<$(CC)>[$(DBG)] Compiling object file "$@" ...'
	$(Q_)${CC} $(CFLAGS) -c $< -o $@

%.o: %.cpp
	echo '<$(CXX)>[$(DBG)] Compiling object file "$@" ...'
	$(Q_)${CXX} $(CFLAGS) -c $< -o $@

all: $(NAME)
$(NAME):$(OBJ)
	echo 1$(OBJ)
	echo 2'<$(AR)>creating binary "$(NAME)"'
	$(Q_)$(LINK)  -o $(NAME) $(OBJ) $(DYN_LINKS_WITH) '-Wl,--start-group' $(STATIC_LINKS_WITH) '-Wl,--end-group'

install:
	cp $(NAME) /usr/local/lib
	cp $(NAME) ~/workdir/rfs/usr/lib
	cp *.h /usr/local/include
	cp *.h ~/workdir/rfs/usr/include

clean:
	rm $(NAME) -f
	rm $(OBJ)  -rf

mkimage:mkimage.o ../md5/md5lib.o ../reach_share/reach_os.o
	$(Q_)$(LINK)  -o mkimage mkimage.o ../md5/md5lib.o ../reach_share/reach_os.o -lpthread -lrt
mkimage_clean:
	rm -f *.o  ../md5/md5lib.o ../reach_share/reach_os.o mkimage
