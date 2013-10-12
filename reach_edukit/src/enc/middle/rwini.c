/*********************************************************************
*
* FileName :  rwini.C
* FileType:
* Other : INI file operation
* Current Ver:  V1.0
* Author:	Ysh
* Finish Date:
*
**********************************************************************/


#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#define SuccessRet 1;
#define FailedRet  0;

#define MAX_CFG_BUF                              512

#define CFG_OK                                   0
#define CFG_SECTION_NOT_FOUND                    -1
#define CFG_KEY_NOT_FOUND                        -2
#define CFG_ERR                                  -10
#define CFG_ERR_FILE                             -10
#define CFG_ERR_OPEN_FILE                        -10
#define CFG_ERR_CREATE_FILE                      -11
#define CFG_ERR_READ_FILE                        -12
#define CFG_ERR_WRITE_FILE                       -13
#define CFG_ERR_FILE_FORMAT                      -14
#define CFG_ERR_SYSTEM                           -20
#define CFG_ERR_SYSTEM_CALL                      -20
#define CFG_ERR_INTERNAL                         -21
#define CFG_ERR_EXCEED_BUF_SIZE                  -22

#define COPYF_OK                                 0
#define COPYF_ERR_OPEN_FILE                      -10
#define COPYF_ERR_CREATE_FILE                    -11
#define COPYF_ERR_READ_FILE                      -12
#define COPYF_ERR_WRITE_FILE                     -13

#define TXTF_OK                                  0
#define TXTF_ERR_OPEN_FILE                       -1
#define TXTF_ERR_READ_FILE                       -2
#define TXTF_ERR_WRITE_FILE                      -3
#define TXTF_ERR_DELETE_FILE                     -4
#define TXTF_ERR_NOT_FOUND                       -5

char CFG_ssl = '[', CFG_ssr = ']';
char CFG_nis = ':';                 /*name and index beyond separator */
char CFG_nts = '#';                 /*remark*/

int  CFG_section_line_no, CFG_key_line_no, CFG_key_lines;

static char *strtrimr(char *buf);
static char *strtriml(char *buf);
static int  FileGetLine(FILE *fp, char *buffer, int maxlen);
static int  SplitKeyValue(char *buf, char **key, char **val);

/**********************************************************************
* Function Name:  strtrimr
* Function scriptor:  The right to remove the empty character string
* Access table:   NULL
* Modify Table:	  NULL
* Input param：   char * buf
* Output Param:  NULL
* Return:  		 Char point
***********************************************************************/
char *strtrimr(char *buf)
{
	int len, i;
	char *tmp = NULL;
	len = strlen(buf);
	tmp = (char *)malloc(len);

	memset(tmp, 0x00, len);

	for(i = 0; i < len; i++) {
		if(buf[i] != ' ') {
			break;
		}
	}

	if(i < len) {
		strncpy(tmp, (buf + i), (len - i));
	}

	strncpy(buf, tmp, len);
	free(tmp);
	return buf;
}

/**********************************************************************
* Function Name:  strtriml
* Function scriptor:  The Left to remove the empty character string
* Access table:   NULL
* Modify Table:	  NULL
* Input param：   char * buf
* Output Param:  NULL
* Return:  		 Char point
***********************************************************************/
char *strtriml(char *buf)
{
	int len, i;
	char *tmp = NULL;
	len = strlen(buf);
	tmp = (char *)malloc(len);
	memset(tmp, 0x00, len);

	for(i = 0; i < len; i++) {
		if(buf[len - i - 1] != ' ') {
			break;
		}
	}

	if(i < len) {
		strncpy(tmp, buf, len - i);
	}

	strncpy(buf, tmp, len);
	free(tmp);
	return buf;
}

