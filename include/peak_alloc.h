/*
 * Copyright (c) 2012-2014 Franco Fichtner <franco@packetwerk.com>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#ifndef PEAK_ALLOC_H
#define PEAK_ALLOC_H

#include <stdlib.h>
#include <string.h>

#define ALLOC_CACHEALIGN(x)	ALLOC_ALIGN(x, ALLOC_CACHELINE)
#define ALLOC_ALIGN(x, y)	((x) + (y) - ((x) % (y) ? (x) % (y) : (y)))
#define ALLOC_CACHELINE		64

#define ALLOC_HEALTHY		0
#define ALLOC_UNDERFLOW		1
#define ALLOC_OVERFLOW		2

#define ALLOC_ERROR(x) do {						\
	switch (x) {							\
	case ALLOC_HEALTHY:						\
		break;							\
	case ALLOC_UNDERFLOW:						\
		panic("buffer underflow detected\n");			\
		/* NOTREACHED */					\
	case ALLOC_OVERFLOW:						\
		panic("buffer overflow detected\n");			\
		/* NOTREACHED */					\
	}								\
} while (0)

#define ALLOC_PAD(name)		(sizeof(struct peak_##name##_head) +	\
    sizeof(uint64_t))
#define ALLOC_MAGIC(x)		(((uint64_t *)(x)) - 1)
#define ALLOC_HEAD(name, x)	(((struct peak_##name##_head *)(x)) - 1)
#define ALLOC_USER(x)		(x)->user
#define ALLOC_SIZE(x)		(x)->size
#define ALLOC_TAIL(x)		(ALLOC_USER(x) + ALLOC_SIZE(x))
#define ALLOC_VALUE(name)	name##_VALUE

#define ALLOC_INIT(name, x, y) do {					\
	ALLOC_SIZE(x) = y;						\
	le64enc(&(x)->magic, ALLOC_VALUE(name));			\
	le64enc(ALLOC_TAIL(x), ALLOC_VALUE(name));			\
} while (0)

#define malign_VALUE		0x9B97A6B5C4D3E2F1ull
#define malloc_VALUE		0xD12E3D4C5B6A7989ull

struct peak_malloc_head {
	uint64_t size;
	uint64_t magic;
	uint8_t user[];
};

struct peak_malign_head {
	uint64_t size;
	uint64_t _0[6];
	uint64_t magic;
	uint8_t user[];
};

static inline void *
_peak_finalise_malloc(const uint64_t size, struct peak_malloc_head *h)
{
	ALLOC_INIT(malloc, h, size);

	return (ALLOC_USER(h));
}

static inline unsigned int
_peak_mcheck_malloc(void *ptr)
{
	unsigned int ret = ALLOC_HEALTHY;
	struct peak_malloc_head *h;

	h = ALLOC_HEAD(malloc, ptr);

	if (ALLOC_VALUE(malloc) != le64dec(ALLOC_TAIL(h))) {
		ret = ALLOC_OVERFLOW;
	}

	return (ret);
}

static inline void *
peak_malloc(size_t size)
{
	void *ptr;

	if (!size) {
		return (NULL);
	}

	if (size > SIZE_MAX - ALLOC_PAD(malloc)) {
		/* do not overflow */
		return (NULL);
	}

	ptr = malloc(size + ALLOC_PAD(malloc));
	if (!ptr) {
		return (NULL);
	}

	return (_peak_finalise_malloc(size, ptr));
}

/*
 * This is sqrt(SIZE_MAX+1), as s1*s2 <= SIZE_MAX
 * if both s1 < MUL_NO_OVERFLOW and s2 < MUL_NO_OVERFLOW
 */
#define MUL_NO_OVERFLOW (1UL << (sizeof(size_t) * 4))

static inline void *
peak_calloc(size_t count, size_t size)
{
	void *ptr;

	if ((count >= MUL_NO_OVERFLOW || size >= MUL_NO_OVERFLOW) &&
	    count > 0 && SIZE_MAX / count < size) {
		return (NULL);
	}

	size *= count;

	ptr = peak_malloc(size);
	if (ptr) {
		memset(ptr, 0, size);
	}

	return (ptr);
}

static inline void *
_peak_realloc(void *ptr, size_t size)
{
	struct peak_malloc_head *h = NULL;
	void *new_ptr = NULL;
	size_t old_size = 0;

	if (ptr) {
		h = ALLOC_HEAD(malloc, ptr);
		old_size = h->size;
	}

	if (size) {
		new_ptr = peak_malloc(size);
		if (!new_ptr) {
			return (NULL);
		}

		size = old_size < size ? old_size : size;
		if (size) {
			memcpy(new_ptr, ALLOC_USER(h), size);
		}
	}

	free(h);

	return (new_ptr);
}

#define peak_realloc(x, y) ({						\
	if (x) {							\
		ALLOC_ERROR(_peak_mcheck_malloc(x));			\
	}								\
	_peak_realloc(x, y);						\
})

