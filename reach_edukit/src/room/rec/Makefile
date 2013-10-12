######Makefile模板########
# 系统平台
ARCH = $(shell arch)
BITS =
ifeq "$(findstring x86_64,$(ARCH))" "x86_64"
BITS = 64
endif
SRC = ../../
ROOT = ../../../
#开关
ON = 1
OFF = 0
#媒体平台直播或者点播
SERVICE_TYPE = record
#版本/
#RECORD_VERSION = $(shell git describe --dirty --always)
#RECORD_VERSION = $(shell git tag)
RECORD_VERSION = 1.0.0

_RECORD_VERSION = \"$(RECORD_VERSION)\"
#日期
DATE=$(shell date +%Y%m%d)
# 目标文件名字
TARGET = $(SERVICE_TYPE)s
APP = $(SERVICE_TYPE)s.$(RECORD_VERSION).$(ARCH)

# c源文件
SRC_C := $(wildcard ./*.c)\
 $(wildcard $(SRC)reach_share/*.c)\
 $(wildcard $(SRC)reach_share/xml/*.c)\
 $(wildcard $(SRC)msg/*.c)\
 $(wildcard $(SRC)reach_share/libmedia/*.c)\
 $(wildcard ../receive/*.c)
# object文件
OBJS = $(SRC_C:.c=.o)
OBJS_DIR = objs
$(shell mkdir -p $(OBJS_DIR)) 
OBJS_DEST = $(wildcard $(OBJS_DIR)/*.o)
# 编译器
CC = gcc
# 头文件路径，可以任意指定头文件
INCLUDE := -I./\
 -I$(SRC)log/\
 -I$(SRC)msg/\
 -I$(SRC)reach_share/\
 -I$(SRC)reach_share/xml/\
 -I$(SRC)reach_share/xml/libxml/\
 -I$(SRC)reach_share/libmedia/\
 -I../receive/
# 链接库
LIBS := -lpthread
LIBS += -lrt -lm
LOCAL_LIBS := $(wildcard $(ROOT)lib$(BITS)/*.a) 
LIBS += '-Wl,--start-group' $(LOCAL_LIBS) '-Wl,--end-group'
# 编译选项
CFLAGS = -g
#CFLAGS := -O2 -Wall -Wno-strict-aliasing -g
#CFLAGS = -O2

# 定义目标文件生成规则
$(TARGET):$(OBJS)
	$(CC) -o $(TARGET) $(OBJS_DEST) $(LIBS)
#宏定义
# 定义obejct文件生成规则
.c.o:
	$(CC) $(INCLUDE) $(CFLAGS) -c $<
	mv *.o $(OBJS_DIR)
# 定义清理函数
.PHONY:clean
clean:
	rm -rf $(OBJS_DEST) $(TARGET)
	rm -rf $(OBJS_DIR)
