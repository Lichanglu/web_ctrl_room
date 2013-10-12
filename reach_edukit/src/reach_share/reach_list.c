#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "reach_os.h"
#include "reach_list.h"
/**
	插入节点
*/
int append_node(node_t **head, void *data, void *info)
{
	node_t *temp = *head;

	if(NULL == *head) {
		*head = (node_t *)r_malloc(sizeof(node_t));
		(*head)->data = data;
		(*head)->info = info;
		(*head)->next = NULL;
	} else {
		while(temp->next != NULL) {
			temp = temp->next;
		}

		temp->next = (node_t *)r_malloc(sizeof(node_t));
		temp =  temp->next;
		temp->data = data;
		temp->info = info;
		temp->next = NULL;
	}

	return 0;
}

void del_head_node(node_t **head)
{
	node_t *tmp_next = NULL;
	node_t *free_node = *head;

	if(NULL == free_node->next) {
		return;
	}

	tmp_next =  free_node->next;
	r_free(free_node);
	free_node = NULL;
	*head = tmp_next;
}

void del_part_list(node_t **head, node_t *curnode)
{
	node_t *temp = *head;

	while(curnode != temp && NULL != temp) {
		temp =  temp->next;
		free(*head);
		*head = temp;
	}

	*head = (node_t *)curnode->next;
	//nslog(NS_INFO, "[del_part_list]++++++++++++++++++ temp : [%p] *head : [%p]\n", t);
	r_free(curnode);
	curnode = NULL;
}

int list_length(node_t *head)
{
	int i = 0;

	while(head != NULL) {
		++ i;
		head =  head->next;
	}

	return i;
}

node_t *get_num(node_t *head, int  num)
{
	num --;

	while(head != NULL) {
		if(0 == num --) {
			return head;
		}

		head =  head->next;
	}

	return NULL;
}


/**
	打印节点信息
*/
void print_list(node_t *head, void (* print)(void *))
{
	while(head != NULL) {
		print(head->data);
		head =  head->next;
	}
}

/**
	销毁链表
*/
void destroy_list(node_t *head)
{
	node_t *temp = head;

	while(temp != NULL) {
		temp =  temp->next;
		free(head);
		head = temp;
	}
}

