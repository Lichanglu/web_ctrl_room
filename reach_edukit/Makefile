

.PHONY:control ftp  cgi repair daemon tools CM_Server_install mkpack CM_Server

SRC:=./src
OBJ:= control ftp  repair room 

APPBIN = control ftpserver mp4repair daemon
WEBBIN = encoder.cgi
CM_APPPATH = ./CM_Server_install/usr/local/reach/

ROOTFS = /home/8168/reach_edu/rfs
MKRFS = /home/8168/reach_edu/mkrfs_edu-sd
UPDATEFS = /home/8168/reach_edu/edukit_update
LICHLFS = /home/reach/workdir/sd_edukit_fs/
$(shell mkdir -p $(UPDATEFS))
all:$(OBJ)

control:
	rm -rf ./bin/
	mkdir ./bin/
	$(MAKE) -C $(SRC)/control -f Makefile
	cp $(SRC)/control/bin/control ./bin/
	cp $(SRC)/control/bin/control $(ROOTFS)/usr/local/reach/ &
	cp $(SRC)/control/bin/control $(MKRFS)/usr/local/reach/  &
	cp $(SRC)/control/bin/control $(UPDATEFS)/usr/local/reach/  &
#	cp $(SRC)/control/bin/control $(LICHLFS)/usr/local/reach/

room:
	$(MAKE) -C $(SRC)/room/ctrl -f Makefile
	cp $(SRC)/room/ctrl/room  $(ROOTFS)/usr/local/reach/
	cp $(SRC)/room/ctrl/room  $(MKRFS)/usr/local/reach/
	cp $(SRC)/room/ctrl/room  $(UPDATEFS)/usr/local/reach/
#	cp $(SRC)/room/ctrl/room  $(LICHLFS)/usr/local/reach/

ftp:
	$(MAKE) -C $(SRC)/ftp -f Makefile
	cp $(SRC)/ftp/ftpserver ./bin/
	cp $(SRC)/ftp/ftpserver   $(ROOTFS)/usr/local/reach/
	cp $(SRC)/ftp/ftpserver   $(MKRFS)/usr/local/reach/
	cp $(SRC)/ftp/ftpserver   $(UPDATEFS)/usr/local/reach/
	
serical:
	$(MAKE) -C $(SRC)/sericalctrl -f Makefile
	cp $(SRC)/sericalctrl/sericalctrl ./bin/
	cp $(SRC)/sericalctrl/sericalctrl   $(ROOTFS)/usr/local/reach/
	
netctrl:
	$(MAKE) -C $(SRC)/netctrl -f Makefile
	cp $(SRC)/netctrl/netctrl ./bin/
	cp $(SRC)/netctrl/netctrl   $(ROOTFS)/usr/local/reach/
	
html:
	rm -rf $(ROOTFS)/var/www/html
	rm -rf $(MKRFS)/var/www/html
	cp -rf $(SRC)/cgi/html  $(ROOTFS)/var/www/	
	cp -rf $(SRC)/cgi/html  $(MKRFS)/var/www/	
	cp -rf $(SRC)/cgi/html  $(UPDATEFS)/var/www/	
cgi:
	$(MAKE) -C $(SRC)/cgi/cgi-enc -f Makefile
	cp $(SRC)/cgi/cgi-enc/encoder.cgi  $(ROOTFS)/var/www/html/cgi-bin/	
	cp $(SRC)/cgi/cgi-enc/encoder.cgi  $(MKRFS)/var/www/html/cgi-bin/	
	cp $(SRC)/cgi/cgi-enc/encoder.cgi  $(UPDATEFS)/var/www/html/cgi-bin/	
repair:
	$(MAKE) -C $(SRC)/mp4repair -f Makefile
	cp $(SRC)/mp4repair/mp4repair ./bin/
	cp ./bin/mp4repair $(ROOTFS)/usr/local/reach/
	cp ./bin/mp4repair $(MKRFS)/usr/local/reach/
	cp ./bin/mp4repair $(UPDATEFS)/usr/local/reach/
