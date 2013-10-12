#include "receive_module.h"
#include "media_msg.h"
#include "xml_base.h"


int32_t receive_xml_login(recv_xml_handle *xml_hand, int8_t *out_buf, int32_t *out_len)
{
    int32_t return_code = OPERATION_ERR;
    parse_xml_t recv_module_xml_t;
    xmlNodePtr New_node, New_node2;

	//assert(xml_hand && out_buf && out_len);

    recv_module_xml_t.pdoc = xmlNewDoc(BAD_CAST"1.0");

    if(NULL == recv_module_xml_t.pdoc) {
        nslog(NS_ERROR, "xmlNewDoc failed");
        return_code = OPERATION_ERR;
        goto EXIT;
    }

    recv_module_xml_t.proot = xmlNewNode(NULL, BAD_CAST"RequestMsg");
    xmlDocSetRootElement(recv_module_xml_t.pdoc, recv_module_xml_t.proot);
    New_node =  xmlNewNode(NULL, BAD_CAST "MsgHead");
    xmlAddChild(recv_module_xml_t.proot, New_node);
    xmlNewTextChild(New_node, NULL, "MsgCode", xml_hand->msgcode);
    xmlNewTextChild(New_node, NULL, "PassKey", xml_hand->passkey);
    New_node2 =  xmlNewNode(NULL, BAD_CAST "MsgBody");
    xmlAddChild(recv_module_xml_t.proot, New_node2);
    xmlNewTextChild(New_node2, NULL, "UserName", xml_hand->user);
    xmlNewTextChild(New_node2, NULL, "Password", xml_hand->password);
    //return_code= xmlSaveFormatFileEnc("ReceiveLog.xml", recv_module_xml_t.pdoc, "UTF-8", 1);

    xmlChar *temp_xml_buf = NULL;
    xmlDocDumpFormatMemoryEnc(recv_module_xml_t.pdoc, &temp_xml_buf, out_len,  "UTF-8", 1);
    r_memcpy(out_buf, temp_xml_buf, *out_len);

    release_dom_tree(recv_module_xml_t.pdoc);
    return_code = OPERATION_SUCC;
EXIT:
	 if(temp_xml_buf)  {
        xmlFree(temp_xml_buf);
    }
    return return_code;
}


int32_t recv_report_xml(int32_t operation, stream_handle *M_handle)
{
    int32_t return_code = OPERATION_ERR;
    parse_xml_t recv_module_xml_t;
    xmlNodePtr New_node = NULL, New_node2 = NULL, New_node3 = NULL;
	xmlChar *temp_xml_buf = NULL;
	int8_t error_buf[20] = {0};
	int8_t *recv_to_meeting_buf = NULL;
	int8_t id_buf[20] = {0};
    int32_t out_len = 0;
    m_msgque recv_to_meeting_msgque;

	//assert(M_handle);
	
    recv_module_xml_t.pdoc = xmlNewDoc(BAD_CAST"1.0");

    if(NULL == recv_module_xml_t.pdoc) {
        nslog(NS_ERROR, "xmlNewDoc failed");
        return_code = OPERATION_ERR;
        goto EXIT;
    }

    recv_module_xml_t.proot = xmlNewNode(NULL, BAD_CAST"RequestMsg");
    xmlDocSetRootElement(recv_module_xml_t.pdoc, recv_module_xml_t.proot);
    New_node =  xmlNewNode(NULL, BAD_CAST "MsgHead");
    xmlAddChild(recv_module_xml_t.proot, New_node);
    xmlNewTextChild(New_node, NULL, "MsgCode", RECV_REPORT_MSGCODE);
    xmlNewTextChild(New_node, NULL, "PassKey", RECV_REPORT_PASSKEY);
    New_node2 =  xmlNewNode(NULL, BAD_CAST "MsgBody");
    xmlAddChild(recv_module_xml_t.proot, New_node2);
    New_node3 = xmlNewNode(NULL, "EncInfo");
    xmlAddChild(New_node2, New_node3);
    sprintf(id_buf, "%d", M_handle->stream_id);
    xmlNewTextChild(New_node3, NULL, "ID", id_buf);
    sprintf(error_buf, "%d", operation);
    xmlNewTextChild(New_node3, NULL, "Status", error_buf);
   // return_code= xmlSaveFormatFileEnc("ReceiveLog.xml", recv_module_xml_t.pdoc, "UTF-8", 1);

    xmlDocDumpFormatMemoryEnc(recv_module_xml_t.pdoc, &temp_xml_buf, &out_len,  "UTF-8", 1);
    release_dom_tree(recv_module_xml_t.pdoc);

    recv_to_meeting_buf = (int8_t *)r_malloc(out_len + MsgLen + 1);
    if(recv_to_meeting_buf == NULL) {
        nslog(NS_ERROR, "r_malloc error, errno = %d", errno);
        return_code = RECEIVE_MALLOC_ERR;
        goto EXIT;
    }

    r_memset(recv_to_meeting_buf, 0, out_len + MsgLen + 1);
    ((msg_header_t *)(recv_to_meeting_buf))->m_len = htons(out_len + MsgLen);
    ((msg_header_t *)(recv_to_meeting_buf))->m_ver = htons(ENC_VER);
    ((msg_header_t *)(recv_to_meeting_buf))->m_msg_type = XML_TYPE;

    r_memcpy(recv_to_meeting_buf + MsgLen, temp_xml_buf, out_len);
    r_memset(&recv_to_meeting_msgque, 0, sizeof(m_msgque));
    recv_to_meeting_msgque.msgtype = M_handle->stream_id;
    recv_to_meeting_msgque.msgbuf = recv_to_meeting_buf;
	recv_to_meeting_buf = NULL;

	//nslog(NS_INFO, "Now i will send msg to meeting, type = 0x%x", recv_to_meeting_msgque.type);
    return_code = r_msg_send(M_handle->msg_recv_to_ctrl, & recv_to_meeting_msgque, sizeof(xmlChar *), IPC_NOWAIT);

    if(return_code) {

		if (recv_to_meeting_msgque.msgbuf) {
				r_free(recv_to_meeting_msgque.msgbuf);
				recv_to_meeting_msgque.msgbuf = NULL;
		}

		if (FALSE == IS_FLAG_BIT_TRUE(M_handle->log_flag, REPORT_BIT)) {
              nslog(NS_WARN, "msg_recv_to_ctrl failed, msgid = %d, errno = %d, stream_id=%d", M_handle->msg_recv_to_ctrl, errno, M_handle->stream_id);
				SET_FLAG_BIT_TRUE(M_handle->log_flag, REPORT_BIT);
		}
		return_code = OPERATION_SUCC;
		goto EXIT;
    }
    SET_FLAG_BIT_FALSE(M_handle->log_flag, REPORT_BIT);//
    return_code = OPERATION_SUCC;
EXIT:
	
    if(temp_xml_buf)  {
        xmlFree(temp_xml_buf);
    }
    return return_code;
}

