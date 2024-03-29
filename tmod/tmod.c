#include <sys/types.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>

#include <fk_mem.h>
#include <fk_pool.h>
#include <fk_dict.h>
#include <fk_item.h>
#include <fk_list.h>
#include <fk_skiplist.h>


typedef struct {
    int id;
    char b;
    int a;
} foo;

#define LEN 1024*1024*5

void t_pool()
{
    int i;
    void **a;
    clock_t t1, t2;

    a = malloc(sizeof(void *) * LEN);

    fk_pool *pool = fk_pool_create(sizeof(foo), 1024);
    t1 = clock();
    for (i=0; i<LEN; i++) {
        a[i] = fk_pool_malloc(pool);
    }
    t2 = clock();
    printf("time used for malloc: %lu\n", t2 - t1);

    t1 = clock();
    for (i=0; i<LEN; i++) {
        fk_pool_free(pool, a[i]);
    }
    t2 = clock();
    printf("time used for free: %lu\n", t2 - t1);

    t1 = clock();
    for (i=0; i<LEN; i++) {
        a[i] = malloc(sizeof(foo));
    }
    t2 = clock();
    printf("time used for malloc: %lu\n", t2 - t1);

    t1 = clock();
    for (i=0; i<LEN; i++) {
        free(a[i]);
    }
    t2 = clock();
    printf("time used for free: %lu\n", t2 - t1);
}

void t_dict()
{
	fk_elt *elt;
	fk_dict *dd = fk_dict_create();
	printf("11111111\n");
	fk_str *ss = fk_str_create("huge", 4);
	fk_str *gg = fk_str_create("ooxx", 4);
	fk_item *oo = fk_item_create(FK_ITEM_STR, gg);

	fk_dict_add(dd, ss, oo);

	fk_dict_iter *iter = fk_dict_iter_begin(dd);
	printf("22222222\n");
	while ((elt = fk_dict_iter_next(iter)) != NULL) {
		ss = (fk_str*)(elt->key);
		oo = (fk_item*)(elt->value);
		gg = (fk_str *)fk_item_raw(oo);
		printf("%s\n", fk_str_raw(ss));
		printf("%s\n", fk_str_raw(gg));
	}
	printf("33333333\n");
}

int int_cmp(void *a, void *b)
{
	int x = *((int*)a);
	int y = *((int*)b);
	return x - y;
}

fk_node_op oopp = {
	NULL,
	NULL,
	int_cmp
};

void t_list()
{
	int i;
	fk_node *nd;
	int x[10] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
	fk_list *ll = fk_list_create(&oopp);
	for (i = 0; i < 10; i++) {
		fk_list_insert_head(ll, x+i);
	}
	printf("list len: %zu\n", ll->len);
	fk_list_iter *iter = fk_list_iter_begin(ll, FK_LIST_ITER_H2T);
	nd = fk_list_iter_next(iter);
	while (nd != NULL) {
		printf("%d\n", *(int*)(nd->data));
		nd = fk_list_iter_next(iter);
	}
	fk_list_iter_end(iter);
}

void t_skiplist()
{
	int i, level;
	fk_skiplist *sl;
	fk_skipnode *p, *q;

	sl = fk_skiplist_create(NULL);

	fk_skiplist_insert(sl, 5, NULL);
	fk_skiplist_insert(sl, 3, NULL);
	fk_skiplist_insert(sl, 4, NULL);
	fk_skiplist_insert(sl, 11, NULL);
	fk_skiplist_insert(sl, 44, NULL);
	fk_skiplist_insert(sl, 24, NULL);
	fk_skiplist_insert(sl, 33, NULL);

	level = fk_skiplist_level(sl);
	printf("level: %d\n", level);

	for (i = level - 1; i >= 0; i--) {
		printf("%d---------\n", i);
		p = sl->head;
		q = p->next[i];
		while (q != NULL) {
			printf("%d\n", q->score);
			q = q->next[i];
		}
	}

	printf("remove node:\n");
	fk_skiplist_remove(sl, 44);
	fk_skiplist_remove(sl, 33);

	level = fk_skiplist_level(sl);
	printf("new level: %d\n", level);
	printf("after remove node:\n");
	for (i = level - 1; i >= 0; i--) {
		printf("%d---------\n", i);
		p = sl->head;
		q = p->next[i];
		while (q != NULL) {
			printf("%d\n", q->score);
			q = q->next[i];
		}
	}
	fk_skiplist_destroy(sl);
}

int main()
{
	//t_dict();	
	//t_list();
	t_skiplist();
    return 0;
}
