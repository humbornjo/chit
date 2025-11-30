#include <stdlib.h>

typedef struct string string;
struct string {
  char *data;
  int len, cap;
};

#define INIT_STRING_CAP 4

static inline string *string_new() {
  string *s = (string *)malloc(sizeof(string));
  s->len = 0;
  s->cap = INIT_STRING_CAP;
  s->data = (char *)malloc(INIT_STRING_CAP * sizeof(char));
  return s;
};

static inline void string_append(string *s, char *c) {
again:
  if (s->len == s->cap) {
    s->cap *= 2;
    s->data = (char *)realloc(s->data, s->cap * sizeof(char));
  }
  if (c == NULL || *c == '\0') {
    s->data[s->len] = '\0';
    return;
  }
  s->data[s->len++] = *c;
  c++;
  goto again;
}

static inline int string_len(string *s) { return s->len; }

static inline char *string_data(string *s) { return s->data; }

static inline void string_free(string *s) {
  free(s->data);
  free(s);
};
