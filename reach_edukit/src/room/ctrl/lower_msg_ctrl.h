#ifndef __LOWER_MSG_CTRL_INCLUDE__
#define __LOWER_MSG_CTRL_INCLUDE__

#include "xml_base.h"

enum {
	ROOMMSGID = 0,
	LIVEMSGID,
	RECOMSGID,
	MSGIDNUM
};

typedef struct LowerMsgEnv {
	int32_t msg_id[MSGIDNUM];
} LowerMsgEnv;

void *lower_msg_process(void *arg);

int32_t lower_msg_get_platfrom_key(parse_xml_t *parseXml);

#endif /*__LOWER_MSG_CTRL_INCLUDE__*/

