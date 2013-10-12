
CURRENT_DIR =  $(CURDIR)
MODULE_DIR =  $(notdir $(CURDIR))
REACH_SHARE = reach_share
NSLOG = log
check_version:
	./get_version.sh
	@if diff  pro_ver.h SDK_ver.h ; then \
		echo "check SDK version! *******OK*******"; \
	else \
		echo "Error check SDK version mismatch!!!   please check version.h and pro_ver.h files! " ; \
		exit 1;  \
	fi

demo:
	#echo #$(MODULE_DIR)#
	#echo #$(CURRENT_DIR)#
	mkdir $(dvr_rdk_PATH)/lib/ti816x-evm/obj/$(MODULE_DIR)/ -p
	mkdir $(dvr_rdk_PATH)/lib/ti816x-evm/obj/$(REACH_SHARE)/xml -p
	mkdir $(dvr_rdk_PATH)/lib/ti816x-evm/obj/$(NSLOG) -p
	
#add subdir 	
	mkdir $(dvr_rdk_PATH)/lib/ti816x-evm/obj/$(MODULE_DIR)/middle -p
	mkdir $(dvr_rdk_PATH)/lib/ti816x-evm/obj/$(MODULE_DIR)/text -p
	mkdir $(dvr_rdk_PATH)/lib/ti816x-evm/obj/$(MODULE_DIR)/xml -p
	mkdir $(dvr_rdk_PATH)/lib/ti816x-evm/obj/$(MODULE_DIR)/modules/porting -p
	mkdir $(dvr_rdk_PATH)/lib/ti816x-evm/obj/$(MODULE_DIR)/stream_output -p
	mkdir $(dvr_rdk_PATH)/lib/ti816x-evm/obj/$(MODULE_DIR)/track -p
	mkdir $(dvr_rdk_PATH)/lib/ti816x-evm/obj/$(MODULE_DIR)/xml_protocol -p

	#$(MAKE) -fMAKEFILE.MK check_version
	$(MAKE) -fMAKEFILE.MK libs
	#cd ./src/room/ctrl/ && $(MAKE) -fMakefile && cd -
	$(MAKE) -fMAKEFILE.MK apps TARGET=exe

libs:
	#$(MAKE) -f Makefile MODULE=$(MODULE_DIR) $(TARGET)
	$(MAKE) -f Makefile -C$(dvr_rdk_PATH)/$(MODULE_DIR)/src/enc MODULE=$(MODULE_DIR) $(TARGET)

apps:
	#$(MAKE) -f Makefile MODULE=$(MODULE_DIR) $(TARGET)
	$(MAKE) -f Makefile -C$(dvr_rdk_PATH)/$(MODULE_DIR)/src/enc MODULE=$(MODULE_DIR) $(TARGET)

clean:
	$(MAKE) -fMAKEFILE.MK libs TARGET=clean


format:
	$(MAKE) -f Makefile -C$(dvr_rdk_PATH)/$(MODULE_DIR)/src/enc format
	
	
all: clean demo

