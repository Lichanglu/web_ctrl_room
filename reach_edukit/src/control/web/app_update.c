#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <sys/types.h>
#include <linux/types.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <sys/types.h>
#include <linux/rtc.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <pthread.h>
#include <errno.h>

//#include "app_gpio.h"
#include "app_update.h"
//#include "log_common.h"
//#include "app_head.h"
//#include "MMP_control.h"
//#include "sysparams.h"

#include "app_update_header.h"

#define KERNELFILENAME		"/opt/dvr_rdk/ti816x_2.8/update/uImage"
#define FPGAFILENAME		"/opt/dvr_rdk/ti816x_2.8/update/fpga.bin"


#define FLASH_ENABLE	41
#define FPGA_PRO	2
#define FPGA_DONE	3


typedef struct _R_GPIO_data_ {
	unsigned int gpio_num;
	unsigned int gpio_value;
} R_GPIO_data;



#define SET_GPIO	(0x55555555)
#define GET_GPIO	(0xAAAAAAAA)
#define PRINTF printf

static int SetGPIOBitValue(int fd, int bit);
static int ClearGPIOBitValue(int fd, int bit);
static int GetGPIOBitValue(int fd, int bit);
static int set_gpio_bit(int bit, int fd);
static int clear_gpio_bit(int bit, int fd);
static int get_gpio_bit(int bit, int fd);
static int set_is_update_fpga(int status);








/*set GPIO port Bit value*/
static int SetGPIOBitValue(int fd, int bit)
{
	R_GPIO_data op;
	int ret ;

	op.gpio_value = 1 ;
	op.gpio_num = bit;
	ret = ioctl(fd, SET_GPIO, &op);

	if(ret < 0)	{
		return -1 ;
	}

	return 0;
}

/*clear GPIO port Bit value*/
static int ClearGPIOBitValue(int fd, int bit)
{
	R_GPIO_data op;
	int ret ;

	op.gpio_value = 0 ;
	op.gpio_num = bit;
	ret = ioctl(fd, SET_GPIO, &op);

	if(ret < 0)	{
		return -1 ;
	}

	return 0;
}
/*get GPIO port Bit value*/
static int GetGPIOBitValue(int fd, int bit)
{

	R_GPIO_data op;
	int ret , val = -1 ;

	op.gpio_value = 0 ;
	op.gpio_num = bit;
	ret = ioctl(fd,	GET_GPIO, &op);

	if(ret < 0)	{
		return -1 ;
	}

	val = op.gpio_value;
	return (val);
}





static int set_gpio_bit(int bit, int fd)
{
	int ret = 0;
	ret = SetGPIOBitValue(fd, bit);

	if(ret < 0) {
		PRINTF("set_gpio_bit() failed!!!\n ");
		return -1 ;
	}

	PRINTF("balance mode switch !!!!bit=%d\n", bit);
	return 0;
}



static int clear_gpio_bit(int bit, int fd)
{
	int ret = 0;
	ret = ClearGPIOBitValue(fd, bit);

	if(ret < 0) {
		PRINTF("clear_gpio_bit() failed!!!\n ");
		return -1 ;
	}

	return 0;
}

static int get_gpio_bit(int bit, int fd)
{
	int ret =  -1;
	ret = GetGPIOBitValue(fd, bit);

	if(ret < 0) {
		PRINTF("GetGPIOBitValue() failed!!!\n ");
		return -1 ;
	}

	return (ret);
}






#define PER_READ_LEN  256
/*升级FPGA程序*/
static int updateFpgaProgram(const char *fpgafile, int fd)
{
	int ret = 0;
	char spidata[PER_READ_LEN];
	int spifd = -1;
	FILE *fpgafd = NULL;
	int readlen = 0;
	int writelen = 0;
	int totalwritelen  = 0;

	PRINTF("Enter into update FPGA Program \n");

	//	ret = set_gpio_bit(41, fd); //写保护

	ret = clear_gpio_bit(FLASH_ENABLE, fd);
	PRINTF("FLASH_ENABLE ret=%x \n", ret);

	system("flash_eraseall /dev/mtd0");
	spifd =  open("/dev/mtd0", O_RDWR, 0);

	if(spifd < 0)	{
		PRINTF("open the SPI flash Failed \n");
		ret = -1;
		goto cleanup;
	}

	fpgafd = fopen(fpgafile, "r+b");

	if(fpgafd == NULL)	{
		PRINTF("open the FPGA bin Failed \n");
		ret = -1;
		goto cleanup;
	}

	rewind(fpgafd);

	while(1) {
		
		readlen = fread(spidata, 1, PER_READ_LEN, fpgafd);

		if(readlen < 1)	{
			PRINTF("file read end \n");
			break;
		}

		writelen = write(spifd, spidata, readlen);
		//		PRINTF("writelen = %d \n", writelen);
		totalwritelen += writelen;

		if(feof(fpgafd)) {
			PRINTF("writelen = %d \n", writelen);
			writelen = write(spifd, spidata, readlen);
			break;
		}
	}

	close(spifd);
	spifd  = -1;
	PRINTF("totalwritelen = %d \n", totalwritelen);
cleanup:
	PRINTF("002 flash_eraseall updateFpgaProgram \n");
	ret = set_gpio_bit(FLASH_ENABLE, fd);
	PRINTF("FLASH_ENABLE ret=%x \n", ret);

	if(spifd > 0) {
		close(spifd);
	}

	if(fpgafd) {
		fclose(fpgafd);
	}

	ret = get_gpio_bit(FPGA_DONE, fd);

	while(!ret) {
		ret = get_gpio_bit(FPGA_DONE, fd);
	}

	ret = clear_gpio_bit(FPGA_PRO, fd);
	ret = set_gpio_bit(FPGA_PRO, fd);
	PRINTF("FPGA_PRO ret2=%x \n", ret);
	PRINTF("gpio done   bit = %d\n", ret);
	return ret;
}
extern int32_t gpio_fd;

int updatefpga()
{
	int ret = 0;
	char filepath[256] = {0};
	int 	fd = gpio_fd;
	/*更新FPGA*/
	sprintf(filepath, "%s", FPGAFILENAME);

	//	sprintf(filepath,"/opt/dvr_rdk/ti816x_2.8/bin/enc2000.bin");
	if(access(filepath, F_OK) == 0) {
		set_is_update_fpga(1);
		sleep(1);
		PRINTF("----begin to update fpga---\n");
		ret = updateFpgaProgram(filepath, fd);
		PRINTF("----end to update fpga---\n");
	} else {
		PRINTF("have no file \n");
	}

	return ret;
}

static int g_fpga_status = 0;
int get_is_update_fpga()
{
	return g_fpga_status;
}
static int set_is_update_fpga(int status)
{
	g_fpga_status = status;
	return 0;
}