/**********************************************************************
* Function Name:  FileGetLine
* Function scriptor:  Reads a line from the file
* Access table:   NULL
* Modify Table:	  NULL
* Input param：   FILE *fp    int maxlen
* Output Param:   Real read Length
* Return:  		 Char point
***********************************************************************/
int  FileGetLine(FILE *fp, char *buffer, int maxlen)
{
	int  i, j;
	char ch1;

	for(i = 0, j = 0; i < maxlen; j++) {
		if(fread(&ch1, sizeof(char), 1, fp) != 1) {
			if(feof(fp) != 0) {
				if(j == 0) {
					return -1;    /* File End */
				} else {
					break;
				}
			}

			if(ferror(fp) != 0) {
				return -2;    /* Read File Error */
			}

			return -2;
		} else {
			if(ch1 == '\n' || ch1 == 0x00) {
				break;    /* Wrap */
			}

			if(ch1 == '\f' || ch1 == 0x1A) {      /* '\f':Formfeed counts as valid characters */
				buffer[i++] = ch1;
				break;
			}

			if(ch1 != '\r') {
				buffer[i++] = ch1;    /* Ignore carriage return character */
			}
		}
	}

	buffer[i] = '\0';
	return i;
}

/**********************************************************************
* Function Name:  FileCopy
* Function scriptor:  File copy
* Access table:   NULL
* Modify Table:	  NULL
* Input param：   void *source_file   void *dest_file
* Output Param:   NULL
* Return:  		  0 -- OK  0-- FAIL
***********************************************************************/
int  FileCopy(void *source_file, void *dest_file)
{
	FILE *fp1, *fp2;
	char buf[1024 + 1];
	int  ret;

	if((fp1 = fopen((char *)source_file, "r")) == NULL) {
		return COPYF_ERR_OPEN_FILE;
	}

	ret = COPYF_ERR_CREATE_FILE;

	if((fp2 = fopen((char *)dest_file, "w")) == NULL) {
		goto copy_end;
	}

	while(1) {
		ret = COPYF_ERR_READ_FILE;
		memset(buf, 0x00, 1024 + 1);

		if(fgets((char *)buf, 1024, fp1) == NULL) {
			if(strlen(buf) == 0) {
				if(ferror(fp1) != 0) {
					goto copy_end;
				}

				break;                                   /* File Finish */
			}
		}

		ret = COPYF_ERR_WRITE_FILE;

		if(fputs((char *)buf, fp2) == EOF) {
			goto copy_end;
		}
	}

	ret = COPYF_OK;
copy_end:

	if(fp2 != NULL) {
		fclose(fp2);
	}

	if(fp1 != NULL) {
		fclose(fp1);
	}

	return ret;
}

/**********************************************************************
* Function Name:  SplitSectionToNameIndex
* Function scriptor:  Split section for name and index
*            [section]
*              /   \
*            name:index
*            jack  :   12
*   	     |     |   |
*            k1    k2  i
*  Access table:   NULL
*  Modify Table:	  NULL
*  Input param：   char *section
*  Output Param:   char **name, char **index
*  Return:   1 --- ok
*			 0 --- blank line
*			-1 --- no name, ":index"
*			-2 --- only name, no ':'
***********************************************************************/
int  SplitSectionToNameIndex(char *section, char **name, char **index)
{
	int  i, k1, k2, n;

	if((n = strlen((char *)section)) < 1) {
		return 0;
	}

	for(i = 0; i < n; i++)
		if(section[i] != ' ' && section[i] != '\t') {
			break;
		}

	if(i >= n) {
		return 0;
	}

	if(section[i] == CFG_nis) {
		return -1;
	}

	k1 = i;

	for(i++; i < n; i++)
		if(section[i] == CFG_nis) {
			break;
		}

	if(i >= n) {
		return -2;
	}

	k2 = i;

	for(i++; i < n; i++)
		if(section[i] != ' ' && section[i] != '\t') {
			break;
		}

	section[k2] = '\0';
	*name = section + k1;
	*index = section + i;
	return 1;
}

/**********************************************************************
* Function Name:  JoinNameIndexToSection
* Function scriptor:  join name and indexsection for section
*            jack  :   12
*            name:index
*              \   /
*            [section]
*  Access table:   NULL
*  Modify Table:	  NULL
*  Input param：   char *name, char *index
*  Output Param:   char **section
*  Return:   1 --- ok
*			 0 --- blank line
***********************************************************************/
int  JoinNameIndexToSection(char **section, char *name, char *index)
{
	int n1, n2;

	if((n1 = strlen((char *)name)) < 1) {
		return 0;
	}

	if((n2 = strlen((char *)index)) < 1) {
		return 0;
	}

	strcat(*section, name);
	strcat(*section + n1, ":");
	strcat(*section + n1 + 1, index);
	*(*section + n1 + 1 + n2) = '\0';
	return 1;
}

