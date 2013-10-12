#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sys_info.h"
#include "nslog.h"

int g_net_occupancy_rate = -1;
int g_cpu_occupancy_rate = -1;
int g_mem_occupancy_rate = -1;

int run_cpu()
{
	FILE *fp = NULL;
	char *szLine = NULL;
	size_t len = 0;
	int cpu_percent_used = 0;

	unsigned long long cpu_total = 0;
	unsigned long long cpu_total_used = 0;
	unsigned long long pre_cpu_total = 0;
	unsigned long long pre_cpu_total_used = 0;

	unsigned long long normal_user = 0;
	unsigned long long nice_user = 0;
	unsigned long long system = 0;
	unsigned long long idle = 0;
	unsigned long long iowait = 0;
	unsigned long long irq = 0;
	unsigned long long softirq = 0;

	fp = fopen("/proc/stat", "r");

	if(NULL ==  fp) {
		nslog(NS_INFO, "[%s]---run_cpu: open /proc/stat error", __func__);
		return -1;
	}

	//fgets(char *buf，int len，stdin);
	if(getline(&szLine, &len, fp) < 0) {
		nslog(NS_INFO, "[%s]---run_cpu: read /proc/stat error", __func__);
		fclose(fp);
		return -1;
	}

	sscanf(szLine, "cpu %Lu %Lu %Lu %Lu %Lu %Lu %Lu",
	       &normal_user, &nice_user,
	       &system, &idle,
	       &iowait, &irq, &softirq);

	pre_cpu_total_used = normal_user + nice_user + system + iowait + irq + softirq;
	pre_cpu_total = idle + pre_cpu_total_used;
	//	nslog(NS_INFO, "[%s]---idle is  -- [%Lu]",__func__,idle);
	//	nslog(NS_INFO, "pre_cpu_total_used is --[%Lu]  pre_cpu_total is --[%Lu]\n",pre_cpu_total_used,pre_cpu_total);

	if(szLine) {
		free(szLine);
	}

	if(fp) {
		fclose(fp);
	}

	fp = NULL;
	szLine = NULL;
	len = 0;

	usleep(100000);
	normal_user = 0;
	nice_user = 0;
	system = 0;
	idle = 0;
	iowait = 0;
	irq = 0;
	softirq = 0;

	fp = fopen("/proc/stat", "r");

	if(NULL == fp) {
		nslog(NS_INFO, "[%s]---run_cpu: open /proc/stat error", __func__);
		return -1;
	}

	if(getline(&szLine, &len, fp) < 0) {
		nslog(NS_INFO, "[%s]---run_cpu: read /proc/stat error", __func__);
		fclose(fp);
		return -1;
	}

	sscanf(szLine, "cpu %Lu %Lu %Lu %Lu %Lu %Lu %Lu",
	       &normal_user, &nice_user,
	       &system, &idle,
	       &iowait, &irq, &softirq);

	cpu_total_used = normal_user + nice_user + system + iowait + irq + softirq;
	cpu_total = idle + cpu_total_used;
	//	nslog(NS_INFO, "[%s]---idle is  -- [%Lu]",__func__,idle);
	//	nslog(NS_INFO, "[%s]---cpu_total_used is --[%Lu]  cpu_total is --[%Lu]\n",__func__,cpu_total_used,cpu_total);
	//计算CPU使用率，这是前一个时间间隔内的平均使用率
	cpu_percent_used = (int)ceil(100 * ((float)(cpu_total_used - pre_cpu_total_used)) / (cpu_total - pre_cpu_total));

	if(szLine) {
		free(szLine);
	}

	if(fp) {
		fclose(fp);
	}

	//	nslog(NS_INFO, "[%s]---[THE cup used]is ---[%d]\n",__func__,cpu_percent_used);
	return cpu_percent_used;

}
//获取内存使用信息
int run_memory()
{
	int mem_percent_used = 0;
	int mem_total = 0;
	int mem_free = 0;
	FILE *fp = NULL;
	char *szLine = NULL;
	size_t len = 0;
	int nTotal = 0;
	int nFree = 0;
	int nBuffers = 0;
	int nCached = 0;
	int nSwapCached = 0;
	fp = fopen("/proc/meminfo", "r");

	if(NULL == fp) {
		nslog(NS_INFO, "[%s]---run_memory: open /proc/meminfo error", __func__);
		return -1;
	}

	while(!feof(fp)) {
		if(szLine) {
			free(szLine);
			szLine = NULL;
		}

		if(getline(&szLine, &len, fp) < 0) {
			if(feof(fp)) {
				continue;
			}

			nslog(NS_INFO, "[%s]---run_memory: read /proc/meminfo error", __func__);
			fclose(fp);
			return -1;
		}

		if(strncmp(szLine, "MemTotal", strlen("MemTotal")) == 0) {
			sscanf(szLine, "MemTotal: %d", &nTotal);
		} else if(strncmp(szLine, "MemFree", strlen("MemFree")) == 0) {
			sscanf(szLine, "MemFree: %d", &nFree);
		} else if(strncmp(szLine, "Buffers", strlen("Buffers")) == 0) {
			sscanf(szLine, "Buffers: %d", &nBuffers);
		} else if(strncmp(szLine, "Cached", strlen("Cached")) == 0) {
			sscanf(szLine, "Cached: %d", &nCached);
		} else if(strncmp(szLine, "SwapCached", strlen("SwapCached")) == 0) {
			sscanf(szLine, "SwapCached: %d", &nSwapCached);
			break;
		} else {
			continue;
		}
	}

	//更新
	mem_total = nTotal;
	mem_free = nFree + nBuffers + nCached + nSwapCached;
	mem_percent_used = (int)ceil(100 * ((float)(mem_total - mem_free) / mem_total));

	//	nslog(NS_INFO, "[%s]---mem_free is -- [%d]\n",__func__,mem_free);
	if(szLine) {
		free(szLine);
	}

	if(fp) {
		fclose(fp);
	}

	return mem_percent_used;
}

