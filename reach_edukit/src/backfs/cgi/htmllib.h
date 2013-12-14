/* htmllib.h */

#ifndef _HTMLLIB_H
#define _HTMLLIB_H

/* function prototypes */
void htmlHeader(char *title);
void htmlBody();
void htmlFooter();
void addTitleElement(char *title);
void setformdata(char* form,char* item,char* value);
void setformintvalue(char* form,char* item,int value);
int checktimeoutornologin();

#endif	/* !_HTMLLIB_H*/
