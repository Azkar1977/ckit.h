#define CKIT_IMPLEMENTATION
#include "ckit.h"

void demo_array()
{
	printf("Dynamic array\n");
	int* a = NULL;
	for (int i = 0; i < 10; ++i) {
		apush(a, i);
	}
	for (int i = 0; i < 10; ++i) {
		printf("%d\n", a[i]);
	}
	printf("len=%d cap=%d\n", acount(a), acap(a));
	afree(a);
}

void demo_map()
{
	printf("Map (hashtable)\n");
	Map m = { 0 };
	for (int i = 0; i < 10; ++i)
		map_add(m, i, i * 10);
	for (int i = 9; i >= 0; --i)
		printf("k : %d, v %d\n", (int)i, (int)map_get(m, i));
	map_free(m);
}

void demo_string_intern()
{
	printf("String interning\n");
	const char* a = sintern("hello");
	const char* b = sintern("he" "llo");
	const char* c = sintern("world");
	printf("a : %s\n", a);
	printf("b : %s\n", b);
	printf("c : %s\n", c);
	printf("a==b? %s\n", (a == b) ? "yes" : "no");
	printf("b==c? %s\n", (b == c) ? "yes" : "no");
}

void unit_test_array()
{
	// build
	int *a = NULL;
	const int N = 1000;
	for (int i = 0; i < N; ++i) apush(a, i);
	assert(acount(a) == N);
	assert(acap(a) >= N);
	for (int i = 0; i < N; ++i) assert(a[i] == i);
	assert(alast(a) == N - 1);
	assert(aend(a) == a + N);

	// pop
	for (int i = N - 1; i >= 0; --i) {
		int x = apop(a);
		assert(x == i);
	}
	assert(acount(a) == 0);

	// reuse
	for (int i = 0; i < 32; ++i) apush(a, i * 2);
	aclear(a);
	assert(acount(a) == 0);

	for (int i = 0; i < 10; ++i) apush(a, i); // [0..9]
	adel(a, 3);
	assert(acount(a) == 9);
	assert(a[3] == 9);

	afree(a);
}

void unit_test_intern()
{
	const char *a = sintern("hello");
	const char *b = sintern("he" "llo");
	const char *c = sintern("world");
	assert(a == b);
	assert(a != c);
	assert(strcmp(a, "hello") == 0);
	assert(strcmp(c, "world") == 0);

	const char *hw = "helloworld";
	const char *h2 = sintern_range(hw, hw + 5); // "hello"
	assert(h2 == a);

	// different content -> different pointer
	const char *d = sintern("HELLO");
	assert(d != a);

	// long/random-ish strings
	char buf[256];
	for (int i = 0; i < 200; ++i) {
		int n = snprintf(buf, sizeof(buf), "str-%d-%d-%d", i, i*i, 12345);
		(void)n;
		const char *s1 = sintern(buf);
		const char *s2 = sintern(buf);
		assert(s1 == s2);
		assert(strcmp(s1, buf) == 0);
	}
}

