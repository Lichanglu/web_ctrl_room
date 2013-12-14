#ifndef _APP_PRODUCT_H__
#define _APP_PRODUCT_H__


#define UPDATE_PRODUCT_ID "ENC2000"   //1200 dvi/sdi 是一样的
#define UPDATE_MAJOR_VERSION 2 //3.0.23   //表示当前文件大版本号。
#if 0
//ENC1200/DVI/SDI
#ifdef DSS_ENC_1200
#define UPDATE_PRODUCT_ID "ENC1200DVI"   //1200 dvi/sdi 是一样的
#define UPDATE_MAJOR_VERSION 4 //3.0.23   //表示当前文件大版本号。
#endif


//ENC1100
#ifdef DSS_ENC_1100
#define UPDATE_PRODUCT_ID "ENC1100DVI"
#define UPDATE_MAJOR_VERSION 2 //1.0.23
#endif

//CL4000 DVI
#ifdef CL4000_DVI
#define UPDATE_PRODUCT_ID "CL4000DVI"
#define UPDATE_MAJOR_VERSION 2 //1.0.23
#endif

//CL4000 SDI
#ifdef CL4000_SDI
#define UPDATE_PRODUCT_ID "CL4000SDI"
#define UPDATE_MAJOR_VERSION 2 //1.0.23
#endif

//CL4000 双路SD
#ifdef CL4000_DOUBLE_SD
#define UPDATE_PRODUCT_ID "CL4000DSD"
#define UPDATE_MAJOR_VERSION 3 //2.2.4
#endif

//DEC1100 
#ifdef DEC1100
#define UPDATE_PRODUCT_ID "DEC1100"
#define UPDATE_MAJOR_VERSION 1 //2.2.4
#endif
#endif


#endif


