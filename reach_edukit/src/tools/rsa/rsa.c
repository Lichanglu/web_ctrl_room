#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <openssl/bn.h>
#include <openssl/rsa.h>
#include <openssl/err.h>
#include "rsa.h"


static int rsa_encrypt(unsigned char *in_data, int in_len
                        ,unsigned char *out_data, int *out_len
                        ,unsigned char *key, int key_len, int is_pub);

static int     rsa_decrypt(unsigned char *in_data, int in_len
                     ,unsigned char *out_data, int *out_len
                     ,unsigned char *key, int key_len, int is_pub);

//******************************************************************************
int     rsa_gen_key_pair(unsigned char *pub_key, int *pub_len 
                           ,unsigned char *pri_key, int *pri_len )
{
    int     ret = 0;
    RSA*    rsa = NULL;
    
    if(!pub_key || *pub_len < 1024 || !pri_key ||  *pri_len < 1024) {
        fprintf(stderr, "rsa generate key pari failed! invalidate param!\n");
        return -1;
    }

    rsa = RSA_generate_key(1024, 3, NULL, NULL);
    if( NULL == rsa ) {
        ret = ERR_get_error();
        fprintf(stderr, "RSA_generate_key failed!, err = %d\n", ret);
        return ret;
    }

    memset(pub_key, 0 , 1024);
    *pub_len = i2d_RSAPublicKey(rsa , &pub_key);

    memset(pri_key, 0, 1024);
    *pri_len = i2d_RSAPrivateKey(rsa, &pri_key);

    RSA_free(rsa);
    
    return ret;
}

//******************************************************************************
int     rsa_public_encrypt(unsigned char *in_data, int in_len
                           ,unsigned char *out_data, int *out_len
                           ,unsigned char *pub_key, int pub_len)
{
    return rsa_encrypt(in_data, in_len, out_data, out_len
                ,pub_key, pub_len, 1);
}

int     rsa_private_decrypt(unsigned char *in_data, int in_len
                             ,unsigned char *out_data, int *out_len
                             ,unsigned char *pri_key, int pri_len)
{
    return rsa_decrypt(in_data, in_len ,out_data, out_len
                ,pri_key, pri_len, 0);
}

//******************************************************************************
int     rsa_private_encrypt(unsigned char *in_data, int in_len
                            ,unsigned char *out_data, int *out_len
                            ,unsigned char *pri_key, int pri_len)
{
    return rsa_encrypt(in_data, in_len, out_data, out_len
                ,pri_key, pri_len, 0);
}

int     rsa_public_decrypt(unsigned char *in_data, int in_len
                             ,unsigned char *out_data, int *out_len
                             ,unsigned char *pub_key, int pub_len)
{
    return rsa_decrypt(in_data, in_len ,out_data, out_len
                ,pub_key, pub_len, 1);
}

//******************************************************************************
static int rsa_encrypt(unsigned char *in_data, int in_len
                        ,unsigned char *out_data, int *out_len
                        ,unsigned char *key, int key_len, int is_pub)
{
    int    ret = 0;
    RSA     *rsa = NULL;

    if( !key || key_len <= 0 ) {
        fprintf(stderr, "key or key_len invalidate!\n");
        return -1;
    }

    if( is_pub )
        rsa = d2i_RSAPublicKey(NULL, (const unsigned char **)&key, key_len);
    else
        rsa = d2i_RSAPrivateKey(NULL, (const unsigned char **)&key, key_len);

    if( NULL == rsa ){
        ret = ERR_get_error();
        fprintf(stderr,"key data invalidate! err = %d\n", ret);
        return ret;
    }

    int rsa_size = RSA_size(rsa);
    if ( !in_data || !out_data || in_len <= 0 || *out_len < 128 
            || ( in_len * (rsa_size - 11)  > (*out_len) * rsa_size )) {
        fprintf(stderr, "rsa generate key pari failed! invalidate param!\n");
        RSA_free(rsa);
        return -1;
    }

    memset(out_data, 0, *out_len);

    int off = 0, enc_len = 0;
    while( off < in_len ) {
        int flen = 0, r = 0;
        int left_len = in_len - off;
        if( left_len >= rsa_size - 11)
            flen = rsa_size - 11;
        else
            flen = left_len;
        
        if( is_pub ) {
            r = RSA_public_encrypt(flen , in_data + off, out_data + enc_len
                , rsa , RSA_PKCS1_PADDING);
        } else {
            r = RSA_private_encrypt(flen , in_data + off, out_data + enc_len
                , rsa , RSA_PKCS1_PADDING);
        }
        if( r == -1 ) {
            ret = ERR_get_error();
            fprintf(stderr, "RSA_public_encrypt failed! err = %d\n", ret);
            RSA_free(rsa);
            return ret;
        }

        enc_len += r;
        off += flen;
    }

    RSA_free(rsa);
    *out_len = enc_len;

    return ret;
 }
    
