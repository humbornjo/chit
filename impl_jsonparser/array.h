#define START_SIZE 4

#define ROUND_UP(x)                                                            \
  ({                                                                           \
    size_t _x = (x);                                                           \
    _x = _x - 1;                                                               \
    _x = _x | (_x >> 1);                                                       \
    _x = _x | (_x >> 2);                                                       \
    _x = _x | (_x >> 4);                                                       \
    _x = _x | (_x >> 8);                                                       \
    _x = _x | (_x >> 16);                                                      \
    _x + 1;                                                                    \
  })

#define InitArray(type)                                                        \
  typedef struct {                                                             \
    size_t cap;                                                                \
    size_t len;                                                                \
    type *buffer;                                                              \
  } Array##type;                                                               \
                                                                               \
  void Array##type##Write(Array##type *arr, const type slot) {                 \
    if (arr->buffer == NULL) {                                                 \
      size_t cap = START_SIZE;                                                 \
      arr->buffer = malloc(cap * sizeof(type));                                \
      if (arr->buffer == NULL) {                                               \
        fprintf(stderr, "Error: array_" #type " write\n");                     \
        exit(EXIT_FAILURE);                                                    \
      }                                                                        \
      arr->cap = cap;                                                          \
      arr->buffer[arr->len++] = slot;                                          \
      return;                                                                  \
    }                                                                          \
                                                                               \
    if (arr->len + 1 <= arr->cap) {                                            \
      arr->buffer[arr->len++] = slot;                                          \
      return;                                                                  \
    }                                                                          \
                                                                               \
    size_t cap = ROUND_UP(arr->cap + 1);                                       \
    type *buffer = malloc(cap * sizeof(type));                                 \
    if (buffer == NULL) {                                                      \
      fprintf(stderr, "Error: array_" #type " write\n");                       \
      exit(EXIT_FAILURE);                                                      \
    }                                                                          \
    memcpy(buffer, arr->buffer, arr->len * sizeof(type));                      \
    free(arr->buffer);                                                         \
    arr->buffer = buffer;                                                      \
    arr->cap = cap;                                                            \
    arr->buffer[arr->len++] = slot;                                            \
  }                                                                            \
                                                                               \
  Array##type *Array##type##Alloc() {                                          \
    Array##type *arr = malloc(sizeof(Array##type));                            \
    arr->cap = 0;                                                              \
    arr->len = 0;                                                              \
    arr->buffer = NULL;                                                        \
    return arr;                                                                \
  }                                                                            \
                                                                               \
  void Array##type##Free(Array##type *arr) {                                   \
    if (arr == NULL) {                                                         \
      return;                                                                  \
    }                                                                          \
    if (arr->buffer != NULL) {                                                 \
      free(arr->buffer);                                                       \
    }                                                                          \
    free(arr);                                                                 \
  }                                                                            \
                                                                               \
  type *Array##type##Atone(Array##type *arr) {                                 \
    type *buffer = arr->buffer;                                                \
    arr->buffer = NULL;                                                        \
    Array##type##Free(arr);                                                    \
    return buffer;                                                             \
  }