int32_t recv_xml_login_response(stream_handle *M_handle, int8_t *xml_buf)
{
		int32_t return_code = OPERATION_ERR;
		parse_xml_t recv_module_xml_t;
		xmlNodePtr r_msghead_node = NULL, r_msgcode_node = NULL, r_returncode_node = NULL;
		int8_t recv_tmp_buf[RECV_TMP_SIZE] = {0}, recv_returncode_buf[RECV_TMP_SIZE] = {0};

		r_memset(&recv_module_xml_t, 0, sizeof(parse_xml_t));
		if(NULL == init_dom_tree(&recv_module_xml_t, xml_buf)) {
        	nslog(NS_ERROR, "init_dom_tree\n*************start***********\n%s\n*************end**********", xml_buf);
			return return_code;
    	}
		r_msghead_node = get_children_node(recv_module_xml_t.proot, RECV_MSGHEAD);
		if (NULL == r_msghead_node) {
			nslog(NS_ERROR, "get_children_node error, errno = %d", errno);
			goto EXIT;
		}
		r_msgcode_node = get_children_node(r_msghead_node, RECV_MSGCODE);
		if (NULL == r_msghead_node) {
			nslog(NS_ERROR, "get_children_node error, errno = %d", errno);
			goto EXIT;
		}
		r_returncode_node = get_children_node(r_msghead_node, RECV_RETURNCODE);
		if (NULL == r_returncode_node) {
			nslog(NS_ERROR, "get_children_node error, errno = %d", errno);
			goto EXIT;
		}
		if (OPERATION_ERR == get_current_node_value(recv_tmp_buf, RECV_TMP_SIZE, recv_module_xml_t.pdoc, r_msgcode_node))
		{
			nslog(NS_ERROR, "get_current_node_value r_msgcode_node error, errno = %d", errno);
			goto EXIT;
		}
		if (OPERATION_ERR == get_current_node_value(recv_returncode_buf, RECV_TMP_SIZE, recv_module_xml_t.pdoc, r_returncode_node))
		{
			nslog(NS_ERROR, "get_current_node_value r_returncode_node error, errno = %d", errno);
			goto EXIT;
		}
		if (RECV_LOGIN_MSGCODE != atoi(recv_tmp_buf)) {

#if 0 
			   if (RECV_ENC_MAX_LOGIN == atoi(recv_tmp_buf)) {
						return_code = MAX_CONNECT;	
			   }
#endif
				goto EXIT;
		}
		if (RECV_LOGIN_SUC != atoi(recv_returncode_buf)) {
				goto EXIT;
		}

		return_code = OPERATION_SUCC;
EXIT:
		release_dom_tree(recv_module_xml_t.pdoc);
		return return_code;
}


