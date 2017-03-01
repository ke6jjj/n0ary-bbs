#ifndef MAKECB_ULS_SLAB_H
#define MAKECB_ULS_SLAB_H

struct slab;
typedef struct slab *slab_t;

int slab_init(size_t item_size, size_t alloc_quantum, slab_t *out);
void *slab_alloc(slab_t slab);
void slab_free(slab_t slab);

#endif
