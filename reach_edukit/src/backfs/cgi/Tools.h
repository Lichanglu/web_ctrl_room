#ifndef __TOOLS__H
#define __TOOLS__H

extern void showPage(char *filename,char *appname);
extern int getLanguageValue(char *filename,char *source,char *value);
extern void forwardPage(char *pagename,int page,char *alertvalue);
extern void forwardPages(char *pagename,int page);
extern int updateConfigFile(char *filename,char *source,char *value);
extern int getOwn();
#endif
