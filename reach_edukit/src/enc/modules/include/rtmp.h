/*
**************************************************************************************
º¯Êı½éÉÜ:  ¹ş¹ş
1.void RtmpInit()
    ¹¦ÄÜ½éÉÜ:
        ¸Ãº¯ÊıÖ÷ÒªÍê³ÉRTSP ¸÷Ïî²ÎÊı³õÊ¼»¯
            a.³õÊ¼»¯Á¬½Ó¿Í»§ĞÅÏ¢
            b.³õÊ¼»¯Ëø
            c.·ÖÅäÒôÆµ´ò°üËùĞèµÄÄÚ´æ¿Õ¼ä

2.void RtmpAddClient(struct sockaddr_in client_addr,int sock,int nPos)
    ¹¦ÄÜ½éÉÜ:
        ¸Ãº¯ÊıÖ÷ÒªÍê³ÉÔö¼Ó¿Í»§¶ËnposµÄÍøÂçµØÖ·
    ²ÎÊıËµÃ÷:
        client_addr:Á¬½ÓµÄ¿Í»§¶ËµÄÍøÂçµØÖ·
        sock:Á¬½Ó¿Í»§µÄsocket
        nPos:ÒªÁ¬½ÓµÄ¿Í»§¶ËµÄĞòºÅ
3.int  RtmpDelClient(int nPos)
    ¹¦ÄÜ½éÉÜ:
        ¸Ãº¯ÊıÓÃÓÚ¶Ï¿ªĞòºÅÎªnPosµÄ¿Í»§¶Ë
    ²ÎÊıËµÃ÷:
        nPos:Òª¶Ï¿ªµÄ¿Í»§¶ËµÄĞòºÅ
4.int  RtmpGetNullClientIndex()
    ¹¦ÄÜ½éÉÜ:
        ¸Ãº¯ÊıÓÃÓÚ»ñÈ¡Ò»¸ö¿ÕµÄ¿Í»§ºÅ£¬×î´ó¿Í»§ºÅÎª5(×î¶àÄÜÁ¬6¸ö¿Í»§)
5.int  RtmpGetClientNum()
    ¹¦ÄÜ½éÉÜ:
        ¸Ãº¯ÊıÖ÷Òª·µ»Ø¿Í»§¶ËµÄÊıÄ¿£¬ÓÃÓÚÅĞ¶ÏÊÇ·ñÓĞ¿Í»§Á¬½Ó
6.int  RtmpExit()
    ¹¦ÄÜ½éÉÜ:
        ÍË³ö RTMP Ğ­Òé
            a.ËøÏú»Ù
            b.ÊÍ·Å·ÖÅäµÄÄÚ´æ    
7.void RtmpSetupConnection(int nPos)
	¹¦ÄÜ½éÉÜ:
		½¨Á¢ÓëµÚn ¸ö¿Í»§Ö®¼äµÄÁ¬½Ó
	²ÎÊıËµÃ÷:
		 nPos:ÒªÁ¬½ÓµÄ¿Í»§¶ËµÄĞòºÅ
		 
8.RtmpAudioPack(int nLen, unsigned char * pData)
    ¹¦ÄÜ½éÉÜ:
        ¸Ãº¯ÊıÖ÷ÒªÍê³ÉÒôÆµµÄrtmp´ò°ü
    ²ÎÊıËµÃ÷:
        nLen:Òª´ò°üµÄaacÊÓÆµ³¤¶È
        pData:Òª´ò°üµÄaacÊÓÆµµØÖ·
        
9.RtmpVideoPack(int nLen, unsigned char *pData)
    ¹¦ÄÜ½éÉÜ:
        ¸Ãº¯ÊıÖ÷ÒªÍê³ÉÊÓÆµµÄrtmp´ò°ü
    ²ÎÊıËµÃ÷:
        nLen:Òª´ò°üµÄ264ÊÓÆµ³¤¶È
        pData:Òª´ò°üµÄ264ÊÓÆµµØÖ·
10.char *RtmpGetVersion()
    ¹¦ÄÜ½éÉÜå£º
        ¸Ãº¯ÊıÖ÷ÒªÓÃÓÚ»ñÈ¡rtmp Ğ­ÒéµÄ°æ±¾ºÅ
**************************************************************************************
*/

#define RTMP_LISTEN_PORT                1935// 4212//

#define PRINT_DEUBG

extern char *RtmpGetVersion();
extern int  RtmpGetClientNum();
extern void RtmpTask();



extern void RtmpAudioPack(int nLen, unsigned char *pData);
extern void RtmpVideoPack(int nLen, unsigned char *pData);

