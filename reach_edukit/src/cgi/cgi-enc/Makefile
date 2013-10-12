MODULES=DSS-ENC-1260
#######################附加模块###宏定义#####开始###############

ifeq "$(findstring DSS-ENC-1200,$(MODULES))" "DSS-ENC-1200"
DEFINES += -DDSS_ENC_1200
TARGETS = $(REACHDLL)_1200
endif

ifeq "$(findstring DSS-ENC-1100,$(MODULES))" "DSS-ENC-1100"
DEFINES += -DDSS_ENC_1100
TARGETS = $(REACHDLL)_1100
endif

ifeq "$(findstring DSS-ENC-1260,$(MODULES))" "DSS-ENC-1260"
DEFINES += -DDSS_ENC_1260
TARGETS = $(REACHDLL)_1260
endif
#######################附加模块###########结束##################

CC=/opt/CodeSourcery/Sourcery_G++_Lite/bin/arm-none-linux-gnueabi-gcc
#CC=/opt/mv_pro_5.0/montavista/pro/devkit/arm/v5t_le/bin/arm_v5t_le-gcc
#CC=/opt/mv_pro_5.0_729/montavista/pro/devkit/arm/v5t_le/bin/arm_v5t_le-gcc
#CC=/opt/CodeSourcery/Sourcery_G++_Lite/bin/arm-none-linux-gnueabi-gcc
#ANSICSTYPE=-pedantic

CFLAGS=-Wall $(ANSICSTYPE) -g 
#CFLAGS=-Wall $(ANSICSTYPE) -g

LDFLAGS=-L./ 
#LDFLAGS=
#LIBS=-lpthread
#PROGLDFLAGS=
INCLUDES=
CFLAGS:=$(CFLAGS) $(INCLUDES) $(DEFINES)
LDFLAGS:=$(LDFLAGS) $(LIBS)
SRCDIRS=.
CURDIR = .
SRCDIRS = .
SRCS=$(foreach dir,$(SRCDIRS),$(wildcard $(dir)/*.c))
LIBOBJS = cgi.o cgic.o htmllib.o Tools.o weblib.o webTcpCom.o
REACHDLL=encoder.cgi
#CC=arm_v5t_le-gcc
RM=rm -f



default:begin all

begin:
	$(RM) $(TARGETS) *.o 
	@echo "const static char *g_make_build_date = \"`date`\";" > build_info.h
	@echo "const static char *g_make_VSS_version = \"$(VSS_REV)\";" >> build_info.h
ifeq "$(findstring DSS-ENC-1100,$(MODULES))" "DSS-ENC-1100"
	@echo "now i will intall the DSS-ENC-1100 !"
endif
ifeq "$(findstring DSS-ENC-1200,$(MODULES))" "DSS-ENC-1200"
	@echo "now i will intall the DSS-ENC-1200 !"
endif
	
all:$(REACHDLL)
%.o : %.c
	@echo -- Compiling "-$(CC) $(CFLAGS) -c  $< -o $@"
	-$(CC) $(CFLAGS) -c  $< -o $@
	
$(REACHDLL) :$(LIBOBJS)
	@echo -- Linking [ $< ]
	@echo "-$(CC) $(LDFLAGS) $(LIBOBJS) -o $@ "
	-$(CC) $(LDFLAGS) $(LIBOBJS) -lrt -o $@ 
#	cp $(REACHDLL) $(TARGETS)
#	cp $(REACHDLL) /home/workdir/filesys/var/www/html/cgi-bin/
	cp $(REACHDLL) /home/reach/workdir/edukit_fs/var/www/html/cgi-bin/
#	cp encoder.cgi /home/reach/updatefile/tardir/var/www/html/cgi-bin
#	cp encoder.cgi /home/zhangmin/reach/8168/rfs/rfs-html/var/www/html/cgi-bin/
#	rm -rf build_info.h
	
$(LIBOBJS):weblib.h
clean:
	$(RM) $(TARGETS) *.o 

install:
	cp $(REACHDLL) /home/reach/workdir/edukit_fs/var/www/html/cgi-bin
 
