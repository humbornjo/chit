#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#define hash_table_template(items)                                             \
  typedef struct {                                                             \
    items kvs;                                                                 \
    hash h;                                                                    \
    int e;                                                                     \
    int size;                                                                  \
    int cap;                                                                   \
  } HashTable

#define hash_table_init(ht, hash, exp)                                         \
  do {                                                                         \
    assert(exp >= 0);                                                          \
    (ht)->kvs = malloc(sizeof(*(ht)->kvs) * (1 << exp));                       \
    (ht)->h = hash;                                                            \
    (ht)->e = exp;                                                             \
    (ht)->size = 0;                                                            \
    (ht)->cap = (1 << exp);                                                    \
  } while (0)

// user code
typedef struct {
  char *p;
  int l;
} str;

typedef struct {
  str k;
  int v;
  int d;
} kv;
typedef kv *kvs;

typedef uint64_t (*hash)(str);
uint64_t dumb_hash(str k) { return 1; }
hash_table_template(kvs);

bool streq(str k1, str k2) {
  if (k1.l != k2.l)
    return false;
  for (int i = 0; i < k1.l; ++i) {
    if (*k1.p++ != *k2.p++)
      return false;
  }
  return true;
}

void hprint(const HashTable *ht) {
  printf("Hash Table: \n");
  for (int i = 0; i < ht->cap; ++i) {
    printf("  slot %d: %s => %d [%d]\n", i, ht->kvs[i].k.p, ht->kvs[i].v,
           ht->kvs[i].d);
  }
}

int hget(const HashTable *ht, str k) {
  int cnt = 0;
  int i = ht->h(k) % ht->cap;
  while (cnt++ < ht->cap && (ht->kvs[i].k.p || ht->kvs[i].d)) {
    if (!ht->kvs[i].d && streq(ht->kvs[i].k, k))
      return ht->kvs[i].v;
    i = (i + 1) % ht->cap;
  }
  return -1;
}

bool hset(HashTable *ht, str k, int v) {
  int cnt = 0;
  int i = ht->h(k) % ht->cap;
  while (cnt++ < ht->cap) {
    if (streq(ht->kvs[i].k, k)) {
      ht->kvs->v = v;
      return true;
    }
    if (!ht->kvs[i].k.p)
      break;
    i = (i + 1) % ht->cap;
  }
  if (!ht->kvs[i].k.p) {
    ht->kvs[i].k = k;
    ht->kvs[i].v = v;
    ht->kvs[i].d = 0;
    ht->size += 1;
    return true;
  }
  return false;
}

bool hdel(HashTable *ht, str k) {
  int cnt = 0;
  int i = ht->h(k) % ht->cap;
  while (cnt++ < ht->cap) {
    if (streq(ht->kvs[i].k, k)) {
      ht->kvs[i].k.p = NULL;
      ht->kvs[i].k.l = 0;
      ht->kvs[i].v = 0;
      ht->kvs[i].d = 1;
      ht->size -= 1;
      return true;
    }
    i = (i + 1) % ht->cap;
  }
  return false;
}

int main(void) {
  HashTable ht;
  hash_table_init(&ht, dumb_hash, 3);
  str d = {"urdad", 5};
  if (hset(&ht, d, 1)) {
  }
  str m = {"urmom", 5};
  if (hset(&ht, m, 10)) {
  }
  str n = {"urnan", 5};
  if (hset(&ht, n, 100)) {
  }

  str mm = {"urmom", 5};
  hdel(&ht, mm);
  str nn = {"urnan", 5};
  if (hget(&ht, nn) != -1) {
    printf("success del\n");
  }
  hdel(&ht, mm);
  hprint(&ht);

  return 0;
}
