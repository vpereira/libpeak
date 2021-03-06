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

#include <peak.h>
#include <assert.h>

#define UNALIGNED_16_ORIG	0x1234
#define UNALIGNED_16_NEW	0x2345
#define UNALIGNED_32_ORIG	0x12345678
#define UNALIGNED_32_NEW	0x23456789
#define UNALIGNED_64_ORIG	0x123456789ABCDEF0ull
#define UNALIGNED_64_NEW	0x23456789ABCDEF01ull

#define THIS_IS_A_STRING	"Hello, world!"

output_init();

static void
test_type(void)
{
	uint16_t test_val_16 = UNALIGNED_16_ORIG;
	uint32_t test_val_32 = UNALIGNED_32_ORIG;
	uint64_t test_val_64 = UNALIGNED_64_ORIG;

	peak_le16enc(&test_val_16, test_val_16);
	assert(peak_le16dec(&test_val_16) ==
	    peak_bswap16(peak_be16dec(&test_val_16)));
	peak_be16enc(&test_val_16, peak_bswap16(UNALIGNED_16_ORIG));
	assert(UNALIGNED_16_ORIG == test_val_16);

	peak_le32enc(&test_val_32, test_val_32);
	assert(peak_le32dec(&test_val_32) ==
	    peak_bswap32(peak_be32dec(&test_val_32)));
	peak_be32enc(&test_val_32, peak_bswap32(UNALIGNED_32_ORIG));
	assert(UNALIGNED_32_ORIG == test_val_32);

	peak_le64enc(&test_val_64, test_val_64);
	assert(peak_le64dec(&test_val_64) ==
	    peak_bswap64(peak_be64dec(&test_val_64)));
	peak_be64enc(&test_val_64, peak_bswap64(UNALIGNED_64_ORIG));
	assert(UNALIGNED_64_ORIG == test_val_64);

	test_val_16 = 0;
	assert(!wrap16(test_val_16));
	assert(wrap16(test_val_16 - 1));

	test_val_32 = 0;
	assert(!wrap32(test_val_32));
	assert(wrap32(test_val_32 - 1));

	test_val_64 = 0;
	assert(!wrap64(test_val_64));
	assert(wrap64(test_val_64 - 1));
}

static void
test_net(void)
{
	const struct netaddr ip4_ref = {
		.u.byte = {
			0x00, 0x00, 0x00, 0x00,
			0x00, 0x00, 0x00, 0x00,
			0x00, 0x00, 0xff, 0xff,
			0x11, 0x22, 0x33, 0x44,
		},
	};
	const struct netaddr ip6_ref = {
		.u.byte = {
			0xfe, 0x80, 0x00, 0x00,
			0x00, 0x00, 0x00, 0x00,
			0x12, 0x40, 0xf3, 0xff,
			0xfe, 0xa6, 0xd7, 0x62,
		},
	};
	uint32_t ipv4_addr;
	struct netaddr ip;
	int i;

	peak_be32enc(&ipv4_addr, 0x11223344);
	netaddr4(&ip, ipv4_addr);
	assert(!memcmp(&ipv4_addr, netto4(&ip), sizeof(ipv4_addr)));
	assert(!netcmp(&ip, &ip4_ref));
	assert(!netcmp4(&ip));
	assert(netcmp6(&ip));

	for (i = 1; i <= 32; ++i) {
		assert(!netprefix(&ip, &ip4_ref, NET_PREFIX4(i)));
	}

	assert(netprefix(&ip, &ip4_ref, NET_PREFIX4(33)));
	assert(netprefix(&ip, &ip4_ref, NET_PREFIX4(0)));

	netaddr6(&ip, &ip6_ref);
	assert(!memcmp(&ip6_ref, netto6(&ip), sizeof(ip6_ref)));
	assert(!netcmp(&ip, &ip6_ref));
	assert(netcmp4(&ip));
	assert(!netcmp6(&ip));

	for (i = 1; i <= 128; ++i) {
		assert(!netprefix(&ip, &ip6_ref, NET_PREFIX6(i)));
	}

	assert(netprefix(&ip, &ip6_ref, NET_PREFIX6(129)));
	assert(netprefix(&ip, &ip6_ref, NET_PREFIX6(0)));

	assert(netprefix(&ip4_ref, &ip6_ref, NET_PREFIX6(1)));
	peak_be32enc(&ipv4_addr, 0x11224433);
	netaddr4(&ip, ipv4_addr);
	assert(!netprefix(&ip, &ip4_ref, NET_PREFIX4(16)));
	assert(netprefix(&ip, &ip4_ref, NET_PREFIX4(17)));
}

