###### Control 进程 ########
# 系统平台
ARCH = $(shell arch)
BITS =
ifeq "$(findstring x86_64,$(ARCH))" "x86_64"
BITS = 64
endif
#开关
ON = 1
OFF = 0

#版本/
VERSION =


#日期
#DATE=$(shell date +%Y%m%d)

# 目标文件名字
#APP = control_$(VERSION)_$(ARCH)
APP = control
BIN_DIR = bin
$(shell mkdir -p $(BIN_DIR)) 
TARGET =$(BIN_DIR)/$(APP)


# c源文件
SRC = $(wildcard ./*.c)\
 $(wildcard ../reach_share/reach_os.c)\
 $(wildcard ../reach_share/xml/*.c)\
 $(wildcard ../reach_share/curl$(BIT)/*.c)\
 $(wildcard ../reach_share/http/*.c)\
 $(wildcard ../tools/rsa/cli_verify.c)\
 $(wildcard ./web/*.c)\
 $(wildcard ./lcd/*.c)\
 $(wildcard ./../md5/*.c)\
 $(wildcard ../sericalctrl/*.c)
# object文件i
OBJS = $(SRC:.c=.o)
OBJS_DIR = objs
$(shell mkdir -p $(OBJS_DIR)) 
#OBJS_DEST = $(wildcard $(OBJS_DIR)/*.o)
EXTERN_OBJ :=  $(foreach n,$(notdir $(OBJS)),$(OBJS_DIR)/$(n))


# 编译器
CC = /opt/CodeSourcery/Sourcery_G++_Lite/bin/arm-none-linux-gnueabi-gcc

# 头文件路径，可以任意指定头文件
INCLUDE := -I./\
 -I../reach_share\
 -I../reach_share/xml\
 -I../reach_share/xml/libxml\
 -I../reach_share/curl$(BITS)\
 -I../reach_share/http\
 -I../reach_share/iconv\
 -I../tools/rsa\
 -I../log\
 -I./lcd\
 -I../md5\
 -I../sericalctrl\
 -I../room/enc

# 链接库
#LIBS := -lpthread -lrt -lm -lstdc++
LIBS := -lpthread -lrt -lm -ldl
LOCAL_LIBS := $(wildcard ../../lib$(BITS)/*.a) \
				$(wildcard ../../lib$(BITS)/*.so.*)

LIBS += '-Wl,--start-group' $(LOCAL_LIBS) '-Wl,--end-group'

# 编译选项
CFLAGS = -g -Wall
#CFLAGS := -O2 -Wall -Wno-strict-aliasing -g
#CFLAGS = -O2
all: $(TARGET)

# 定义目标文件生成规则
$(TARGET):FIRST $(OBJS)
	echo #$(OBJS)#
	echo #$(EXTERN_OBJ)#
	$(CC) -o $(TARGET) $(EXTERN_OBJ) $(LIBS)

FIRST:
	rm -rf $(OBJS)
	
.SUFFIXES:.c	
# 宏定义
# 定义obejct文件生成规则
.c.o:
	$(CC) $(INCLUDE) $(CFLAGS) -c $<
	@mv *.o $(OBJS_DIR)
# 打安装包
pkgs:
	mkdir -p pkg/$(APP)
	cp -rf $(TARGET) pkg/$(APP)
	cp -rf pkg/$(SERVICE_TYPE)d pkg/$(APP)
	cp -rf pkg/$(TARGET)d pkg/$(APP)
	cp -rf pkg/install_$(SERVICE_TYPE) pkg/$(APP)
	cp -rf pkg/uninstall_$(SERVICE_TYPE) pkg/$(APP)
	cp -rf pkg/readme_$(SERVICE_TYPE) pkg/$(APP)
	cp -rf pkg/cfg_$(SERVICE_TYPE).xml pkg/$(APP)
	cp -rf pkg/nslog_$(SERVICE_TYPE).conf pkg/$(APP)
	echo $(APP) > pkg/version
	cp -rf pkg/version pkg/$(APP)
	cd pkg;tar cfvz $(APP).build$(DATE).tgz $(APP);rm -rf $(APP)
	cd pkg;cat inst_script.sh $(APP).build$(DATE).tgz > $(APP)_intaller;chmod +x $(APP)_intaller

# 定义清理函数
.PHONY:clean
clean:
	@rm -rf $(OBJS_DIR) $(TARGET)
	@rm -rf pkg/$(APP)*
	@rm -rf $(OBJS_DIR)
	@rm -rf $(BIN_DIR)
	@rm -rf version.h
	@rm -rf control_config.xml log_file
