#include "nslog.h"
#include <errno.h>

static int nslog_mk_rules_file(char *rule_file, char *rule_cname, char *out)
{
	if(!rule_file || !rule_cname) {
		printf("error : rule_file = %p, rule_cname = %p\n", rule_file, rule_cname);
		return -1;
	}

	FILE *fp = NULL;

	fp =	fopen(rule_file, "w+");

	if(NULL == fp) {
		printf("fopen  : %s\n", strerror(errno));
		return -1;
	}

	fprintf(fp, \
	        "[global]\n"\
	        "default format = \"%%D.%%ms%%V [%%t:%%p:%%f:%%U:%%L] %%m%%n\"\n"\
	        "[levels]\n"\
	        "INFO = 40, LOG_INFO\n"\
	        "[rules]\n"\
	        "%s.DEBUG %s\n", \
	        rule_cname, out);
	fclose(fp);
	printf("[nslog_mk_rules_file] end!!!\n");
	system("sync");
	return 0;
}


int NslogInit(nslog_conf_info_t *info)
{
	int rc = 0;

	rc = dzlog_init(info->conf, info->cname);

	if(rc) {
		printf("dzlog_init failed : <%s><%s>\n", info->conf, info->cname);
		nslog_mk_rules_file(info->conf, info->cname, info->output);
		rc = dzlog_init(info->conf, info->cname);

		if(rc) {
			printf("dzlog_init failed !!!<%s><%s>\n", info->conf, info->cname);
			return -1;
		}

		printf("dzlog_init [%s]  File Not Found! Please restart the application !\n", info->conf);

		printf("[global]\n"\
		       "default format = \"%%D.%%ms%%V [%%t:%%p:%%f:%%U:%%L] %%m%%n\"\n"\
		       "[levels]\n"\
		       "INFO = 40, LOG_INFO\n"\
		       "[rules]\n"\
		       "%s.DEBUG %s\n", \
		       info->cname, info->output);
		return -1;
	}

	return 0;
}