struct interval {
	int left;
	int right;
};

static const struct interval test[2] = { { 2, 1 }, { 1, 2 } };

static int
test_stash_cmp(const void *xx, const void *yy)
{
	const struct interval *x = xx;
	const struct interval *y = yy;
	int ret;

	ret = x->left - y->left;
	if (ret) {
		return (ret);
	}

	return (x->right - y->right);
}

static void
_test_stash(stash_t ptr)
{
	STASH_REBUILD(stash, struct interval, ptr);
	struct interval *p;
	unsigned int i = 0;

	assert(STASH_EMPTY(stash));

	assert(STASH_PUSH(&test[0], stash));
	assert(STASH_PUSH(&test[1], stash));

	assert(!STASH_PUSH(&test[1], stash));
	assert(STASH_FULL(stash));

	STASH_FOREACH(p, stash) {
		assert(p->left == test[i].left &&
		    p->right == test[i].right);
		++i;
	}

	assert(STASH_COUNT(stash) == i);
}

static void
test_stash(void)
{
	STASH_DECLARE(stash, struct interval, 2);
	struct interval *p;
	unsigned int i = 0;

	assert(!STASH_POP(stash));

	_test_stash(stash);

	STASH_SORT(stash, test_stash_cmp);

	STASH_FOREACH_REVERSE(p, stash) {
		assert(p->left == test[i].left &&
		    p->right == test[i].right);
		++i;
	}

	assert(STASH_POP(stash));
	assert(STASH_POP(stash));

	assert(!STASH_POP(stash));
	assert(STASH_EMPTY(stash));
}

