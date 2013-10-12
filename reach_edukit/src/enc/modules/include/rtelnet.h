#ifndef __LCS_TELNET
#define __LCS_TELNET

typedef int (*func)(int type, char *in, int inlen, char *out, int outlen);

int rtelnet_set_parse_func(func func);
int rtelnet_init(char *rtelnet_ip, int port);
void rtelnet_uninit(void);
int rtelnet_log_output(char *log, int loglen);

#endif
