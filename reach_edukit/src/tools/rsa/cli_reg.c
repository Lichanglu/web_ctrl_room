#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "rsa.h"


#define R_C_PRIVATE_KEY_FILE  "/etc/reach/c_pri.key"
#define R_S_PUBLIC_KEY_FILE  "/etc/reach/s_pub.key"
#define R_C_REG_FILE "/etc/reach/c_device.reg"

static int write_pri_key(unsigned char *pri_key, int pri_len);
static int write_client_reg(unsigned char *reg, int reg_len);

//static int read_file(const char *sz_file, unsigned char *buf, int *buf_len);
//static int write_file(const char *sz_file, unsigned char *buf,int buf_len);

    int
main( int argc, char **argv )
{
    int ret = 0;
    int len = 0;
    unsigned char *reg_data;
    int reg_len = 0; 
    const int hd_info_len = 256;
    unsigned char hardware_info[hd_info_len];


    system("mkdir -p /etc/reach");

    //get hardware info 
    memset(hardware_info, 0 , hd_info_len);
    ret =  rsa_read_hardware_info(hardware_info, hd_info_len);
    if( ret != 0) {
        printf("read hardware info failed!\n");
        return -1;
    }
    printf("hardware_info: %s\n", (char *)hardware_info);

    // generate client device public-private key pair
    unsigned char pub_key[1024];
    unsigned char pri_key[1024];
    int pub_len = 1024, pri_len = 1024;

    ret = rsa_gen_key_pair(pub_key, &pub_len , pri_key, &pri_len);
    if( ret != 0) {
        printf("gen key pair failed!\n");
        return -1;
    }
    
    //write client private key file
    ret = write_pri_key(pri_key, pri_len);
    if( ret != 0) {
        printf("write client private key file failed!\n");
        return -1;
    }

    //write hardware info and client public key to  client register file
    reg_len = sizeof(int) * 2 + pub_len + hd_info_len;
    reg_data = calloc(reg_len, sizeof(char));
    if( reg_data == NULL) {
        printf("alloc register buf failed!\n");
        return -1;
    }

    memcpy(reg_data, &hd_info_len , sizeof(int));
    memcpy(reg_data + sizeof(int) , hardware_info, hd_info_len);
    memcpy(reg_data + hd_info_len + sizeof(int) , &pub_len, sizeof(int));
    memcpy(reg_data + hd_info_len + sizeof(int) * 2, pub_key, pub_len);

    write_client_reg(reg_data, reg_len);

    free(reg_data);

    return 0;
}


static int write_client_reg(unsigned char *reg, int reg_len)
{
    FILE * fp = NULL;
    int ret = 0;
    unsigned char *pub_key = NULL;
    int  pub_len = 0;
    unsigned char *out_data = NULL;
    int  out_len = 0;

    // read public key 
    ret = rsa_read_file(R_S_PUBLIC_KEY_FILE, pub_key, &pub_len);
    if( ret == -1)
        return -1;

    pub_key = calloc(pub_len, sizeof(char));
    if (pub_key == NULL) {
        printf("malloc failed!\n");
        return -1;
    }

    //ret = rsa_read_file(R_S_PUBLIC_KEY_FILE, pub_key, &pub_len);
    ret = rsa_read_file(R_S_PUBLIC_KEY_FILE, pub_key, &pub_len);
    if( ret == -1) {
        printf("read %s data failed!\n", R_S_PUBLIC_KEY_FILE);
        goto free_memory;
    }

    // use public key encrypt
    out_len = 4 * reg_len ; // in = 128 - 11, out = 128
    out_data = calloc(out_len , sizeof(char));
    if( out_data == NULL) {
        printf("malloc encrypt buffer failed!\n");
        goto free_memory;
    }

    ret = rsa_public_encrypt(reg, reg_len, out_data, &out_len 
            ,pub_key, pub_len);
    if( ret == -1) {
        printf("rsa public encrypt failed!\n");
        goto free_memory;
    }

    // write encrypted data to file
    ret = rsa_write_file(R_C_REG_FILE, out_data, out_len);
    if( ret == -1){
        printf("write %s fialed!\n", R_C_REG_FILE);
        goto free_memory;
    }
    
free_memory:
    if(pub_key)
        free(pub_key);
    if(out_data)
        free(out_data);

    return 0;
}

static int write_pri_key(unsigned char *pri_key, int pri_len)
{
    return rsa_write_file(R_C_PRIVATE_KEY_FILE, pri_key, pri_len);
}

/*
static int read_file(const char *sz_file, unsigned char *buf, int *buf_len)
{
    FILE *fp = NULL;
    int   ret = 0 , len = 0;

    fp = fopen(sz_file, "rb");
    if( fp == NULL) {
        printf("open %s file failed!\n", sz_file);
        return -1;
    }

    ret = fread(&len, sizeof(char), sizeof(int), fp);
    if( ret != sizeof(int)) {
        printf("read file data len failed!\n");
        fclose(fp);
        return -1;
    }
    
    //just get buf length
    if( buf == NULL) {
        *buf_len = len;
        fclose(fp);
        return 0;
    }

    if( len > *buf_len) {
        printf("read file data , buf is too small!\n");
        fclose(fp);
        return -1;
    }
    *buf_len = len;

    len  =  fread(buf, sizeof(char), len, fp);
    fclose(fp);

    if( len != *buf_len ){
        printf("read file data failed!");
        return -1;
    }

    return 0;
}
*/
/*
static int rsa_write_file(const char *sz_file, unsigned char *buf,int buf_len)
{
    FILE *fp = NULL;

    fp = fopen(sz_file,"wb");
    if( fp == NULL) {
        printf("open %s failed!", sz_file);
        return -1;
    }

    int len = fwrite(&buf_len, sizeof(char), sizeof(int), fp);
    if( len != sizeof(int)) {
        printf("write buf_len failed!");
        fclose(fp);
        return -1;
    }

    len = fwrite(buf, sizeof(char), buf_len, fp);
    if( len != buf_len) {
        printf("write buf failed!");
        fclose(fp);
        return -1;
    }
    
    fclose(fp);
    return 0;
}
*/