static void
test_alloc(void)
{
	uint64_t *test_ptr = peak_calloc(1, sizeof(*test_ptr));

	assert(!*test_ptr);

	peak_free(test_ptr);

	test_ptr = peak_malign(1, sizeof(*test_ptr));
	assert(test_ptr);
	memset(test_ptr, 0, sizeof(*test_ptr));
	assert(!*test_ptr);

	peak_free(test_ptr);

	assert(!peak_realloc(NULL, 0));

	test_ptr = peak_realloc(NULL, 7);
	uint64_t backup = *test_ptr;
	*test_ptr = 0;

	assert(ALLOC_OVERFLOW == _peak_mcheck(test_ptr));

	*test_ptr = backup;

	test_ptr = peak_realloc(test_ptr, 16);

	test_ptr[0] = 'o';
	test_ptr[1] = 'k';

	assert('o' == test_ptr[0]);
	assert('k' == test_ptr[1]);

	test_ptr = peak_reallocarray(test_ptr, 1, 24);

	assert('o' == test_ptr[0]);
	assert('k' == test_ptr[1]);

	test_ptr[2] = '!';

	test_ptr = peak_realloc(test_ptr, 8);

	assert('o' == test_ptr[0]);
	assert('k' != test_ptr[1]);

	assert(!peak_reallocarray(test_ptr, 1, 0));

	char *test_str = peak_strdup(THIS_IS_A_STRING);

	assert(!strcmp(test_str, THIS_IS_A_STRING));

	char *test_str_mod = test_str - 1;
	char test_char = *test_str_mod;

	*test_str_mod = 0;

	assert(ALLOC_UNDERFLOW == _peak_free(test_str));

	*test_str_mod = test_char;
	test_str_mod = test_str + strsize(test_str);
	test_char = *test_str_mod;
	*test_str_mod = 0;

	assert(ALLOC_OVERFLOW == _peak_free(test_str));

	*test_str_mod = test_char;

	peak_free(test_str);

	assert(ALLOC_CACHEALIGN(ALLOC_CACHELINE + 1) ==
	    2 * ALLOC_CACHELINE);
	assert(ALLOC_CACHEALIGN(ALLOC_CACHELINE - 1) == ALLOC_CACHELINE);
	assert(ALLOC_CACHEALIGN(ALLOC_CACHELINE) == ALLOC_CACHELINE);
	assert(ALLOC_CACHEALIGN(1) == ALLOC_CACHELINE);
	assert(ALLOC_CACHEALIGN(0) == 0);

	peak_free(peak_malign(1, 0));
	peak_free(peak_malign(1, 1));
	peak_free(peak_calloc(1, 0));
	peak_free(peak_calloc(1, 1));

	assert(!peak_calloc(2, (SIZE_MAX / 2) + 42));

	char *test_str_aligned = peak_malign(1, sizeof(THIS_IS_A_STRING));

	memcpy(test_str_aligned, THIS_IS_A_STRING,
	    sizeof(THIS_IS_A_STRING));

	char *test_str_mod_aligned = test_str_aligned - 1;
	char test_char_aligned = *test_str_mod_aligned;

	*test_str_mod_aligned = 0;

	assert(ALLOC_UNDERFLOW == _peak_mcheck(test_str_aligned));

	*test_str_mod_aligned = test_char_aligned;
	test_str_mod_aligned = test_str_aligned + sizeof(THIS_IS_A_STRING);
	test_char_aligned = *test_str_mod_aligned;
	*test_str_mod_aligned = 0;

	assert(ALLOC_OVERFLOW == _peak_mcheck(test_str_aligned));

	*test_str_mod_aligned = test_char_aligned;

	assert(ALLOC_HEALTHY == _peak_mcheck(test_str_aligned));

	peak_free(test_str_aligned);
}

static void
test_prealloc(void)
{
	prealloc_t test_mem;

	assert(prealloc_init(&test_mem, 1, 8));
	void *test_chunk = prealloc_gets(&test_mem);

	assert(test_chunk && !prealloc_gets(&test_mem));

	prealloc_puts(&test_mem, test_chunk);
	test_chunk = prealloc_gets(&test_mem);

	assert(_prealloc_exit(&test_mem));

	prealloc_puts(&test_mem, test_chunk);
	prealloc_exit(&test_mem);

	assert(prealloc_init(&test_mem, 8, sizeof(uint64_t)));

	uint64_t *test_chunks[8];
	unsigned int i;

	for (i = 0; i < 8; ++i) {
		test_chunks[i] = prealloc_gets(&test_mem);

		assert(test_chunks[i]);

		*test_chunks[i] = i;

		prealloc_put(&test_mem, test_chunks[i]);
		assert(prealloc_gets(&test_mem) == test_chunks[i]);

		*test_chunks[i] = i;
	}

	assert(!prealloc_gets(&test_mem));

	for (i = 0; i < 8; ++i) {
		assert(i == *test_chunks[i]);

		prealloc_puts(&test_mem, test_chunks[i]);
	}

	assert(!prealloc_valid(&test_mem, test_chunks));

	prealloc_exit(&test_mem);

	assert(!prealloc_init(&test_mem, 1, 151));
	assert(!prealloc_init(&test_mem, 2, ~0ULL));
}

static void
test_output(void)
{
	output_bug();

	if (0) {
		/* compile only */
		panic("test");
		alert("test");
		critical("test");
		error("test");
		warning("test");
		notice("test");
		info("test");
		debug("test");
	}
}