int run_throughput(char *eth_name,  RX_TX_bytes_t *rt)
{
	FILE *fp = NULL;
	char *szLine = NULL;
	size_t len = 0;
	unsigned long long nouse = 0;
	int eth_name_len = strlen(eth_name);
	memset(rt, 0, sizeof(RX_TX_bytes_t));

	fp = fopen("/proc/net/dev", "r");

	if(NULL == fp) {
		nslog(NS_INFO, "[%s]---run_throughput: open /proc/net/dev error", __func__);
		return -1;
	}

	while(!feof(fp)) {
		char *p = NULL;

		if(szLine) {
			free(szLine);
			szLine = NULL;
		}

		if(getline(&szLine, &len, fp) < 0) {
			if(feof(fp)) {
				continue;
			}

			nslog(NS_INFO, "[%s]---run_throughput: read /proc/net/dev error", __func__);
			fclose(fp);
			return -1;
		}

		//??????п?????
		p = szLine;

		while(*p && (*p == ' ')) {
			p++;
		}

		if(strncmp(p, eth_name, eth_name_len) == 0) {
			sscanf(p + eth_name_len, ":%Lu %Lu %Lu %Lu %Lu %Lu %Lu %Lu %Lu",
			       &(rt->RX_bytes),
			       &nouse, &nouse, &nouse, &nouse, &nouse, &nouse, &nouse,
			       &(rt->TX_bytes));
		}

#if 0
		else if(strncmp(p, eth_name, strlen(eth_name)) == 0) {
			sscanf(p, "eth1:%Lu %Lu %Lu %Lu %Lu %Lu %Lu %Lu %Lu",
			       &nEth1In,
			       &nouse, &nouse, &nouse, &nouse, &nouse, &nouse, &nouse,
			       &nEth1Out);
			break;
		}

#endif
		else {
			continue;
		}
	}

	//cpu_net_num = nEth0In+nEth0Out+nEth1In+nEth1Out;
	//      nslog(NS_INFO, "nEth0In is --[%Lu]  nEth0Out is --[%Lu]\n",nEth0In,nEth0Out);
	if(szLine) {
		free(szLine);
	}

	if(fp) {
		fclose(fp);
	}

	return 0;
}
#if 1
int get_bandwidth(void)
{
	return 125;
}
int run_throughput_boost(void)
{
	int cpu_net_used = -1;
	int bandwidth = get_bandwidth();
	double tx_per;
	RX_TX_bytes_t rt1, rt2;

	if(-1 == run_throughput("eth0", &rt1)) {
		return cpu_net_used;
	}

	usleep(1000000);

	if(-1 == run_throughput("eth0", &rt2)) {
		return cpu_net_used;
	}

	tx_per = (1000 * (rt2.TX_bytes - rt1.TX_bytes)) / (bandwidth * 1024 * 1024);
	cpu_net_used = (int)ceil(tx_per);
#if SYS_INFO
	nslog(NS_INFO, "rb2 - rb1 : [%Lu], tx_per : [%lf] cpu_net_used : [%d]\n", (rt2.TX_bytes - rt1.TX_bytes), tx_per, cpu_net_used);
#endif
	return cpu_net_used;
}

