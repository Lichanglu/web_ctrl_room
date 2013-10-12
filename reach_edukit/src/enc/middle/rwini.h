#ifndef _RWINI_H__
#define _RWINI_H__
int  ConfigSetKey(void *CFG_file, void *section, void *key, void *buf);
int  ConfigGetKey(void *CFG_file, void *section, void *key, void *buf);

#endif