PAGE_DECLARE(test_page_head, int, 7);

#define TEST_PAGE_CMP(x, y)	(*(x) - *(y))
#define TEST_PAGE_MALLOC(x, y)	malloc(y)
#define TEST_PAGE_FREE(x, y)	free(y)
#define TEST_PAGE_NULL(x, y)	NULL

#define TEST_PAGE_SCMP(x, y)	((x)->value - (y)->value)

struct test_page_struct {
	int reserved;
	int value;
};

PAGE_DECLARE(test_page_single, struct test_page_struct, 1);

#define TEST_PAGE_LOOP	23

static int test_page_list[TEST_PAGE_LOOP] = {
	/*
	 * These test values are designed to go through
	 * all corner cases of a PAGE_INSERT() invoke.
	 * Don't mess with them unless you are sure what
	 * you are doing.
	 */
	2, 1, 20, 21, 19, 18, 17, 22, 0, 3, 4, 7,
	6, 5, 16, 15, 14, 13, 12, 11, 10, 8, 9,
};

static void
test_page(void)
{
	struct test_page_single *spage, *sroot = NULL;
	struct test_page_head *page, *root = NULL;
	struct test_page_struct *selem;
	struct test_page_struct wrap;
	int *elem;
	int i = 0;

	/* list size > 1 */

	PAGE_FOREACH(elem, page, root) {
		++i;
	}

	assert(!PAGE_MIN(elem, page, root));
	assert(!PAGE_MAX(elem, page, root));

	assert(!elem);
	assert(!i);

	assert(!PAGE_FIND(root, &test_page_list[0], TEST_PAGE_CMP));
	assert(!PAGE_INSERT(root, &test_page_list[0], TEST_PAGE_CMP,
	    TEST_PAGE_NULL, NULL));
	assert(PAGE_INSERT(root, &test_page_list[0], TEST_PAGE_CMP,
	    TEST_PAGE_MALLOC, NULL));
	assert(*PAGE_MIN(elem, page, root) == *PAGE_MAX(elem, page, root));

	assert(!PAGE_FIND(root, &test_page_list[1], TEST_PAGE_CMP));
	assert(PAGE_INSERT(root, &test_page_list[1], TEST_PAGE_CMP,
	    TEST_PAGE_MALLOC, NULL));
	assert(*PAGE_MIN(elem, page, root) != *PAGE_MAX(elem, page, root));

	for (i = 2; i < TEST_PAGE_LOOP; ++i) {
		assert(!PAGE_FIND(root, &test_page_list[i], TEST_PAGE_CMP));
		assert(PAGE_INSERT(root, &test_page_list[i],
		    TEST_PAGE_CMP, TEST_PAGE_MALLOC, NULL));
		assert(PAGE_INSERT(root, &test_page_list[i],
		    TEST_PAGE_CMP, TEST_PAGE_MALLOC, NULL));
	}

	i = 0;

	PAGE_FOREACH(elem, page, root) {
		assert(i++ == *elem);
	}

	PAGE_COLLAPSE(root, TEST_PAGE_FREE, NULL);
	assert(!root);

	/* flat linked list case */

	for (i = 0; i < TEST_PAGE_LOOP; ++i) {
		wrap.value = test_page_list[i];
		wrap.reserved = 0;

		assert(PAGE_INSERT(sroot, &wrap,
		    TEST_PAGE_SCMP, TEST_PAGE_MALLOC, NULL));
	}

	i = 0;

	PAGE_FOREACH(selem, spage, sroot) {
		assert(selem == PAGE_FIND(sroot, selem, TEST_PAGE_SCMP));
		assert(i++ == selem->value);
	}

	PAGE_COLLAPSE(sroot, TEST_PAGE_FREE, NULL);
	assert(!sroot);
}