static inline void *
_peak_reallocarray(void *ptr, size_t count, size_t size)
{
	if ((count >= MUL_NO_OVERFLOW || size >= MUL_NO_OVERFLOW) &&
	    count > 0 && SIZE_MAX / count < size) {
		return (NULL);
	}

	size *= count;

	return (_peak_realloc(ptr, size));
}

#define peak_reallocarray(x, y, z) ({					\
	if (x) {							\
		ALLOC_ERROR(_peak_mcheck_malloc(x));			\
	}								\
	_peak_reallocarray(x, y, z);					\
})

static inline char *
peak_strdup(const char *s1)
{
	size_t size;
	char *s2;

	size = strsize(s1);
	s2 = peak_malloc(size);
	if (s2) {
		strcpy(s2, s1);
	}

	return (s2);
}

static inline void *
_peak_finalise_malign(const uint64_t size, struct peak_malign_head *h)
{
	ALLOC_INIT(malign, h, size);

	return (ALLOC_USER(h));
}

static inline unsigned int
_peak_mcheck_malign(void *ptr)
{
	unsigned int ret = ALLOC_HEALTHY;
	struct peak_malign_head *h;

	h = ALLOC_HEAD(malign, ptr);

	if (ALLOC_VALUE(malign) != le64dec(ALLOC_TAIL(h))) {
		ret = ALLOC_OVERFLOW;
	}

	return (ret);
}

static inline void *
peak_malign(size_t count, size_t size)
{
	void *ptr = NULL;

	if (!size) {
		return (NULL);
	}

	if ((count >= MUL_NO_OVERFLOW || size >= MUL_NO_OVERFLOW) &&
	    count > 0 && SIZE_MAX / count < size) {
		return (NULL);
	}

	size *= count;

	if (size > SIZE_MAX - ALLOC_CACHELINE - ALLOC_PAD(malign)) {
		/* still an overflow */
		return (NULL);
	}

	if (posix_memalign(&ptr, ALLOC_CACHELINE,
	    ALLOC_CACHEALIGN(size) + ALLOC_PAD(malign))) {
		return (NULL);
	}

	return (_peak_finalise_malign(size, ptr));
}

static inline unsigned int
__peak_free(void *ptr, const unsigned int really_free)
{
	unsigned int ret = ALLOC_HEALTHY;

	if (!ptr) {
		return (ret);
	}

	switch (le64dec(ALLOC_MAGIC(ptr))) {
	case ALLOC_VALUE(malloc):
		ret = _peak_mcheck_malloc(ptr);
		if (really_free && !ret) {
			free(ALLOC_HEAD(malloc, ptr));
		}
		break;
	case ALLOC_VALUE(malign):
		ret = _peak_mcheck_malign(ptr);
		if (really_free && !ret) {
			free(ALLOC_HEAD(malign, ptr));
		}
		break;
	default:
		ret = ALLOC_UNDERFLOW;
		break;
	}

	return (ret);
}

#undef malign_VALUE
#undef malloc_VALUE

#undef ALLOC_TAIL_OK
#undef ALLOC_VALUE
#undef ALLOC_MAGIC
#undef ALLOC_USER
#undef ALLOC_SIZE
#undef ALLOC_INIT
#undef ALLOC_HEAD
#undef ALLOC_TAIL
#undef ALLOC_PAD

#define _peak_mcheck(x)	__peak_free(x, 0)
#define _peak_free(x)	__peak_free(x, 1)

#define peak_mcheck(x)	ALLOC_ERROR(_peak_mcheck(x))
#define peak_free(x)	ALLOC_ERROR(_peak_free(x))

#ifndef WANT_UNGUARDED

/* pave over all standard functions */
#define realloc		peak_realloc
#define reallocarray	peak_reallocarray
#define malign		peak_malign
#define malloc		peak_malloc
#define calloc		peak_calloc
#undef strdup		/* Linux, geez */
#define strdup		peak_strdup
#define mcheck		peak_mcheck
#define free		peak_free

#else /* WANT_UNGUARDED */

static inline void *
malign(size_t count, size_t size)
{
	void *ptr = NULL;

	if (!size) {
		return (NULL);
	}

	if ((count >= MUL_NO_OVERFLOW || size >= MUL_NO_OVERFLOW) &&
	    count > 0 && SIZE_MAX / count < size) {
		return (NULL);
	}

	size *= count;

	if (size > SIZE_MAX - ALLOC_CACHELINE) {
		/* still an overflow */
		return (NULL);
	}

	if (posix_memalign(&ptr, ALLOC_CACHELINE, ALLOC_CACHEALIGN(size))) {
		return (NULL);
	}

	return (ptr);
}

#ifndef __OpenBSD__
static inline void *
reallocarray(void *ptr, size_t count, size_t size)
{
	if ((count >= MUL_NO_OVERFLOW || size >= MUL_NO_OVERFLOW) &&
	    count > 0 && SIZE_MAX / count < size) {
		return (NULL);
	}

	size *= count;

	return (realloc(ptr, size));
}
#endif /* !__OpenBSD__ */

#define mcheck(x)	do { (void)x; } while (0)

#endif /* !WANT_UNGUARDED */

#endif /* !PEAK_ALLOC_H */
