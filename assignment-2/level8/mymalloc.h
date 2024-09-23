#ifndef __MYMALLOC_H__
#define __MYMALLOC_H__

#include <stddef.h>

extern size_t size_blocks;

void *mymalloc(size_t size);
void *myrealloc(void *p, size_t size);
void myfree(void *p);

#endif /* !defined( __MYMALLOC_H__) */
