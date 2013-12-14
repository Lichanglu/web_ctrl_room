#ifndef _REACH_DEBUG_H__
#define _REACH_DEBUG_H__


//…Ë÷√µ˜ ‘malloc
#define r_malloc(s) debug_malloc(s,__FILE__,__LINE__,__FUNCTION__)
#define r_free(p) do{ \
	printf("%s:%d:%s:free(0x%1x)\n",__FILE__,__LINE__,__FUNCTION__,(unsigned long )p);\
	free(p); \
}while(0)


#endif