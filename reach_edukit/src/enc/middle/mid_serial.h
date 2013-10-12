#ifndef _MID_SERIAL_H__
#define _MID_SERIAL_H__
int mid_serial_set_opt(int fd, int nspeed, int nbits, char nevent, int nstop);
int mid_serial_read_data(int fd, char *buff, int len, int timeout);
int mid_serial_close_data(int fd);
int mid_serial_write_data(int fd, char *buff, int len);
int mid_serial_open_fd(char *dev);
#endif

