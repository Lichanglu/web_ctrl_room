NAME := sericalctrl

BIT:=64
PLATFORM:=arm
ifeq ($(PLATFORM), arm)
    PRE:=arm-none-linux-gnueabi-
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
	 
SRC = $(wildcard *.c *.cpp)
SRC +=$(wildcard ../reach_share/xml/*.c)
SRC +=$(filter ../reach_share/reach_os.c,$(wildcard ../reach_share/*.c))\
   
DIR = $(notdir$(SRC))
OBJ += $(patsubst %.c,%.o,$(patsubst %.cpp,%.o,$(SRC)))

CFLAGS += $(INCDIR)
 
#DYN_LINKS_WITH +=  -lpthread -lrt -lm -lstdc++
DYN_LINKS_WITH +=  -lpthread -lrt -lm
ifeq ($(PLATFORM), arm)
    STATIC_LINKS_WITH += ../../lib/libxml2.a ../../lib/libz.a ../../lib/libzlog.a
endif

STATIC_LINKS_WITH += 

ifeq ($(PLATFORM), linux)
    STATIC_LINKS_WITH += ../../lib64/libxml2.a ../../lib64/libz.a ../../lib64/libzlog.a
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

