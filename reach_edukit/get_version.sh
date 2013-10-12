#!/bin/sh
#
#该程序为自动获取SDK的开发版本号，并且能够比较项目的平台版本
#
#信息与当前SDK版本信息是否一致。
#
echo "#######################################################################################";
#################################下面代码不要修改，是息息相关的##############################
echo "DVRRDK GIT SERVER Version Information"
project_pwd=`pwd`
cur_pwd=${project_pwd}/../
dvr_rdk_ver=`cd ${cur_pwd};git describe --dirty --always`
sed -e 's/<dvr_rdk_ver>/'${dvr_rdk_ver}'/g' version.h.in > version.h.rdk
echo "dvr_rdk" ${dvr_rdk_ver}
mcfw_ver=`cd ${cur_pwd}/mcfw;git describe --dirty --always`
sed -e 's/<mcfw_ver>/'${mcfw_ver}'/g' version.h.rdk  > version.h.mcfw
rm -f version.h.rdk
echo "mcfw" ${mcfw_ver}

ti_tools_ver=`cd ${cur_pwd}/../ti_tools;git describe `
sed -e 's/<ti_tools_ver>/'${ti_tools_ver}'/g' version.h.mcfw  > version.h.tools
rm -f version.h.mcfw
echo "ti_tools" ${ti_tools_ver}

hdvpss_ver=`cd ${cur_pwd}/../ti_tools/hdvpss;git describe --dirty --always`
sed -e 's/<hdvpss_ver>/'${hdvpss_ver}'/g' version.h.tools  > version.h.hdvpss
rm -f version.h.tools
echo "hdvpss" ${hdvpss_ver}

linux_lsp_ver=`cd ${cur_pwd}/../ti_tools/linux_lsp;git describe`
sed -e 's/<linux_lsp_ver>/'${linux_lsp_ver}'/g' version.h.hdvpss  > version.h.lsp
rm -f version.h.hdvpss
echo "linux_lsp" ${linux_lsp_ver}

kernel_ver=`cd ${cur_pwd}../ti_tools/linux_lsp/linux-psp-dvr-04.04.00.01/src/linux-04.04.00.01;git describe`
sed -e 's/<kernel_ver>/'${kernel_ver}'/g' version.h.lsp  > version.h.kernel
rm -f version.h.lsp
echo "kernel" ${kernel_ver}

uboot_ver=`cd ${cur_pwd}../ti_tools/linux_lsp/linux-psp-dvr-04.04.00.01/src/u-boot-04.04.00.01;git describe`
sed -e 's/<uboot_ver>/'${uboot_ver}'/g' version.h.kernel  > version.h.uboot
rm -f version.h.kernel
echo "uboot" ${uboot_ver}

SDK_ver=`cd ${cur_pwd}/../;git describe --dirty --always`
sed -e 's/<SDK_ver>/'${SDK_ver}'/g' version.h.uboot  > version.h.SDK
rm -f version.h.uboot
echo "SDK" ${SDK_ver}
cat version.h.SDK > version.h
cat version.h.SDK > SDK_ver.h
rm version.h.SDK
####################################增加在代码请在下面增加#############################################

#reach_ver=`cd ${cur_pwd}/reach_project;git describe`
#cat version.h > version.h.tmp
#sed -e 's/<project_ver>/'${reach_ver}'/g' version.h.tmp > version.h.project
#rm -f version.h.tmp
#cat version.h.project > version.h
#rm -f version.h.project


#example start: 增加ENC1260项目的自动版本信息实例
#ENC1260_ver=`cd ${cur_pwd}/enc1260;git describe --dirty --always`
#cat version.h > version.h.tmp
#sed -e 's/<enc1260_ver>/'${ENC1260_ver}'/g' version.h.tmp > version.h.enc1260
#rm -f version.h.tmp
#cat version.h.enc1260 > version.h
#
#example end:


echo "Version Finish"
#if diff  pro_ver.h version.h ; then 
#	echo "OK--------------------------------------- verison.h"
#	exit 1;
#else 
#	echo "ERROR------------------------------------ version.h" ; 
#	exit 0;
#fi
echo "######################################################################################"