/**********************************************************************
* Function Name:  SplitKeyValue
* Function scriptor:  split  key and value
*　　　　　　key=val
*			jack   =   liaoyuewang
*			|      |   |
*			k1     k2  i
*  Access table:   NULL
*  Modify Table:	  NULL
*  Input param：   char *buf
*  Output Param:   char **key;char **val
*  Return:   1 --- ok
*			 0 --- blank line
*			-1 --- no key, "= val"
*			-2 --- only key, no '='
***********************************************************************/
int  SplitKeyValue(char *buf, char **key, char **val)
{
	int  i, k1, k2, n;

	if((n = strlen((char *)buf)) < 1) {
		return 0;
	}

	for(i = 0; i < n; i++)
		if(buf[i] != ' ' && buf[i] != '\t') {
			break;
		}

	if(i >= n) {
		return 0;
	}

	if(buf[i] == '=') {
		return -1;
	}

	k1 = i;

	for(i++; i < n; i++)
		if(buf[i] == '=') {
			break;
		}

	if(i >= n) {
		return -2;
	}

	k2 = i;

	for(i++; i < n; i++)
		if(buf[i] != ' ' && buf[i] != '\t') {
			break;
		}

	buf[k2] = '\0';
	*key = buf + k1;
	*val = buf + i;
	return 1;
}

/**********************************************************************
* Function Name:  ConfigGetKey
* Function scriptor:  Get Key Value
* Access table:   NULL
* Modify Table:	  NULL
* Input param：   void *CFG_file File   void *section   void *key
* Output Param:   void *buf  key Value
* Return:  		  0 -- OK  no 0-- FAIL
***********************************************************************/


int  ConfigGetKey(void *CFG_file, void *section, void *key, void *buf)
{
	FILE *fp;
	char buf1[MAX_CFG_BUF + 1], buf2[MAX_CFG_BUF + 1];
	char *key_ptr, *val_ptr;
	int  line_no, n, ret;

	line_no = 0;
	CFG_section_line_no = 0;
	CFG_key_line_no = 0;
	CFG_key_lines = 0;

	if((fp = fopen((char *)CFG_file, "rb")) == NULL) {
		return CFG_ERR_OPEN_FILE;
	}

	while(1) {                                     /* Find section */
		ret = CFG_ERR_READ_FILE;
		n = FileGetLine(fp, buf1, MAX_CFG_BUF);

		if(n < -1) {
			goto r_cfg_end;
		}

		ret = CFG_SECTION_NOT_FOUND;

		if(n < 0) {
			goto r_cfg_end;    /* File end no find */
		}

		line_no++;
		n = strlen(strtriml(strtrimr(buf1)));

		if(n == 0 || buf1[0] == CFG_nts) {
			continue;    /* blank or '#' */
		}

		ret = CFG_ERR_FILE_FORMAT;

		if(n > 2 && ((buf1[0] == CFG_ssl && buf1[n - 1] != CFG_ssr))) {
			goto r_cfg_end;
		}

		if(buf1[0] == CFG_ssl) {
			buf1[n - 1] = 0x00;

			if(strcmp(buf1 + 1, section) == 0) {
				break;    /* Find section */
			}
		}
	}

	CFG_section_line_no = line_no;

	while(1) {                                     /* Find key */
		ret = CFG_ERR_READ_FILE;
		n = FileGetLine(fp, buf1, MAX_CFG_BUF);

		if(n < -1) {
			goto r_cfg_end;
		}

		ret = CFG_KEY_NOT_FOUND;

		if(n < 0) {
			goto r_cfg_end;    /* File End  no Find key */
		}

		line_no++;
		CFG_key_line_no = line_no;
		CFG_key_lines = 1;
		n = strlen(strtriml(strtrimr(buf1)));

		if(n == 0 || buf1[0] == CFG_nts) {
			continue;    /* blank or '#'*/
		}

		ret = CFG_KEY_NOT_FOUND;

		if(buf1[0] == CFG_ssl) {
			goto r_cfg_end;
		}

		if(buf1[n - 1] == '+') {                     /* '+' next line go on  */
			buf1[n - 1] = 0x00;

			while(1) {
				ret = CFG_ERR_READ_FILE;
				n = FileGetLine(fp, buf2, MAX_CFG_BUF);

				if(n < -1) {
					goto r_cfg_end;
				}

				if(n < 0) {
					break;    /* File end */
				}

				line_no++;
				CFG_key_lines++;
				n = strlen(strtrimr(buf2));
				ret = CFG_ERR_EXCEED_BUF_SIZE;

				if(n > 0 && buf2[n - 1] == '+') {      /* '+' next line go on  */
					buf2[n - 1] = 0x00;

					if(strlen(buf1) + strlen(buf2) > MAX_CFG_BUF) {
						goto r_cfg_end;
					}

					strcat(buf1, buf2);
					continue;
				}

				if(strlen(buf1) + strlen(buf2) > MAX_CFG_BUF) {
					goto r_cfg_end;
				}

				strcat(buf1, buf2);
				break;
			}
		}

		ret = CFG_ERR_FILE_FORMAT;

		if(SplitKeyValue(buf1, &key_ptr, &val_ptr) != 1) {
			goto r_cfg_end;
		}

		strtriml(strtrimr(key_ptr));

		if(strcmp(key_ptr, key) != 0) {
			continue;    /* and key not match */
		}

		strcpy(buf, val_ptr);
		break;
	}

	ret = CFG_OK;
r_cfg_end:

	if(fp != NULL) {
		fclose(fp);
	}

	return ret;
}


