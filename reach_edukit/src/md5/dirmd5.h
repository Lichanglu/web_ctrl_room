#ifndef _DIR_MDF_H
#define _DIR_MDF_H



#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <dirent.h>  
#include <assert.h>
#include "string.h"
#include "reach_os.h"
#include "md5lib.h"


#ifndef MAX_FILE_PATH_LEN
#define MAX_FILE_PATH_LEN  (400)
#endif

#define MP4_READ_LEN (4*1024)


extern  int32_t  DirDetectionFile(int8_t *UpLoadFilePath);
#endif
