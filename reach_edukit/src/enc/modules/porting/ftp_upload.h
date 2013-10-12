#ifndef __FTP_UPLOAD_H
#define __FTP_UPLOAD_H

#define SIZE			1024
#define PATH_SIZE       256
#define BUF_SIZE        SIZE * 512
#define FTPPATH         "ftpupload.ini"
#define DEFAULT_TIME	"3"

void ftpupload_uninit(void);
void ftpupload_init(void);
void app_ftp_init(void);
int user_set_ftp_size(int size);

int set_ftp_url_ip(char *, char *);



#endif