/**********************************************************************
* Function Name:  ConfigSetKey
* Function scriptor:  Set Key Value
* Access table:   NULL
* Modify Table:	  NULL
* Input param：   void *CFG_file File   void *section   void *key
					void *buf
* Output Param:   void *buf  key Value
* Return:  		  0 -- OK  no 0-- FAIL
***********************************************************************/
int  ConfigSetKey(void *CFG_file, void *section, void *key, void *buf)
{
	FILE *fp1, *fp2;
	char buf1[MAX_CFG_BUF + 1];
	int  line_no, line_no1, n, ret, ret2;
	char tmpfname[] = "tmp_XXXXXXXXXXXXXXXX";
	int  tmpfd = 0;

	ret = ConfigGetKey(CFG_file, section, key, buf1);

	if(ret <= CFG_ERR && ret != CFG_ERR_OPEN_FILE) {
		return ret;
	}

	if(ret == CFG_ERR_OPEN_FILE || ret == CFG_SECTION_NOT_FOUND) {

		if((fp1 = fopen((char *)CFG_file, "a")) == NULL)

		{
			return CFG_ERR_CREATE_FILE;
		}

		if(fprintf(fp1, "%c%s%c\n", CFG_ssl, (char *)section, CFG_ssr) == EOF) {
			fclose(fp1);
			return CFG_ERR_WRITE_FILE;
		}

		if(fprintf(fp1, "%s=%s\n", (char *)key, (char *)buf) == EOF) {
			fclose(fp1);
			return CFG_ERR_WRITE_FILE;
		}

		fclose(fp1);
		return CFG_OK;
	}

	if((tmpfd = mkstemp((char *)tmpfname)) == -1)  {
		printf("create temp file failed \n");
		return CFG_ERR_CREATE_FILE;
	}

	close(tmpfd);

	if((fp2 = fopen(tmpfname, "w")) == NULL) {
		return CFG_ERR_CREATE_FILE;
	}

	ret2 = CFG_ERR_OPEN_FILE;

	if((fp1 = fopen((char *)CFG_file, "rb")) == NULL) {
		goto w_cfg_end;
	}


	if(ret == CFG_KEY_NOT_FOUND) {
		line_no1 = CFG_section_line_no;
	} else { /* ret = CFG_OK */
		line_no1 = CFG_key_line_no - 1;
	}

	for(line_no = 0; line_no < line_no1; line_no++) {
		ret2 = CFG_ERR_READ_FILE;
		n = FileGetLine(fp1, buf1, MAX_CFG_BUF);

		if(n < 0) {
			goto w_cfg_end;
		}

		ret2 = CFG_ERR_WRITE_FILE;

		if(fprintf(fp2, "%s\n", buf1) == EOF) {
			goto w_cfg_end;
		}
	}

	if(ret != CFG_KEY_NOT_FOUND)
		for(; line_no < line_no1 + CFG_key_lines; line_no++) {
			ret2 = CFG_ERR_READ_FILE;
			n = FileGetLine(fp1, buf1, MAX_CFG_BUF);

			if(n < 0) {
				goto w_cfg_end;
			}
		}

	ret2 = CFG_ERR_WRITE_FILE;

	if(fprintf(fp2, "%s=%s\n", (char *)key, (char *)buf) == EOF) {
		goto w_cfg_end;
	}

	while(1) {
		ret2 = CFG_ERR_READ_FILE;
		n = FileGetLine(fp1, buf1, MAX_CFG_BUF);

		if(n < -1) {
			goto w_cfg_end;
		}

		if(n < 0) {
			break;
		}

		ret2 = CFG_ERR_WRITE_FILE;

		if(fprintf(fp2, "%s\n", buf1) == EOF) {
			goto w_cfg_end;
		}
	}

	ret2 = CFG_OK;
w_cfg_end:

	if(fp1 != NULL) {
		fclose(fp1);
	}

	if(fp2 != NULL) {
		fclose(fp2);
	}

	if(ret2 == CFG_OK) {
		ret = FileCopy(tmpfname, CFG_file);

		if(ret != 0) {
			return CFG_ERR_CREATE_FILE;
		}
	}

	unlink(tmpfname);
	return ret2;
}

