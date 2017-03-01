#include <stdlib.h>
#include <stdint.h>

#include "slab.h"

struct chunk {
	void *mem;
	size_t next_item;
	size_t total_items;
	struct chunk *next;
};

struct slab {
	size_t item_size;
	size_t quanta;
	struct chunk *chunks_head;
};

int
slab_init(size_t item_sz, size_t quanta, slab_t *result)
{
	struct slab *sl = (struct slab *) malloc(sizeof(struct slab));
	if (sl == NULL)
		return -1;

	sl->chunks_head = NULL;
	sl->item_size = item_sz;
	sl->quanta = quanta;

	*result = sl;

	return 0;
}

void *
slab_alloc(slab_t sl)
{
	struct chunk *chk;
	void *chunk_mem, *item;

	if (sl->chunks_head == NULL ||
            sl->chunks_head->next_item == sl->chunks_head->total_items) {
		chunk_mem = malloc(sl->item_size * sl->quanta);
		if (chunk_mem == NULL)
			goto BadMemAlloc;

		chk = (struct chunk *) malloc(sizeof(struct chunk));
		if (chk == NULL)
			goto BadChunkAlloc;

		chk->mem = chunk_mem;
		chk->next_item = 0;
		chk->total_items = sl->quanta;
		chk->next = sl->chunks_head;
		sl->chunks_head = chk;
	}

	chk = sl->chunks_head;

	item = chk->mem + (sl->item_size * chk->next_item);
	chk->next_item++;

	return item;

BadChunkAlloc:
	free(chunk_mem);
BadMemAlloc:
	return NULL;
}

void
slab_free(slab_t sl)
{
	struct chunk *chk;

	while (sl->chunks_head != NULL) {
		chk = sl->chunks_head;
		free(chk->mem);
		sl->chunks_head = chk->next;
		free(chk);
	}

	free(sl);
}
