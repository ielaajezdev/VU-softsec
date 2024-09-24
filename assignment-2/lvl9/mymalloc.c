#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>

#include "mymalloc.h"

#define ALLOC_GRANULARITY (sizeof(void *))

#define MMAP_SIZE_MIN 0x10000

#define ENTRY_FLAG_LAST 1L /* entry is last in block */
#define ENTRY_FLAG_USED 2L /* entry is allocated */
#define ENTRY_FLAG_MASK (ENTRY_FLAG_LAST | ENTRY_FLAG_USED)

struct entry {
	size_t size;
	struct entry *free_prev;
	struct entry *free_next;
	struct entry *block_prev;
};

static struct entry *free_first;

size_t size_blocks;

static int free_list_compare(struct entry *e1, struct entry *e2) {
	/* small entries first to reduce fragmentation */
	if ((e1->size & ~ENTRY_FLAG_MASK) < (e2->size & ~ENTRY_FLAG_MASK)) return -1;
	if ((e1->size & ~ENTRY_FLAG_MASK) > (e2->size & ~ENTRY_FLAG_MASK)) return 1;

	/* sort equally sized entries by address */
	if (e1 < e2) return -1;
	if (e1 > e2) return 1;
	return 0;
}

static void free_list_add(struct entry *entry) {
	struct entry *list, *list_prev = NULL;

	/* add to the free list, sorting by size then address */
	for (list = free_first; list; list = list->free_next) {
		if (free_list_compare(list, entry) > 0) break;
		list_prev = list;
	}

	entry->free_prev = list_prev;
	entry->free_next = list;
	if (list_prev) {
		list_prev->free_next = entry;
	} else {
		free_first = entry;
	}
	if (list) list->free_prev = entry;
}

static void free_list_remove(struct entry *entry) {
	/* remove reference from previous entry, if any */
	if (entry->free_prev) {
		entry->free_prev->free_next = entry->free_next;
	} else {
		free_first = entry->free_next;
	}

	/* remove reference from next entry, if any */
	if (entry->free_next) {
		entry->free_next->free_prev = entry->free_prev;
	}
}

static struct entry *alloc_block(size_t size) {
	struct entry *entry;

	/* round size up to a multiple of the minimum mmap size */
	size += MMAP_SIZE_MIN - size % MMAP_SIZE_MIN;

	/* allocate new block using mmap */
	entry = mmap(NULL, size, PROT_READ | PROT_WRITE,
		MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
	if (entry == MAP_FAILED) {
		perror("mmap failed");
		exit(-1);
	}
	entry->size = size | ENTRY_FLAG_LAST;
	entry->block_prev = NULL;

	size_blocks += size;

	/* add new entry to the free list */
	free_list_add(entry);

	return entry;
}

static struct entry *find_free(size_t size) {
	struct entry *entry = free_first;

	/* is there an existing entry that is large enough? */
	size += sizeof(struct entry);
	while (entry) {
		if ((entry->size & ~ENTRY_FLAG_MASK) >= size) return entry;
		entry = entry->free_next;
	}

	/* create a new free entry of sufficient size */
	return alloc_block(size);
}

static struct entry *get_block_next(struct entry *entry) {
	return (struct entry *) ((char *) entry + (entry->size & ~ENTRY_FLAG_MASK));
}

static void mark_used(struct entry *entry, size_t size) {
	struct entry *newentry;
	size_t newsize;

	/* remove from free list */
	free_list_remove(entry);

	/* split entry if there is space left */
	newsize = (entry->size & ~ENTRY_FLAG_MASK) - sizeof(struct entry) - size;
	if (newsize > sizeof(struct entry)) {
		/* create new entry */
		newentry = (struct entry *) ((char *) (entry + 1) + size);
		newentry->size = newsize | (entry->size & ENTRY_FLAG_LAST);

		/* add new entry to free list */
		free_list_add(newentry);

		/* add new entry to block list */
		newentry->block_prev = entry;
		if (!(newentry->size & ENTRY_FLAG_LAST)) {
			get_block_next(newentry)->block_prev = newentry;
		}

		/* truncate old block and clear flags */
		entry->size = sizeof(struct entry) + size;
	}

	/* block is now in use */
	entry->size |= ENTRY_FLAG_USED;
}

static void merge_with_next(struct entry *entry) {
	struct entry *nextentry;

	/* take next entry off free list */
	nextentry = get_block_next(entry);
	free_list_remove(nextentry);

	/* take next entry off block list */
	if (!(nextentry->size & ENTRY_FLAG_LAST)) {
		get_block_next(nextentry)->block_prev = entry;
	}

	/* add next entry and copy its last flag */
	entry->size = (entry->size & ~ENTRY_FLAG_MASK) + nextentry->size;
}

static struct entry *get_entry(void *p) {
	return (struct entry *) p - 1;
}

static size_t get_size(void *p) {
	if (!p) return 0;
	return get_entry(p)->size & ~ENTRY_FLAG_MASK;
}

void *mymalloc(size_t size) {
	struct entry *entry;

	if (!size) return NULL;

	/* round allocation size up to granularity */
	if (size % ALLOC_GRANULARITY) {
		size += ALLOC_GRANULARITY - size % ALLOC_GRANULARITY;
	}

	/* find entry of sufficient size */
	entry = find_free(size);

	/* remove entry from the free list */
	mark_used(entry, size);

	return entry + 1;
}

void *myrealloc(void *p, size_t size) {
	void *pnew;
	size_t sizecommon, sizeold;

	pnew = mymalloc(size);
	sizeold = get_size(p);
	sizecommon = (size < sizeold) ? size : sizeold;

	if (sizecommon > 0) memcpy(pnew, p, sizecommon);

	myfree(p);
	return pnew;
}

void myfree(void *p) {
	struct entry *entry;

	if (!p) return;

	/* recover pointer to struct entry */
	entry = get_entry(p);

	/* mark entry as free and add it to the free list */
	entry->size &= ~ENTRY_FLAG_USED;
	free_list_add(entry);

	/* if possible, merge with next entry in block */
	if (!(entry->size & ENTRY_FLAG_LAST) &&
		!(get_block_next(entry)->size & ENTRY_FLAG_USED)) {
		merge_with_next(entry);
	}

	/* if possible, merge with previous entry in block */
	if (entry->block_prev &&
		!(entry->block_prev->size & ENTRY_FLAG_USED)) {
		merge_with_next(entry->block_prev);
	}
}