/**********************************************************************
* Function Name:  ConfigGetSections
* Function scriptor:  Get All Section
* Access table:   NULL
* Modify Table:	  NULL
* Input param：   void *CFG_file File
* Output Param:   char *sections[]  : save section name
* Return:  		  section count
***********************************************************************/
int  ConfigGetSections(void *CFG_file, char *sections[])
{
	FILE *fp;
	char buf1[MAX_CFG_BUF + 1];
	int  n, n_sections = 0, ret;


	if((fp = fopen(CFG_file, "rb")) == NULL) {
		return CFG_ERR_OPEN_FILE;
	}

	while(1) {                                     /* Find section */
		ret = CFG_ERR_READ_FILE;
		n = FileGetLine(fp, buf1, MAX_CFG_BUF);

		if(n < -1) {
			goto cfg_scts_end;
		}

		if(n < 0) {
			break;    /* File End */
		}

		n = strlen(strtriml(strtrimr(buf1)));

		if(n == 0 || buf1[0] == CFG_nts) {
			continue;    /* blank or '#' */
		}

		ret = CFG_ERR_FILE_FORMAT;

		if(n > 2 && ((buf1[0] == CFG_ssl && buf1[n - 1] != CFG_ssr))) {
			goto cfg_scts_end;
		}

		if(buf1[0] == CFG_ssl) {
			buf1[n - 1] = 0x00;
			strcpy(sections[n_sections], buf1 + 1);
			n_sections++;
		}
	}

	ret = n_sections;
cfg_scts_end:

	if(fp != NULL) {
		fclose(fp);
	}

	return ret;
}

