#ifndef _OUR_MD5_H_
#define _OUR_MD5_H_
#include "our_md5.h"


char * our_MD5End(MD5_CTX *ctx, char *buf);
char * our_MD5File(const char *filename, char *buf);
char * our_MD5Data (const unsigned char *data, unsigned int len, char *buf);

#endif /* _OUR_MD5_H_ */