static int     rsa_decrypt(unsigned char *in_data, int in_len
                     ,unsigned char *out_data, int *out_len
                     ,unsigned char *key, int key_len, int is_pub)
{
    int    ret = 0;
    RSA    *rsa = NULL;

    if( !key || key_len <= 0) {
        fprintf(stderr, "key or key len invalidate!\n");
        return -1;
    }

    if( is_pub ) {
        rsa = d2i_RSAPublicKey(NULL, (const unsigned char **)&key , key_len);
    } else {
        rsa = d2i_RSAPrivateKey(NULL, (const unsigned char **)&key , key_len);
    }
    
    if( rsa == NULL) {
        ret = ERR_get_error();
        fprintf(stderr, "key data invalidate, err = %d\n", ret);
        return ret;
    }

    int rsa_size = RSA_size(rsa);
    if( !in_data || in_len < rsa_size || !out_data || *out_len <= rsa_size - 11
            || (in_len * (rsa_size - 11) > (*out_len) * rsa_size) ) {
        fprintf(stderr, "rsa_decrypt: invalidate paramter! \n ");
        RSA_free(rsa);
        return -1;
    }

    memset( out_data, 0, *out_len);

    int dec_len = 0, off = 0;
    while( in_len - off > 0) {
        int r = 0;
        if( is_pub ) {
            r = RSA_public_decrypt(rsa_size , in_data + off, out_data + dec_len
                    , rsa , RSA_PKCS1_PADDING);
        } else {
            r = RSA_private_decrypt(rsa_size , in_data + off, out_data + dec_len
                    , rsa , RSA_PKCS1_PADDING);
        }
        if( r == -1 ) {
            ret = ERR_get_error();
            fprintf(stderr, "RSA_decrypt failed! err = %d \n", ret);
            RSA_free(rsa);
            return ret;
        }
        
        dec_len += r;
        off += rsa_size;
    }

    RSA_free(rsa);

    *out_len = dec_len;    
    return ret;
}

int rsa_read_file(const char *sz_file, unsigned char *buf, int *buf_len)
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
        printf("buf len = %d \n" , len);
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

int rsa_write_file(const char *sz_file, unsigned char *buf,int buf_len)
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

int rsa_read_hardware_info(unsigned char *hardware_info, int hd_info_len)
{
    int ret = 0;

    ret = system("rm -f /tmp/sn.txt");
    if (ret == -1) {
    	printf("rm tmp file failed!\n");
    } else {
    	printf("rm tmp file success!\n");
    }

    // exec dmidecode and grep bios serial number
    ret = system("dmidecode -t 1 | grep \"Serial Number\" > /tmp/sn.txt");
    if(ret == -1) {
        printf("exec dmidecode failed!\n");
        return -1;
    } else {
        printf("exec dmidecode success!\n");
    }

    ret = system("cat /etc/mtab | grep ext | awk \'END {print $1}\' | xargs smartctl -i  | grep \"Serial Number\" |awk \'{print $3}\' >> /tmp/sn.txt");
    if(ret == -1) {
        printf("exec fdisk sn failed!\n");
        return -1;
    } else {
        printf("exec fdisk sn success!\n");
    }

    ret = system("fdisk -l | grep \"identifier\" | awk \'{print $3}\' >> /tmp/sn.txt");
    if(ret == -1) {
        printf("exec fdisk identifier failed!\n");
        return -1;
    } else {
        printf("exec fdisk identifier success!\n");
    }

    ret = system("ifconfig eth0 | grep HWaddr |awk \'{print $5;}\' >> /tmp/sn.txt");
    if(ret == -1) {
        printf("exec ifconfig failed!\n");
        return -1;
    } else {
        printf("exec ifconfig success!\n");
    }

    FILE *fp = NULL;
    fp = fopen("/tmp/sn.txt", "rb");
    if( fp == NULL){
        printf("open hardware_info file failed!\n");
        return -1;
    }

    int  len = fread(hardware_info, sizeof(char), hd_info_len, fp);
    fclose(fp);

    ret = system("rm -f /tmp/sn.txt");
    if (ret == -1) {
    	printf("rm tmp file failed!\n");
    } else {
    	printf("rm tmp file success!\n");
    }

    if( len <= 0){
        printf("read hardware_info file failed!\n");
        return -1;
    }

    return 0;
}


