#pragma once
// Host shim for <esp_heap_caps.h>: the layout code allocates its render-plan
// state with heap_caps_calloc(MALLOC_CAP_*). On the host we just use the C
// library. Capability flags collapse to 0.

#include <stdlib.h>

#define MALLOC_CAP_INTERNAL 0
#define MALLOC_CAP_SPIRAM 0
#define MALLOC_CAP_DMA 0
#define MALLOC_CAP_8BIT 0
#define MALLOC_CAP_DEFAULT 0

static inline void *heap_caps_calloc(size_t n, size_t sz, unsigned) {
    return calloc(n, sz);
}
static inline void *heap_caps_malloc(size_t sz, unsigned) {
    return malloc(sz);
}
static inline void heap_caps_free(void *p) {
    free(p);
}
static inline size_t heap_caps_get_free_size(unsigned) {
    return 8u * 1024u * 1024u;
}
static inline size_t heap_caps_get_largest_free_block(unsigned) {
    return 4u * 1024u * 1024u;
}
static inline size_t heap_caps_get_minimum_free_size(unsigned) {
    return 4u * 1024u * 1024u;
}