static void
test_hash(void)
{
	ROLL_HEAD(whatever) head;
	uint32_t backup;

	assert(hash_fnv32("test", 4) == 0xAFD071E5u);
	assert(hash_fnv32(NULL, 0) == 0x811C9DC5u);

	assert(hash_joaat("test", 4) == 0x76705442u);
	assert(hash_joaat(NULL, 0) == 0x11C50567u);

	ROLL_INIT(&head, "test");
	assert(!ROLL_HASH(&head));
	ROLL_NEXT(&head);
	backup = ROLL_HASH(&head);
	assert(backup);
	ROLL_SHIFT(&head);
	assert(backup != ROLL_HASH(&head));
	ROLL_SHIFTN(&head, 2);
	assert(backup == ROLL_HASH(&head));

	ROLL_INIT(&head, "test");
	ROLL_NEXT(&head);
	ROLL_NEXT(&head);
	ROLL_NEXT(&head);
	ROLL_NEXT(&head);
	backup = ROLL_HASH(&head);

	assert(backup == hash_roll("test", 4));

	ROLL_INIT(&head, "testest");
	ROLL_NEXTN(&head, 4);
	assert(backup == ROLL_HASH(&head));
	ROLL_SHIFT(&head);
	assert(backup != ROLL_HASH(&head));
	ROLL_SHIFTN(&head, 2);
	assert(backup == ROLL_HASH(&head));

	assert(backup != hash_roll("tesT", 4));
}

static void
test_time(void)
{
	struct peak_timeval stackptr(now);
	timeslice_t stackptr(timer);

	TIMESLICE_INIT(timer);

	now->tv_sec = 42;
	now->tv_usec = 0;
	TIMESLICE_CALIBRATE(timer, now);

	now->tv_sec = 42;
	now->tv_usec = 15003;
	TIMESLICE_ADVANCE(timer, now);
	assert(timer->mono_sec == 0);
	assert(timer->mono_msec == 15);
	assert(timer->mono_usec == 15003);

	now->tv_sec = 45;
	now->tv_usec = 18001;
	TIMESLICE_ADVANCE(timer, now);
	assert(timer->mono_sec == 3);
	assert(timer->mono_msec == 3018);
	assert(timer->mono_usec == 3018001);

	/*
	 * Going backwards in time is evil.  We avoid
	 * it by stopping time and resuming in the next
	 * slice, given the time is moving forward again.
	 */

	now->tv_sec = 44;
	now->tv_usec = 842;
	TIMESLICE_ADVANCE(timer, now);
	assert(timer->mono_sec == 3);
	assert(timer->mono_msec == 3018);
	assert(timer->mono_usec == 3018001);

	now->tv_sec = 43;
	now->tv_usec = 842;
	TIMESLICE_ADVANCE(timer, now);
	assert(timer->mono_sec == 3);
	assert(timer->mono_msec == 3018);
	assert(timer->mono_usec == 3018001);

	now->tv_sec = 44;
	now->tv_usec = 845;
	TIMESLICE_ADVANCE(timer, now);
	assert(timer->mono_sec == 4);
	assert(timer->mono_msec == 4018);
	assert(timer->mono_usec == 4018004);

	/* Subseconds have the same mechanic. */

	now->tv_sec = 44;
	now->tv_usec = 840;
	TIMESLICE_ADVANCE(timer, now);
	assert(timer->mono_sec == 4);
	assert(timer->mono_msec == 4018);
	assert(timer->mono_usec == 4018004);

	now->tv_sec = 44;
	now->tv_usec = 841;
	TIMESLICE_ADVANCE(timer, now);
	assert(timer->mono_sec == 4);
	assert(timer->mono_msec == 4018);
	assert(timer->mono_usec == 4018005);
}

int
main(void)
{
	pout("peak base test suite... ");

	test_type();
	test_net();
	test_stash();
	test_alloc();
	test_prealloc();
	test_output();
	test_page();
	test_hash();
	test_time();

	pout("ok\n");

	return (0);
}
