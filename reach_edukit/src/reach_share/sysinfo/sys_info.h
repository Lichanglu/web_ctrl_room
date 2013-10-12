#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/resource.h>
#include <unistd.h>
#include <fcntl.h>
#include <syslog.h>
#include <time.h>
#include <math.h>

typedef struct RX_TX_bytes
{
	unsigned long long RX_bytes;
	unsigned long long TX_bytes;
} RX_TX_bytes_t;


int run_cpu();
int run_memory();
int run_throughput(char *eth_name,  RX_TX_bytes_t *rt);
int run_throughput_boost(void);

void* create_cpu_occupancy_rate_thread(void* arg);
int get_cpu_occupancy_rate(void);
void* create_mem_occupancy_rate_thread(void* arg);
int get_mem_occupancy_rate(void);
void* create_net_occupancy_rate_thread(void* arg);
int get_net_occupancy_rate(void);



