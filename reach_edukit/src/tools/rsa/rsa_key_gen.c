#include <stdio.h>
#include <stdlib.h>
#include "rsa.h"


static int write_key(const char *file_name, unsigned char *key ,int key_len);

    int
main( int argc, char **argv )
{
    int ret = 0;
    unsigned char pub_key[1024];
    unsigned char pri_key[1024];
    int pub_len = 1024, pri_len = 1024;

    ret = rsa_gen_key_pair( pub_key, &pub_len, pri_key, &pri_len);
    if(ret != 0){
        fprintf(stderr,"gen key pair failed!\n");
        return ret;
    }

    printf("pub key len = %d , pri key len = %d\n ",pub_len, pri_len);

    if( rsa_write_file(S_PUBLIC_KEY_FILE, pub_key, pub_len) < 0){
        printf("write %s failed!\n", S_PUBLIC_KEY_FILE);
    }

    if( rsa_write_file(S_PRIVATE_KEY_FILE, pri_key, pri_len) < 0){
        printf("write %s failed!\n", S_PRIVATE_KEY_FILE);
    }

    return 0;
}


