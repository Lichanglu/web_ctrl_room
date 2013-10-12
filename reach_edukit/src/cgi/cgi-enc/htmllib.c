/* htmllib.c
 * HTML common library functions for the CGI programs. */

#include <stdio.h>
#include <stdlib.h>
#include <time.h>		/* time() */
#include "htmllib.h"
#include "cgic.h"
#include "Tools.h"
#include "cgi.h"

void htmlHeader(char *title) {
  cgiHeaderContentType("text/html");
  fprintf(cgiOut,"<!DOCTYPE html PUBLIC '-//W3C//DTD HTML 4.01//EN' 'http://www.w3.org/TR/html4/strict.dtd'> \r\n");
  fprintf(cgiOut,"<html><head>\r\n");
  fprintf(cgiOut,"<meta http-equiv='Content-Type' content='text/html; charset=utf-8'>\r\n\r\n");
  fprintf(cgiOut,"<TITLE>%s</TITLE></HEAD>",title);
}

void htmlBody() {
    fprintf(cgiOut,"<BODY>");
}

void htmlFooter() {
    fprintf(cgiOut,"</BODY></HTML>");
}

void addTitleElement(char *title) {
	fprintf(cgiOut,"<H1>%s</H1>", title);
}




void setformdata(char* form,char* item,char* value)
{
	//fprintf(cgiOut,"document.%s.%s.value='%s';\n",form,item,value);
//	printf("document.%s.%s.value='%s';\n",form,item,value);
}


void setformintvalue(char* form,char* item,int value)
{
	//fprintf(cgiOut,"document.%s.%s.value=%d;\n",form,item,value);	
//	printf("document.%s.%s.value=%d;\n",form,item,value);	
}


int checktimeoutornologin()
{
	int old_time;
	int nowvalue;
	cgiCookieInteger("sessiontime",&old_time,0);
	nowvalue = (int)time(NULL);
	if(nowvalue-old_time >atoi(sys_timeout))
	{
		forwardPages("decode.cgi",202);
		return -1;
	}
	//??cookie|??
	cgiSetCookienoTime("sessiontime",nowvalue);
	
	return 0;
}
