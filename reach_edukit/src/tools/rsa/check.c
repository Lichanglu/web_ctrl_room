#include <stdio.h>
#include <stdlib.h>

#include "cli_verify.h"

int main(int argc, char **argv)
{
    char *pri_data = NULL;

    /*
     * 参数在里面分配空间，需在外面析放.
     * 成功返回私有数据指针，失败返回NULL
     */

    pri_data = check_hw_and_get_pri_data(&pri_data);
    if (pri_data) {
        printf("pri_data = %s", pri_data);
    } else {
    	printf("check error\n");
    }

    if (pri_data)
	free(pri_data);

    return 0;
}
