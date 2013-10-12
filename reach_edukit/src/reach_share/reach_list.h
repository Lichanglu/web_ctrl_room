#ifndef _GEN_NOLOOP_SINGLE_
#define _GEN_NOLOOP_SINGLE_

typedef struct node {
	void *data;
	void *info;
	struct node *next;
} node_t;


int append_node(node_t **head, void *data, void *info);
void del_part_list(node_t **head, node_t *curnode);
node_t *get_num(node_t *head, int num);
int list_length(node_t *head);
void del_head_node(node_t **head);
/**
	Ïú»ÙÁ´±í
*/
void destroy_list(node_t *head);
void print_list(node_t *head, void (* print)(void *data));

#endif /* _GEN_NOLOOP_SINGLE_ */