void unit_test_map_basic()
{
	Map m = { 0 };
	assert(map_get(m, 1234) == 0);

	const int N = 2000;
	for (uint64_t i = 0; i < (uint64_t)N; ++i) {
		uint64_t key = (i % 2 == 0) ? i : (i * 2654435761u); // mix to create dispersion
		uint64_t val = key * 3u + 1u;
		map_add(m, key, val);
		uint64_t got = map_get(m, key);
		assert(got == val);
	}
	assert(m.size >=  (N > 0 ? 1 : 0));
	assert(m.slot_capacity >= 16);
	assert(m.slot_count > 0);

	// update existing
	for (uint64_t i = 0; i < (uint64_t)N; i += 5) {
		uint64_t key = (i % 2 == 0) ? i : (i * 2654435761u);
		uint64_t val2 = (key * 7u) ^ 0xBEEF;
		map_add(m, key, val2);
		assert(map_get(m, key) == val2);
	}

	// verify all present
	for (uint64_t i = 0; i < (uint64_t)N; ++i) {
		uint64_t key = (i % 2 == 0) ? i : (i * 2654435761u);
		uint64_t v = map_get(m, key);
		uint64_t updated = (i % 5 == 0);
		uint64_t expect = updated ? ((key * 7u) ^ 0xBEEF) : (key * 3u + 1u);
		assert(v == expect);
	}

	// test delete a subset
	int deleted = 0;
	for (uint64_t i = 0; i < (uint64_t)N; i += 3) {
		uint64_t key = (i % 2 == 0) ? i : (i * 2654435761u);
		int ok = map_del(m, key);
		if (ok) {
			++deleted;
			assert(map_get(m, key) == 0);
		}
	}
	assert(m.size == N - deleted);

	// remaining still fetch
	for (uint64_t i = 0; i < (uint64_t)N; ++i) {
		uint64_t key = (i % 2 == 0) ? i : (i * 2654435761u);
		uint64_t v = map_get(m, key);
		if (i % 3 == 0) {
			assert(v == 0);
		} else {
			uint64_t updated = (i % 5 == 0);
			uint64_t expect = updated ? ((key * 7u) ^ 0xBEEF) : (key * 3u + 1u);
			assert(v == expect);
		}
	}

	// clear and reuse
	map_clear(m);
	assert(m.size == 0);
	assert(m.slot_count == 0);
	for (int i = 0; i < 100; ++i) {
		map_add(m, (uint64_t)i, (uint64_t)(i + 42));
	}
	for (int i = 0; i < 100; ++i) {
		assert(map_get(m, (uint64_t)i) == (uint64_t)(i + 42));
	}

	map_free(m);
}

void unit_test_map_swap()
{
	Map m = { 0 };

	// Insert predictable keys/vals so we can track by index.
	const int N = 256;
	for (int i = 0; i < N; ++i) {
		uint64_t key = (uint64_t)(i + 1000);
		uint64_t val = (uint64_t)(key * 11 + 7);
		map_add(m, key, val);
	}
	assert(m.size == N);

	// grab two indices to swap
	int i = 3, j = 200;
	uint64_t ki = m.keys[i], kj = m.keys[j];
	uint64_t vi = map_get(m, ki), vj = map_get(m, kj);
	int si = m.islot[i], sj = m.islot[j];
	assert(si >= 0 && sj >= 0);

	// swap items (O(1) expected)
	map_swap(m, i, j);

	// keys in dense arrays should have swapped
	assert(m.keys[i] == kj && m.keys[j] == ki);
	assert(m.vals[i] == vj && m.vals[j] == vi);

	// lookups by key must still return the correct value
	assert(map_get(m, ki) == vi);
	assert(map_get(m, kj) == vj);

	// slots' item_index must be updated to the new dense indices
	assert(m.slots[si].item_index == j);
	assert(m.slots[sj].item_index == i);
	assert(m.islot[i] == sj && m.islot[j] == si);

	// Do a bunch of random swaps; ensure lookups are consistent.
	for (int t = 0; t < 1000; ++t) {
		int a = (t * 17) & (N - 1);
		int b = (t * 113 + 7) & (N - 1);
		if (a == b) continue;
		uint64_t ka = m.keys[a], kb = m.keys[b];
		uint64_t va = map_get(m, ka), vb = map_get(m, kb);
		map_swap(m, a, b);
		assert(map_get(m, ka) == va);
		assert(map_get(m, kb) == vb);
	}

	map_free(m);
}

void unit_test()
{
	unit_test_array();
	unit_test_map_basic();
	unit_test_map_swap();
	unit_test_intern();
}

int main()
{
	demo_array();
	printf("---\n");
	demo_map();
	printf("---\n");
	demo_string_intern();

	unit_test();

	return 0;
}