int32_t recv_xml_analyze_msgcode(stream_handle *M_handle, int8_t *xml_buf)
{
		int32_t return_code = OPERATION_SUCC;
		parse_xml_t recv_module_xml_t;
		xmlNodePtr r_msghead_node = NULL, r_msgcode_node = NULL;
		int8_t recv_tmp_buf[RECV_TMP_SIZE] = {0};

		r_memset(&recv_module_xml_t, 0, sizeof(parse_xml_t));
		if(NULL == init_dom_tree(&recv_module_xml_t, xml_buf)) {
        	nslog(NS_ERROR, "init_dom_tree errno = %d", errno);
			return return_code;
    	}
		r_msghead_node = get_children_node(recv_module_xml_t.proot, RECV_MSGHEAD);
		if (NULL == r_msghead_node) {
			nslog(NS_ERROR, "get_children_node error, errno = %d", errno);
			goto EXIT;
		}
		r_msgcode_node = get_children_node(r_msghead_node, RECV_MSGCODE);
		if (NULL == r_msghead_node) {
			nslog(NS_ERROR, "get_children_node error, errno = %d", errno);
			goto EXIT;
		}
		if (OPERATION_ERR == get_current_node_value(recv_tmp_buf, RECV_TMP_SIZE, recv_module_xml_t.pdoc, r_msgcode_node))
		{
			nslog(NS_ERROR, "get_current_node_value r_msgcode_node error, errno = %d", errno);
			goto EXIT;
		}
		if (RECV_ENC_REPORT_MSGCODE != atoi(recv_tmp_buf)) {
				goto EXIT;
		}  
		return_code = OPERATION_CONTINUE;
EXIT:
		release_dom_tree(recv_module_xml_t.pdoc);
		return return_code;
}

//test request stream
int32_t receive_request_stream_xml(recv_xml_handle *xml_hand, int8_t *out_buf, int32_t *out_len, int32_t  stream_id)
{
    int32_t return_code = OPERATION_ERR;
    parse_xml_t recv_module_xml_t;
    xmlNodePtr New_node, New_node2;
	xmlChar *temp_xml_buf = NULL;
	char recv_tmp_buf[XML_MSGCODE_LEN] = {0};

    recv_module_xml_t.pdoc = xmlNewDoc(BAD_CAST"1.0");

    if(NULL == recv_module_xml_t.pdoc) {
        nslog(NS_ERROR, "xmlNewDoc failed");
        return_code = OPERATION_ERR;
        return  return_code;
    }

    recv_module_xml_t.proot = xmlNewNode(NULL, BAD_CAST"RequestMsg");
    xmlDocSetRootElement(recv_module_xml_t.pdoc, recv_module_xml_t.proot);
    New_node =  xmlNewNode(NULL, BAD_CAST "MsgHead");
    xmlAddChild(recv_module_xml_t.proot, New_node);
    xmlNewTextChild(New_node, NULL, "MsgCode", "30011");
    xmlNewTextChild(New_node, NULL, "PassKey", "RecServer");
    New_node2 =  xmlNewNode(NULL, BAD_CAST "MsgBody");
    xmlAddChild(recv_module_xml_t.proot, New_node2);
	New_node =  xmlNewNode(NULL, BAD_CAST "StrmReq");
	xmlAddChild(New_node2, New_node);
	xmlNewTextChild(New_node, NULL, "RoomID", "0");
	sprintf(recv_tmp_buf, "%d", ((stream_id - 1) % 2));
	xmlNewTextChild(New_node, NULL, "Quality", recv_tmp_buf);
	xmlNewTextChild(New_node, NULL, "EncodeIndex", "123");
	xmlNewTextChild(New_node, NULL, "Ipaddr1", "192.168.4.123");
	xmlNewTextChild(New_node, NULL, "Port1", "22");
	xmlNewTextChild(New_node, NULL, "Ipaddr2", "192.168.4.123");
	xmlNewTextChild(New_node, NULL, "Port2", "33");
	xmlNewTextChild(New_node, NULL, "Ipaddr3", "192.168.4.123");
	xmlNewTextChild(New_node, NULL, "Port3", "44");
	
   // xmlNewTextChild(New_node, NULL, "PassKey", "RecServer");
   // save file test.
    //return_code= xmlSaveFormatFileEnc("ReceiveLog.xml", recv_module_xml_t.pdoc, "UTF-8", 1);
    xmlDocDumpFormatMemoryEnc(recv_module_xml_t.pdoc, &temp_xml_buf, out_len,  "UTF-8", 1);
    r_memcpy(out_buf, temp_xml_buf, *out_len);

    release_dom_tree(recv_module_xml_t.pdoc);
    return_code = OPERATION_SUCC;
EXIT:
	 if(temp_xml_buf)  {
       xmlFree(temp_xml_buf);
    }
    return return_code;
}



