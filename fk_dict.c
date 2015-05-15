#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <stdint.h>

#include <fk_dict.h>
#include <fk_mem.h>

#define FK_DICT_INIT_SIZE 4

#define fk_elt_create()	(fk_elt *)fk_mem_alloc(sizeof(fk_elt))
 
#define fk_elt_destroy(elt)	fk_mem_free(elt)

#define fk_elt_key_set(dct, elt, k)	{				\
	if ((dct)->eop->key_copy != NULL) {				\
		(elt)->key = (dct)->eop->key_copy((k));		\
	} else {										\
		(elt)->key = (k);							\
	}												\
}

#define fk_elt_value_set(dct, elt, v)	{			\
	if ((dct)->eop->val_copy != NULL) {				\
		(elt)->value = (dct)->eop->val_copy((v));	\
	} else {										\
		(elt)->value = (v);							\
	}												\
}

#define fk_elt_key_free(dct, elt)	{				\
	if ((dct)->eop->key_free != NULL) {				\
		(dct)->eop->key_free((elt)->key);			\
	}												\
	(elt)->key = NULL;								\
}

#define fk_elt_value_free(dct, elt)	{				\
	if ((dct)->eop->val_free != NULL) {				\
		(dct)->eop->val_free((elt)->value);			\
	}												\
	(elt)->value = NULL;							\
}

static uint32_t fk_dict_hash(fk_str *key);
static int fk_dict_stretch(fk_dict *dct);
static fk_elt *fk_dict_search(fk_dict *dct, fk_str *key, size_t *bidx);
static void fk_dict_init(fk_dict *dct);
static void fk_dict_clear(fk_dict *dct);
#ifdef FK_DEBUG
static void fk_dict_buckets_print(size_t idx, fk_elt_list *lst);
#endif

static fk_elt_op default_eop = {
	NULL,
	NULL,
	NULL,
	NULL
};

fk_dict *fk_dict_create(fk_elt_op *eop)
{
	fk_dict *dct;
	dct = (fk_dict *)fk_mem_alloc(sizeof(fk_dict));
	dct->eop = &default_eop;
	if (eop != NULL) {
		dct->eop = eop;
	}
	fk_dict_init(dct);

	return dct;
}

void fk_dict_init(fk_dict *dct)
{
	dct->size = FK_DICT_INIT_SIZE;
	dct->size_mask = dct->size - 1;
	dct->limit = dct->size / 2;/*when up to 50%, it should extend space */
	dct->used = 0;
	dct->buckets = (fk_elt_list **)fk_mem_alloc(sizeof(fk_elt_list) * FK_DICT_INIT_SIZE);
	bzero(dct->buckets, sizeof(fk_elt_list *) * FK_DICT_INIT_SIZE);
}

/*
 * free all the elements
 * free all the buckets
 */
void fk_dict_clear(fk_dict *dct)
{
	size_t i;
	fk_elt *nd;
	fk_elt_list *lst;

	for (i = 0; i < dct->size; i++) {
		lst = dct->buckets[i];
		if (lst == NULL) {
			continue;
		}
		nd = fk_rawlist_head(lst);/*get the new head*/
		while (nd != NULL) {
			fk_rawlist_any_remove(lst, nd);
			fk_elt_key_free(dct, nd);/*free key*/
			fk_elt_value_free(dct, nd);/*free value*/
			fk_elt_destroy(nd);/*free element*/
			dct->used--;/*unnecessary*/
			nd = fk_rawlist_head(lst);/*go to the new head*/
		}
		fk_rawlist_destroy(lst);/*free element list*/
		dct->buckets[i] = NULL;/*mark list NULL*/
	}
	fk_mem_free(dct->buckets);/*free buckets*/
	dct->buckets = NULL;/*mark bucket NULL*/
}

void fk_dict_empty(fk_dict *dct)
{
	fk_dict_clear(dct);/*only clear all the elements*/

	fk_dict_init(dct);/*go back to the initial state*/
}

void fk_dict_destroy(fk_dict *dct)
{
	fk_dict_clear(dct);

	fk_mem_free(dct);/*free dict itself*/
}

fk_elt *fk_dict_search(fk_dict *dct, fk_str *key, size_t *bidx)
{
	int cmp;
	size_t idx;
	fk_elt *nd;
	uint32_t hash;
	fk_elt_list *lst;

	hash = fk_dict_hash(key);
	idx = (size_t)hash & (dct->size_mask);
	if (bidx != NULL) {
		*bidx = idx;
	}
	lst = dct->buckets[idx];
	if (lst == NULL) {
		return NULL;
	}
	nd = fk_rawlist_head(lst);
	while (nd != NULL) {
		cmp = fk_str_cmp(nd->key, key);
		if (cmp == 0) {
			return nd;
		}
		nd = nd->next;
	}
	return NULL;
}

void *fk_dict_get(fk_dict *dct, fk_str *key)
{
	fk_elt *elt;

	elt = fk_dict_search(dct, key, NULL);
	if (elt == NULL){
		return NULL;
	}

	return elt->value;
}