rmcourse:
	cp $(SRC)/sbin/rmcourse $(ROOTFS)/usr/local/reach/
	cp $(SRC)/sbin/rmcourse $(MKRFS)/usr/local/reach/	
	cp $(SRC)/sbin/rmcourse $(UPDATEFS)/usr/local/reach/	
reachkit:
	cp $(SRC)/sbin/reachkit.sh $(ROOTFS)/usr/local/reach/
	cp $(SRC)/sbin/reachkit.sh $(MKRFS)/usr/local/reach/
	cp $(SRC)/sbin/reachkit.sh $(UPDATEFS)/usr/local/reach/
recourse:
	$(MAKE) -C $(SRC)/recourse -f Makefile
	cp $(SRC)/recourse/recourse_list $(ROOTFS)/usr/local/reach/
	cp $(SRC)/recourse/recourse_list $(MKRFS)/usr/local/reach/
	cp $(SRC)/recourse/recourse_list $(UPDATEFS)/usr/local/reach/
#enc:
#	$(MAKE) -C $(SRC)/enc -f Makefile	
	
#daemon:
#	$(MAKE) -C $(SRC)/tools/daemon -f Makefile
#	cp $(SRC)/tools/daemon/daemon ./bin/ 
	
#mkpack:
#	$(MAKE) -C $(SRC)/tools/makepackage -f Makefile
#	cp $(SRC)/tools/makepackage/mkpack ./bin/

install:
	@rm -rf ./bin/
	@mkdir ./bin/

clean:
	$(MAKE) -C $(SRC)/control -f Makefile clean
	$(MAKE) -C $(SRC)/room/ctrl -f Makefile clean
	$(MAKE) -C $(SRC)/ftp -f Makefile clean
	$(MAKE) -C $(SRC)/sericalctrl -f Makefile clean
	$(MAKE) -C $(SRC)/netctrl -f Makefile clean
	$(MAKE) -C $(SRC)/cgi/cgi-enc -f Makefile clean
	$(MAKE) -C $(SRC)/mp4repair -f Makefile clean
	$(MAKE) -C $(SRC)/recourse -f Makefile clean
#	$(MAKE) -C $(SRC)/tools/daemon -f Makefile clean
#	#$(MAKE) -C $(SRC)/tools/makepackage -f Makefile clean
	rm -f ./bin -fr

fs_install:fs_clean
	cd bin && cp  $(APPBIN) ../$(CM_APPPATH) -f 
	cd bin && cp  $(WEBBIN) ../$(CM_APPPATH)var/www/html/cgi-bin/ -f

fs_clean:
	cd $(CM_APPPATH) && rm  $(APPBIN) -f
	cd $(CM_APPPATH) && rm  var/www/html/cgi-bin/$(WEBBIN) -f
	rm -fr CM_Server_install/filelist.xml

selfinstall:all install fs_install
	cp $(SRC)/tools/SelfInstall/* ./bin/ -fr
	cd bin && ./install ../CM_Server_install

selfinstall_clean:
	cd bin && rm -f CM_Server_install.sh

upgrade:all install fs_install
	cp /home/reach/workdir/rfs/usr/local/reach/dvr_rdk/ti816x/bin/reach_edukit.out CM_Server_install/usr/local/reach/dvr_rdk/ti816x/bin/
	cp /home/reach/workdir/rfs/usr/local/reach/dvr_rdk/ti816x/firmware/* CM_Server_install/usr/local/reach/dvr_rdk/ti816x/firmware/
	#mv ./CM_Server_install/etc/sysconfig/network-scripts ./network-scripts_tmp
	cp $(SRC)/tools/SelfInstall/mkpack ./bin/
	cd bin && ./mkpack ../CM_Server_install
	#mv ./network-scripts_tmp ./CM_Server_install/etc/sysconfig/network-scripts
upgrade_clean:
	cd bin && rm -f update.bin
