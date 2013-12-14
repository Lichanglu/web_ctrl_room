#include "Tools.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "cgic.h"
#include "cgi.h"


int getOwn()
{
	char tmpOwn[2]={0};
	int result = 0;
	getLanguageValue("setOwn.txt","own",tmpOwn);
	result = atoi(tmpOwn);
	return result;
}

//��ȡ�����ļ�ֵ
int getLanguageValue(char *filename,char *source,char *value)
{
	FILE* f=NULL;
	char tmpbuf[200];
	char tmpsource[100];
	f=fopen(filename,"r");
	if(f==NULL){
		strcpy(value,source);
		return -1;
	}
	memset(tmpbuf,0,200);
	memset(tmpsource,0,100);
	sprintf(tmpsource,"%s=",source);

	while(fgets(tmpbuf,200,f)!=NULL){
		if(strstr(tmpbuf,tmpsource)!=NULL){
			//int readlen=strlen(tmpbuf)-strlen(source)-1;//��ȥ= \n
			int readlen=strlen(tmpbuf)-strlen(source)-2;//��ȥ= \n
			strncpy(value,strstr(tmpbuf,source)+strlen(source)+1,readlen);
			value[readlen]='\0';
			break;
		}
	}
	fclose(f);
	return 0;
}
//�޸������ļ�ֵ
int updateConfigFile(char *filename,char *source,char *value)
{
	FILE* file=NULL;
	char tmpbuf[200];
	//int len=0;
	char tmpfilename[20]={0};
	char syscmd[100]={0};
	strcpy(tmpfilename,"tmpconfig");
	sprintf(syscmd,"cp -f %s %s;sync",tmpfilename,filename);
	file=fopen(tmpfilename,"wb+");
	if(file==NULL){
		return -1;
	}
	memset(tmpbuf,0,200);
		fwrite("username=admin\n",15,1,file);
	memset(tmpbuf,0,200);
	if(!strcmp(source,"password")){
		sprintf(tmpbuf,"%s=%s\n",source,value);
		fwrite(tmpbuf,strlen(tmpbuf),1,file);
	}else{
		sprintf(tmpbuf,"password=%s\n",sys_password);
		fwrite(tmpbuf,strlen(tmpbuf),1,file);
	}

	memset(tmpbuf,0,200);
		fwrite("webusername=operator\n",21,1,file);
	memset(tmpbuf,0,200);
	if(!strcmp(source,"webpassword")){
		sprintf(tmpbuf,"%s=%s\n",source,value);
		fwrite(tmpbuf,strlen(tmpbuf),1,file);
	}else{
		sprintf(tmpbuf,"webpassword=%s\n",sys_webpassword);
		fwrite(tmpbuf,strlen(tmpbuf),1,file);
	}

	memset(tmpbuf,0,200);
	if(!strcmp(source,"timeout")){
		sprintf(tmpbuf,"%s=%s\n",source,value);
		fwrite(tmpbuf,strlen(tmpbuf),1,file);
	}else{
		sprintf(tmpbuf,"timeout=%s\n",sys_timeout);
		fwrite(tmpbuf,strlen(tmpbuf),1,file);
	}

	memset(tmpbuf,0,200);
	if(!strcmp(source,"language")){
		sprintf(tmpbuf,"%s=%s\n",source,value);
		fwrite(tmpbuf,strlen(tmpbuf),1,file);
	}else{
		sprintf(tmpbuf,"language=%s\n",sys_language);
		fwrite(tmpbuf,strlen(tmpbuf),1,file);
	}
	fflush(file);
	fclose(file);
	system(syscmd);
	return 0;
}
void forwardPage(char *pagename,int page,char *alertvalue)
{
	fprintf(cgiOut,"<script language='JavaScript'>alert('%s');document.location='%s?actioncode=%d'</script>",alertvalue,pagename,page);
}
void forwardPages(char *pagename,int page)
{
	fprintf(cgiOut,"<script language='JavaScript'>document.location='%s?actioncode=%d'</script>",pagename,page);
}
void forwardPageU(char * pagename, int page, char * username)
{
	fprintf(cgiOut,"<script language='JavaScript'>document.location='%s?actioncode=%d&u=%s'</script>",pagename,page,username);	
}

void showPage_xml(char *filename,char *appname)
{
	int ch=0;
	int ch2 = 0;
	char key[100];
	char buf[300];
	
	FILE* html=NULL;	
	html=fopen(filename,"r");
	if(html==NULL){
		fprintf(cgiOut,"\nfile %s not exist",filename);
		return;
	}
	while((ch=getc(html)) != EOF)
	{
		if(ch == '\r' || ch == '\n' )
		{
			continue;
			//putc(' ',cgiOut);
		}
		else
			putc(ch,cgiOut);
	}
	fclose(html);
}



void showPage(char *filename,char *appname)
{
	int ch=0;
	char key[100];
	char buf[150];
	
	FILE* html=NULL;	
	html=fopen(filename,"r");
	if(html==NULL){
		fprintf(cgiOut,"\nfile %s not exist",filename);
		return;
	}
	while((ch=getc(html)) != EOF){
		if(ch!='$')
			//putchar(ch);
			putc(ch,cgiOut);
		else{
			ch=getc(html);
			if(ch=='/'||ch=='"'){//������ʽ
				//putchar('$');
				putc('$',cgiOut);
				//putchar(ch);
				putc(ch,cgiOut);
			}else{
				int i=1;
				memset(key,0,100);
				memset(buf,0,150);
				key[0]=ch;
				while((ch=getc(html)) != '$'){
					key[i]=ch;
					i++;
				}
				if(strcmp(key,"ini.companyname")==0&&getOwn()==1)
					strcpy(key,"ini.companyname_own");
				if(strcmp(key,"bgloginjpg")==0&&getOwn()==1)
					strcpy(key,"bgloginjpg_own");
				if(strcmp(key,"topGif")==0&&getOwn()==1)
					strcpy(key,"topGif_own");

				getLanguageValue(appname,key,buf);
                if(strcmp(key,"ini.reachSystem")==0){
                    char tmp[20]={0};
                    getLanguageValue(appname,key,buf);
                    WebGetDevideType(tmp);
                    sprintf(buf,"%s(%s)",buf,tmp);
                }                
				fprintf(cgiOut,buf);
			}
		}
	}
	fclose(html);
}
