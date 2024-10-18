#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/_types/_size_t.h>

#define DBG false
#define CAP_HEAP_LIMIT 640000
#define NUM_CHUNK_LIMIT 1024
#define UNIMPLEMENTED()                                                        \
  do {                                                                         \
    fprintf(stderr,                                                            \
            "[TODO] %s:%d func %s has not been implemented "                   \
            "yet.\n",                                                          \
            __FILE__, __LINE__, __func__);                                     \
    abort();                                                                   \
  } while (0)
#define DEBUG(msg)                                                             \
  if (DBG)                                                                     \
  printf("[DEBUG] %s:%d %s\n", __FILE__, __LINE__, msg)

typedef struct {
  void *start;
  size_t size;
} Chunk;

typedef struct {
  Chunk chunks[NUM_CHUNK_LIMIT];
  size_t size;
} ChunkList;

char heap[CAP_HEAP_LIMIT] = {0};

ChunkList chunks_alloced = {0};
ChunkList chunks_free = {
    .size = 1,
    .chunks = {[0] = {.size = CAP_HEAP_LIMIT, .start = heap}},
};

int chunk_cmp_less(const void *p1, const void *p2) {
  return ((Chunk *)p1)->start - ((Chunk *)p2)->start;
}

int chunk_list_find(const ChunkList *list, void *ptr) {
  Chunk key = {.start = ptr};
  void *ret =
      bsearch(&key, list->chunks, list->size, sizeof(Chunk), chunk_cmp_less);

  // char temp[80];
  // Chunk *c_temp = ret;
  // sprintf(temp, "bsearch ret: %p with start: %p", ret, c_temp->start);
  // DEBUG(temp);

  if (ret != NULL) {
    assert(ret >= (void *)list->chunks);
    return (ret - (void *)list->chunks) / sizeof(Chunk);
  } else {
    return -1;
  }
}

void chunk_list_insert(ChunkList *list, void *ptr, size_t size) {
  assert(list->size < NUM_CHUNK_LIMIT);
  list->chunks[list->size].start = ptr;
  list->chunks[list->size].size = size;
  for (size_t i = list->size;
       i > 0 && list->chunks[i].start <= list->chunks[i - 1].start; i--) {
    const Chunk temp = list->chunks[i];
    list->chunks[i] = list->chunks[i - 1];
    list->chunks[i - 1] = temp;
  }
  list->size += 1;
}

void chunk_list_remove(ChunkList *list, size_t index) {
  assert(index >= 0 && index < list->size);
  for (size_t i = index; i < list->size - 1; i++) {
    list->chunks[i] = list->chunks[i + 1];
  }
  list->size -= 1;
}

void chunk_list_merge(ChunkList *list) {
  size_t p = 0;
  for (size_t i = 1; i < list->size; i++) {
    if (list->chunks[p].start + list->chunks[p].size == list->chunks[i].start) {
      list->chunks[p].size += list->chunks[i].size;
    } else {
      list->chunks[++p] = list->chunks[i];
    }
  };
  list->size = ++p;
}

void chunk_list_dump(const ChunkList *list) {
  printf("ChunkList(%zu)\n", list->size);
  for (size_t i = 0; i < list->size; i++) {
    printf("  chunk %zu: start: %p, size: %zu\n", i, list->chunks[i].start,
           list->chunks[i].size);
  }
}

void *heap_alloc(size_t size) {
  if (size == 0)
    return NULL;

  bool try_free = false;
alloc:
  for (size_t i = 0; i < chunks_free.size; i++) {
    if (chunks_free.chunks[i].size >= size) {
      Chunk *c = &chunks_free.chunks[i];
      void *ret = c->start;
      chunk_list_insert(&chunks_alloced, c->start, size);
      if (c->size - size > 0) {
        c->start += size;
        c->size -= size;
      } else {
        chunk_list_remove(&chunks_free, i);
      }
      return ret;
    }
  }
  chunk_list_merge(&chunks_free);
  if (!try_free) {
    try_free = !try_free;
    goto alloc;
  }
  return NULL;
}

void heap_free(void *ptr) {
  if (ptr != NULL) {
    const int index = chunk_list_find(&chunks_alloced, ptr);
    assert(index >= 0);
    chunk_list_insert(&chunks_free, chunks_alloced.chunks[index].start,
                      chunks_alloced.chunks[index].size);
    chunk_list_remove(&chunks_alloced, (size_t)index);
  }
}

// void heap_collect() { UNIMPLEMENTED(); }

int main(void) {
  for (size_t i = 0; i < 10; i++) {
    void *ptr = heap_alloc(i);
    heap_free(ptr);
  }

  chunk_list_merge(&chunks_free);
  heap_alloc(100);

  chunk_list_dump(&chunks_alloced);
  chunk_list_dump(&chunks_free);

  return 0;
}
