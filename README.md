# ckit.h

A tiny, single-header kit of high-performance C essentials.

- ~400 LoC
- Dynamic arrays (stretchy buffers)
- Map (hashtable): `uint64_t → uint64_t`
- String interning (stable pointer per unique string)

# Usage

A small program that exercises all three parts:

```c
#define CKIT_IMPLEMENTATION
#include "ckit.h"

#include <stdio.h>
#include <assert.h>

int main(void)
{
    // Dynamic array.
    int* a = NULL;
    for (int i = 0; i < 10; ++i) {
        apush(a, i);
    }
    for (int i = 0; i < 10; ++i) {
        printf("%d\n", a[i]);
    }
    printf("len=%d cap=%d\n", acount(a), acap(a));
    afree(a);

    // Map.
    Map m = { 0 };
    for (uint64_t i = 0; i < 10; ++i)
        map_add(m, i, i * 10);
    for (uint64_t i = 0; i < 10; ++i)
        printf("k : %d, v %d\n", (int)i, (int)map_get(m, i));
    map_free(m);

    // String interning.
    const char *a = sintern("hello");
    const char *b = sintern("he" "llo");
    assert(a == b);

    return 0;
}
```

See [**example.c**](example.c) for demonstration and unit tests.

# License

The code is in public domain. See [LICENSE](LICENSE) for more info.
