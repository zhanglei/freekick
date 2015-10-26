#ifndef _FK_SkLIST_H_
#define _FK_SkLIST_H_

#include <sys/types.h>

#define FK_SKLIST_MAX_LEVEL		16

typedef struct _fk_sknode {
	int score;/* the value to sort */
	void *data;/* hold the fk_item */
	struct _fk_sknode *next[1];/* at least 1 element */
} fk_sknode;

typedef struct _fk_sknode_op {
	void *(*data_copy)(void *);
	void (*data_free)(void *);
	void (*data_cmp)(void *, void *);
} fk_sknode_op;

typedef struct _fk_sklist {
	fk_sknode *head;
	int level;/* the max level of the nodes */
	size_t len;
	fk_sknode_op *skop;
} fk_sklist;

fk_sklist *fk_sklist_create(fk_sknode_op *skop);
void fk_sklist_destroy(fk_sklist *sl);
void fk_sklist_insert(fk_sklist *sl, int score, void *data);
void fk_sklist_remove(fk_sklist *sl, int score);
fk_sknode *fk_sklist_search(fk_sklist *sl, int score);

#define fk_sklist_len(sl)	((sl)->len)

#define fk_sklist_level(sl)	((sl)->level)

#endif
