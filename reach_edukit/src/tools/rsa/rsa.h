#ifndef _REACH_RSA_H
#define _REACH_RSA_H

#ifdef __cplusplus
extern "C"{
#endif

#define C_PRIVATE_KEY_FILE  "c_pri.key"
#define C_PUBLIC_KEY_FILE   "c_pub.key"
#define S_PRIVATE_KEY_FILE  "s_pri.key"
#define S_PUBLIC_KEY_FILE   "s_pub.key"
#define C_REG_FILE          "c_device.reg"
#define C_DEVICE_KEY_FILE   "c_device.key"
#define NRS_SETUP_FILE      "nrs_setup.dat"

int     rsa_gen_key_pair(unsigned char *pub_key, int *pub_len 
                           ,unsigned char *pri_key, int *pri_len );

//******************************************************************************
int     rsa_public_encrypt(unsigned char *in_data, int in_len
                            ,unsigned char *out_data, int *out_len
                            ,unsigned char *pub_key, int pub_len);

int     rsa_private_decrypt(unsigned char *in_data, int in_len
                             ,unsigned char *out_data, int *out_len
                             ,unsigned char *pri_key, int pri_len);

//******************************************************************************
int     rsa_private_encrypt(unsigned char *in_data, int in_len
                            ,unsigned char *out_data, int *out_len
                            ,unsigned char *pri_key, int pri_len);

int     rsa_public_decrypt(unsigned char *in_data, int in_len
                             ,unsigned char *out_data, int *out_len
                             ,unsigned char *pub_key, int pub_len);

int     rsa_read_file(const char *sz_file, unsigned char *buf, int *buf_len);
int     rsa_write_file(const char *sz_file, unsigned char *buf,int buf_len);

int     rsa_read_hardware_info(unsigned char *hardware_info, int hd_info_len);

#ifdef __cplusplus
}
#endif
#endif

