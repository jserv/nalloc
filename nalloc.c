/**
 * Nalloc is a replacement for the standard memory allocation routines that
 * provides structure aware allocations.
 *
 * Each chunk of nalloc'ed memory has a header of the following form:
 *
 * +---------+---------+---------+--------···
 * |  first  |  next   |  prev   | memory
 * |  child  | sibling | sibling | chunk
 * +---------+---------+---------+--------···
 *
 * Thus, a nalloc hierarchy tree would look like this:
 *
 *   NULL <-- chunk --> NULL
 *              ^
 *              |
 *              +-> chunk <--> chunk <--> chunk --> NULL
 *                    |          |          ^
 *                    v          v          |
 *                   NULL       NULL        +-> chunk <--> chunk --> NULL
 *                                                |          |
 *                                                v          v
 *                                               NULL       NULL
 */

#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "nalloc.h"
#include "util.h"

/**
 * Nalloc tree node helpers.
 */

#define HEADER_SIZE (sizeof(void *) * 3)

#define raw2usr(mem) (void *)((void **)(mem) + 3)
#define usr2raw(mem) (void *)((void **)(mem)-3)
#define child(mem) (((void **)(mem))[-3])
#define next(mem) (((void **)(mem))[-2])
#define prev(mem) (((void **)(mem))[-1])
#define parent(mem) prev(mem) /* Valid only when is_first(mem) */
#define is_root(mem) (!prev(mem))
#define is_first(mem) (next(prev(mem)) != (mem))

/**
 * Initialize a raw chunk of memory.
 *
 * @param mem     pointer to a raw memory chunk.
 * @param parent  pointer to previously nalloc'ed memory chunk from which this
 *                chunk depends, or NULL.
 *
 * @return pointer to the allocated memory chunk, or NULL if there was an error.
 */
static inline void *nalloc_init(void *mem, void *parent)
{
	if (unlikely(!mem))
		return NULL;

	memset(mem, 0, HEADER_SIZE);
	mem = raw2usr(mem);

	nalloc_set_parent(mem, parent);
	return mem;
}

EXPORT
void *nalloc(size_t size, void *parent)
{
	return nalloc_init(malloc(size + HEADER_SIZE), parent);
}

EXPORT
void *ncalloc(size_t size, void *parent)
{
	return nalloc_init(calloc(1, size + HEADER_SIZE), parent);
}

EXPORT
void *nrealloc(void *usr, size_t size)
{
	void *mem = realloc(usr ? usr2raw(usr) : NULL, size + HEADER_SIZE);

	if (unlikely(!usr || !mem))
		return nalloc_init(mem, NULL);

	mem = raw2usr(mem);

	/* If the buffer starting address changed, update all references. */
	if (mem != usr) {
		if (child(mem))
			parent(child(mem)) = mem;

		if (!is_root(mem)) {
			if (next(mem))
				prev(next(mem)) = mem;

			if (next(prev(mem)) == usr)
				next(prev(mem)) = mem;

			if (child(parent(mem)) == usr)
				child(parent(mem)) = mem;
		}
	}

	return mem;
}

/**
 * Deallocate all the descendants of parent(mem) recursively.
 *
 * @param mem  pointer to previously nalloc'ed memory chunk.
 */
static inline void __nfree(void *mem)
{
	if (unlikely(!mem))
		return;

	/* Fail if the tree hierarchy has cycles. */
	assert(prev(mem));
	prev(mem) = NULL;

	__nfree(child(mem));
	__nfree(next(mem));
	free(usr2raw(mem));
}

EXPORT
void *nfree(void *mem)
{
	if (unlikely(!mem))
		return NULL;

	nalloc_set_parent(mem, NULL);

	__nfree(child(mem));
	free(usr2raw(mem));

	return NULL;
}

EXPORT
void *nalloc_get_parent(const void *mem)
{
	if (unlikely(!mem || is_root(mem)))
		return NULL;

	while (!is_first(mem))
		mem = prev(mem);

	return parent(mem);
}

EXPORT
void nalloc_set_parent(void *mem, void *parent)
{
	if (unlikely(!mem))
		return;

	if (!is_root(mem)) {
		/* Remove node from old tree. */
		if (next(mem))
			prev(next(mem)) = prev(mem);

		if (!is_first(mem))
			next(prev(mem)) = next(mem);
		else
			child(parent(mem)) = next(mem);
	}

	next(mem) = prev(mem) = NULL;

	if (parent) {
		/* Insert node into new tree. */

		if (child(parent)) {
			next(mem) = child(parent);
			prev(child(parent)) = mem;
		}

		parent(mem) = parent;
		child(parent) = mem;
	}
}

EXPORT
void nalloc_cut(void *mem, void *parent)
{
	if (unlikely(!mem))
		return;

	nalloc_set_parent(mem, NULL);

	if (!child(mem))
		return;

	if (parent) {
		/* Insert mem children in front of the list of parent children. */
		if (child(parent)) {
			void *last = child(mem);

			while (next(last))
				last = next(last);

			prev(child(parent)) = last;
			next(last) = child(parent);
		}

		child(parent) = child(mem);
	}

	parent(child(mem)) = parent;
	child(mem) = NULL;
}