#endif


void *create_cpu_occupancy_rate_thread(void *arg)
{
	while(1) {
		g_cpu_occupancy_rate = run_cpu();

		if(g_cpu_occupancy_rate == -1) {
			nslog(NS_INFO, "[%s]---[run_throughput_boost]---run_throughput_boost is error!\n", __func__);
			break;
		}

		//            nslog(NS_INFO, "[%s]---[g_cpu_occupancy_rate] is--- [%d]!\n",__func__,g_net_occupancy_rate);
	}

	return NULL;
}

int get_cpu_occupancy_rate(void)
{
	return g_cpu_occupancy_rate;
	//      nslog(NS_INFO, "[%s]---[g_net_occupancy_rate] is--- [%d]!\n",__func__,g_net_occupancy_rate);
}


void *create_mem_occupancy_rate_thread(void *arg)
{
	while(1) {
		g_mem_occupancy_rate = run_memory();

		if(g_mem_occupancy_rate == -1) {
			nslog(NS_INFO, "[%s]---[run_throughput_boost]---run_throughput_boost is error!\n", __func__);
			break;
		}

		//              nslog(NS_INFO, "[%s]---[g_mem_occupancy_rate] is--- [%d]!\n",__func__,g_net_occupancy_rate);
		sleep(1);
	}

	return NULL;
}

int get_mem_occupancy_rate(void)
{
	return g_mem_occupancy_rate;
	//      nslog(NS_INFO, "[%s]---[g_net_occupancy_rate] is--- [%d]!\n",__func__,g_net_occupancy_rate);
}


void *create_net_occupancy_rate_thread(void *arg)
{
	while(1) {
		g_net_occupancy_rate = run_throughput_boost();

		if(g_net_occupancy_rate == -1) {
			nslog(NS_INFO, "[%s]---[run_throughput_boost]---run_throughput_boost is error!\n", __func__);
			break;
		}

		//            nslog(NS_INFO, "[%s]---[g_net_occupancy_rate] is--- [%d]!\n",__func__,g_net_occupancy_rate);
	}

	return NULL;
}

int get_net_occupancy_rate(void)
{
	return g_net_occupancy_rate;
	//      nslog(NS_INFO, "[%s]---[g_net_occupancy_rate] is--- [%d]!\n",__func__,g_net_occupancy_rate);
}


#if 0
int main()
{
	int cpu_used = 0;
	int cpu_net = 0 ;
	int cpu_mem = 0;
	cpu_used = run_cpu();
	cpu_net = run_throughput_boost();
	cpu_mem = run_memory();

	nslog(NS_INFO, "cpu_used is --[%d] cpu_net  is --[%d] cpu_mem  is --[%d]\n", cpu_used, cpu_net, cpu_mem);

	return 0;
}

#endif