/*
uint32_t fk_dict_hash(fk_str *key)
{
	char *s;
	uint32_t hash;

	s = fk_str_raw(key);
	hash = 0;
	while (*s) {
		hash = (hash << 5) + hash + (uint8_t) * s++;
	}

	printf("===hash: %lu\n", hash);
	return hash;
}
*/

uint32_t fk_dict_hash(fk_str *key)
{
	char *buf;
	size_t len;
    uint32_t hash;

	hash = 5381;
	len = fk_str_len(key);
	buf = fk_str_raw(key);
    while (len--) {
		hash = ((hash << 5) + hash) + (*buf++);
	}
    return hash;
}

/*
 * return value:
 * 1: key already exists
 * 0: key not exists yet
 */
int fk_dict_add(fk_dict *dct, fk_str *key, void *value)
{
	size_t idx;
	fk_elt *elt;
	fk_elt_list *lst;

	/*
	 * should be called before fk_dict_search
	 * if fk_dict_search called first, the idx 
	 * maybe need to be calculated once more
	 */
	if (dct->used == dct->limit) {
		fk_dict_stretch(dct);
	}

	elt = fk_dict_search(dct, key, &idx);
	if (elt != NULL) {
		return 1;
	}

	lst = dct->buckets[idx];
	if (lst == NULL) {
		lst = fk_rawlist_create(fk_elt_list);
		fk_rawlist_init(lst);
		dct->buckets[idx] = lst;
	}
	elt = fk_elt_create();
	fk_elt_key_set(dct, elt, key);
	fk_elt_value_set(dct, elt, value);

	fk_rawlist_head_insert(lst, elt);
	dct->used++;

	return 0;
}

/*
 * return value:
 * 0: key not exists yet
 * 1: key already exists
 */
int fk_dict_replace(fk_dict *dct, fk_str *key, void *value)
{
	size_t idx;
	fk_elt *elt;

	elt = fk_dict_search(dct, key, &idx);
	if (elt == NULL) {
		return fk_dict_add(dct, key, value);
	}
	/*free old value*/
	fk_elt_value_free(dct, elt);
	/*use new value to replace*/
	fk_elt_value_set(dct, elt, value);
	return 1;/*the key exist already*/
}

int fk_dict_remove(fk_dict *dct, fk_str *key)
{
	size_t idx;
	fk_elt *elt;
	fk_elt_list *lst;

	elt = fk_dict_search(dct, key, &idx);
	if (elt == NULL) {
		return -1;
	}

	lst = dct->buckets[idx];

	fk_rawlist_any_remove(lst, elt);
	fk_elt_key_free(dct, elt);
	fk_elt_value_free(dct, elt);
	fk_elt_destroy(elt);
	dct->used--;

	return 0;
}

int fk_dict_stretch(fk_dict *dct)
{
	fk_elt *nd;
	fk_str *key;
	uint32_t hash;
	size_t new_size, i, idx;
	fk_elt_list **bks, *lst;

	if (dct->used < dct->limit) {
		return 0;
	}
	new_size = dct->size << 1;
	bks = (fk_elt_list **)fk_mem_alloc(new_size * sizeof(fk_elt_list *));
	bzero(bks, sizeof(fk_elt_list *) * new_size);

	/*rehash to the new buckets*/
	for (i = 0; i < dct->size; i++) {
		lst = dct->buckets[i];
		if (lst == NULL) {
			continue;
		}
		nd = fk_rawlist_head(lst);
		while (nd != NULL) {
			fk_rawlist_any_remove(lst, nd);
			key = nd->key;
			hash = fk_dict_hash(key);
			idx = (size_t)hash & (new_size - 1);
			if (bks[idx] == NULL) {
				bks[idx] = fk_rawlist_create(fk_elt_list);
				fk_rawlist_init(bks[idx]);
			}
			fk_rawlist_head_insert(bks[idx], nd);
			nd = fk_rawlist_head(lst);
		}
	}

	/*free old buckets*/
	for (i = 0; i < dct->size; i++) {
		if (dct->buckets[i] != NULL) {
			fk_rawlist_destroy(dct->buckets[i]);
		}
	}
	fk_mem_free(dct->buckets);

	dct->buckets = bks;
	dct->size = new_size;
	dct->size_mask = new_size - 1;
	dct->limit = new_size / 2;

	return 0;
}

#ifdef FK_DEBUG
void fk_dict_buckets_print(size_t idx, fk_elt_list *lst)
{
	fk_elt *elt;

	printf("%lu: ", idx);
	elt = lst->head;
	while (elt != NULL) {
		printf("%s-->", elt->key->seq);
		elt = elt->next;
	}
	printf("\n");
}

void fk_dict_print(fk_dict *dct)
{
	size_t i;
	fk_elt_list *lst;

	printf("size: %lu, size_mask: %lu, limit: %lu, used: %lu\n", 
			dct->size, 
			dct->size_mask,
			dct->limit,
			dct->used
	);
	for (i = 0; i < dct->size; i++) {
		lst = dct->buckets[i];
		if (lst == NULL) {
			continue;
		}
		fk_dict_buckets_print(i, lst);
	}
}
#endif
