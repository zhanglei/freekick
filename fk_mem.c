#include <stdio.h>
#include <stdlib.h>

#include <unistd.h>

#include <jemalloc/jemalloc.h>

#include <fk_def.h>

/*
#ifdef __APPLE__
#include <malloc/malloc.h>
#else
#ifdef __linux__
#include <malloc.h>
#endif
#endif
*/

#include <fk_mem.h>

static size_t allocated = 0;
static size_t freed = 0;
static size_t alloc_times = 0;
static size_t free_times = 0;

static void fk_mem_panic()
{
	fprintf(stderr, "out of memory");
	fflush(stderr);
	sleep(1);
	abort();
}

void *fk_mem_alloc(size_t size)
{
	void *ptr;
	size_t real_size;

	ptr = malloc(size);
	if (ptr == NULL) {
		fk_mem_panic();
	}
//#ifdef __APPLE__
		//real_size = malloc_size(ptr);
//#else
		real_size = malloc_usable_size(ptr);
//#endif
		allocated += real_size;
	alloc_times += 1;
	return ptr;
}

void *fk_mem_calloc(size_t count, size_t size)
{
	void *ptr;
	size_t real_size;

	ptr = calloc(count, size);
	if (ptr == NULL) {
		fk_mem_panic();
	}
//#ifdef __APPLE__
		//real_size = malloc_size(ptr);
//#else
		real_size = malloc_usable_size(ptr);
//#endif
		allocated += real_size;
	alloc_times += 1;
	return ptr;
}

void *fk_mem_realloc(void *ptr, size_t size)
{
	void *new_ptr;
	size_t old_size, new_size;

//#ifdef __APPLE__
	//old_size = malloc_size(ptr);
//#else
	old_size = malloc_usable_size(ptr);
//#endif

	new_ptr = realloc(ptr, size);
	if (new_ptr == NULL) {
		fk_mem_panic();
	}

//#ifdef __APPLE__
	//new_size = malloc_size(ptr);
//#else
	new_size = malloc_usable_size(ptr);
//#endif
	allocated = allocated - old_size + new_size;

	return new_ptr;
}

void fk_mem_free(void *ptr)
{
	size_t real_size;
//#ifdef __APPLE__
	//real_size = malloc_size(ptr);
//#else
	real_size = malloc_usable_size(ptr);
//#endif
	allocated -= real_size;
	freed += real_size;
	free_times += 1;
	// how to get the size of the freeing memory???????
	free(ptr);
}

size_t fk_mem_allocated()
{
	return allocated;
}

size_t fk_mem_alloc_times()
{
	return alloc_times;
}

size_t fk_mem_free_times()
{
	return free_times;
}
