#ifndef APP_SIGNAL_H
#define APP_SIGNAL_H
#include <mcfw/src_linux/devices/inc/device.h>




#define HV_TABLE_D_NAME         "/usr/local/reach/.config/digital.ini"
#define HV_TABLE_A_NAME         "/usr/local/reach/.config/analog.ini"
#define INPUT_CONFIG   			"/usr/local/reach/dvr_rdk/ti816x/.config/input.dat"
#define VIDEO_CONFIG            "/usr/local/reach/dvr_rdk/ti816x/.config/video.dat"



/*Æ«ÒÆÁ¿*/
typedef struct __HV__ {
	short	hporch;
	short	vporch;
} HV;

typedef struct __HV_Table__ {
	HV	analog[DEVICE_STD_REACH_LAST];
	HV	digital[DEVICE_STD_REACH_LAST];
} HVTable;

void init_HV_table(void);
int get_HV_table(HVTable* hv_table);
int write_HV_table(HVTable *pNew, int digital);


int get_signal_default_hv(int mode_id, int *h, int *v);
int get_signal_analog_base_hv(int mode_id, int *h, int *v);
char *get_signal_analog_name(int mode_id);

int app_init_fpga_version(void);
int app_get_fpga_version(void);
int init_kernel_version(void);
char *get_kernel_version(void);

#endif