/**********************************************************************
* Function Name:  ConfigGetKeys
* Function scriptor:  Get All Key
* Access table:   NULL
* Modify Table:	  NULL
* Input param：   void *CFG_file File    void *section
* Output Param:    char *keys[]   : save key name
* Return:  		  key count   error: < 0
***********************************************************************/
int  ConfigGetKeys(void *CFG_file, void *section, char *keys[])
{
	FILE *fp;
	char buf1[MAX_CFG_BUF + 1], buf2[MAX_CFG_BUF + 1];
	char *key_ptr, *val_ptr;
	int  n, n_keys = 0, ret;


	if((fp = fopen(CFG_file, "rb")) == NULL) {
		return CFG_ERR_OPEN_FILE;
	}

	while(1) {                                     /* Find section */
		ret = CFG_ERR_READ_FILE;
		n = FileGetLine(fp, buf1, MAX_CFG_BUF);

		if(n < -1) {
			goto cfg_keys_end;
		}

		ret = CFG_SECTION_NOT_FOUND;

		if(n < 0) {
			goto cfg_keys_end;    /* File End */
		}

		n = strlen(strtriml(strtrimr(buf1)));

		if(n == 0 || buf1[0] == CFG_nts) {
			continue;    /* Blank or '#' */
		}

		ret = CFG_ERR_FILE_FORMAT;

		if(n > 2 && ((buf1[0] == CFG_ssl && buf1[n - 1] != CFG_ssr))) {
			goto cfg_keys_end;
		}

		if(buf1[0] == CFG_ssl) {
			buf1[n - 1] = 0x00;

			if(strcmp(buf1 + 1, section) == 0) {
				break;    /* Find section */
			}
		}
	}

	while(1) {
		ret = CFG_ERR_READ_FILE;
		n = FileGetLine(fp, buf1, MAX_CFG_BUF);

		if(n < -1) {
			goto cfg_keys_end;
		}

		if(n < 0) {
			break;    /* File End */
		}

		n = strlen(strtriml(strtrimr(buf1)));

		if(n == 0 || buf1[0] == CFG_nts) {
			continue;    /* Blank and '#' */
		}

		ret = CFG_KEY_NOT_FOUND;

		if(buf1[0] == CFG_ssl) {
			break;    /* Next section */
		}

		if(buf1[n - 1] == '+') {                     /*'+' next line go on */
			buf1[n - 1] = 0x00;

			while(1) {
				ret = CFG_ERR_READ_FILE;
				n = FileGetLine(fp, buf2, MAX_CFG_BUF);

				if(n < -1) {
					goto cfg_keys_end;
				}

				if(n < 0) {
					break;    /* File End */
				}

				n = strlen(strtrimr(buf2));
				ret = CFG_ERR_EXCEED_BUF_SIZE;

				if(n > 0 && buf2[n - 1] == '+')    {       /*'+' next line go on */
					buf2[n - 1] = 0x00;

					if(strlen(buf1) + strlen(buf2) > MAX_CFG_BUF) {
						goto cfg_keys_end;
					}

					strcat(buf1, buf2);
					continue;
				}

				if(strlen(buf1) + strlen(buf2) > MAX_CFG_BUF) {
					goto cfg_keys_end;
				}

				strcat(buf1, buf2);
				break;
			}
		}

		ret = CFG_ERR_FILE_FORMAT;

		if(SplitKeyValue(buf1, &key_ptr, &val_ptr) != 1) {
			goto cfg_keys_end;
		}

		strtriml(strtrimr(key_ptr));
		strcpy(keys[n_keys], key_ptr);
		n_keys++;
	}

	ret = n_keys;
cfg_keys_end:

	if(fp != NULL) {
		fclose(fp);
	}

	return ret;
}

#if 0

/**********************************************************************
* 函数名称： main
* 功能描述： 测试函数入口
* 访问的表： 无
* 修改的表： 无
* 输入参数： 无
* 输出参数： 无
* 返 回 值： 无
* 其它说明： 无
*
***********************************************************************/
int main(void)
{
	char buf[20] = "";
	char buf1[20] = "";
	char buf2[20] = "";
	char buf3[20] = "";
	int  ret;
	long abc;
	int i;

	char *section;
	char *key;
	char *val;
	char *name;
	char *index;
	section = buf1;
	key = buf2;
	val = buf3;

	ret = ConfigSetKey("Config.dat", "Jack:Lio", "Jack", "-12321");

	for(i = 0; i < 20; i++) {
		memset(buf, 0x00, 20);
		memset(buf1, 0x00, 20);
		memset(buf2, 0x00, 20);
		memset(buf3, 0x00, 20);
		sprintf(buf, "%d", i);
		sprintf(buf2, "Lio%d", i);
		sprintf(buf3, "%d", i);
		JoinNameIndexToSection(&section, "Jack", buf);
		ConfigSetKey("Config.dat", section, key, val);
	}

	name = buf2;
	index = buf3;
	memset(buf1, 0x00, 20);
	memset(buf2, 0x00, 20);
	memset(buf3, 0x00, 20);
	strcpy(buf1, "Jack:Lio");
	SplitSectionToNameIndex(section, &name, &index);
	printf("\n name=%s,index=%s\n", name, index);

	ret = ConfigGetKey("Config.dat", "Jack:Lio", "Jack", buf);

	if(strcmp(buf, "") != 0) {
		abc = atol(buf);
	}

	printf("\n buf=%s\n", buf);
	printf("\n abc=%ld\n", abc);

	ConfigSetKey("Config.dat", "Jack:Lio", "Email", "liaoyuewang@163.com");

	FileCopy("Config.dat", "Configbak.dat");

	printf("\nFile %s line%d\n", __FILE__, __LINE__);


	printf("\n******** This test is created by Jack Lio. Email:liaoyuewang@163.com********\n\n");
	return 0;

}
#endif //RWINI_TEST

