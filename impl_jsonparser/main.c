#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "array.h"

static const int CHAR_ARRAY_START     = 0x5B;
static const int CHAR_ARRAY_CLOSE     = 0x5D;
static const int CHAR_OBJECT_START    = 0x7B;
static const int CHAR_OBJECT_CLOSE    = 0x7D;
static const int CHAR_NAME_SEPARATOR  = 0x3A;
static const int CHAR_VALUE_SEPARATOR = 0x2C;
static const int CHAR_NULL_START      = 0x6E;
static const int CHAR_TRUE_START      = 0x74;
static const int CHAR_FALSE_START     = 0x66;
static const int CHAR_QUOTATION_MARK  = 0x22;
static const int CHAR_MINUS           = 0x2d;
static const int CHAR_DECIMAL_POINT   = 0x2e;
static const int CHAR_EXPONENTIAL     = 0x65;

#define Assert(expr)                                                           \
  {                                                                            \
    if (!(expr)) {                                                             \
      fprintf(stderr, "Error: assertion " #expr " failed\n");                  \
      return 1;                                                                \
    }                                                                          \
  }

#define IS_ARRAY_START(c)     ((c) == CHAR_ARRAY_START)
#define IS_ARRAY_CLOSE(c)     ((c) == CHAR_ARRAY_CLOSE)
#define IS_OBJECT_START(c)    ((c) == CHAR_OBJECT_START)
#define IS_OBJECT_CLOSE(c)    ((c) == CHAR_OBJECT_CLOSE)
#define IS_NAME_SEPARATOR(c)  ((c) == CHAR_NAME_SEPARATOR)
#define IS_VALUE_SEPARATOR(c) ((c) == CHAR_VALUE_SEPARATOR)
#define IS_NULL_START(c)      ((c) == CHAR_NULL_START)
#define IS_TRUE_START(c)      ((c) == CHAR_TRUE_START)
#define IS_FALSE_START(c)     ((c) == CHAR_FALSE_START)
#define IS_QUOTATION_MARK(c)  ((c) == CHAR_QUOTATION_MARK)
#define IS_MINUS(c)           ((c) == CHAR_MINUS)
#define IS_ZERO(c)            ((c) == '0')
#define IS_DIGIT(c)           ('0' <= (c) && (c) <= '9')
#define IS_DECIMAL_POINT(c)   ((c) == CHAR_DECIMAL_POINT)
#define IS_EXPONENTIAL(c)     ((c) == CHAR_EXPONENTIAL)
#define IS_ESCAPE(c)          ((c) == 0x5c)
#define IS_UNESCAPED(c)       \
  ((c)  == 0x20 || (c) == 0x21 || (0x23 <= (c) && (c) <= 0x5b) || (c) >= 0x5D)
#define IS_WHITESPACE(c)                                                       \
  ((c)  == 0x20 || (c) == 0x09 || (c) == 0x0A || (c) == 0x0D)

typedef enum {
  NODE_TYPE_ARRAY,
  NODE_TYPE_OBJECT,
  NODE_TYPE_NUMBER,
  NODE_TYPE_STRING,
  NODE_TYPE_NULL,
  NODE_TYPE_TRUE,
  NODE_TYPE_FALSE,
} node_t;

typedef char Char;
InitArray(Char);
typedef ArrayChar String;

typedef struct {
  node_t typ;
  size_t size;
  void *val;
} Jnode;
InitArray(Jnode);

typedef struct {
  Jnode key;
  Jnode val;
} ObjectKv;
InitArray(ObjectKv);

int Parse(FILE *, Jnode *);
int parseArray(FILE *, Jnode *);
int parseObject(FILE *, Jnode *);
int parseToken(FILE *, Jnode *);
int matchString(FILE *, Jnode *);
int matchNumber(FILE *, Jnode *);
int matchLiteral(FILE *, Jnode *);

int next(FILE *fp) {
  int c = fgetc(fp);
  while (IS_WHITESPACE(c)) {
    c = fgetc(fp);
  }

  if (c == EOF) {
    if (feof(fp)) {
      return EOF;
    }
    if (ferror(fp)) {
      fprintf(stderr, "Error: stream next");
      exit(EXIT_FAILURE);
    }
  }
  return c;
}

int peek(FILE *fp) {
  int c = fgetc(fp);
  while (IS_WHITESPACE(c)) {
    c = fgetc(fp);
  }

  if (c == EOF) {
    if (feof(fp)) {
      return EOF;
    }
    if (ferror(fp)) {
      fprintf(stderr, "Error: stream peek");
      exit(EXIT_FAILURE);
    }
  } else {
    ungetc(c, fp);
  }
  return c;
}

int matchString(FILE *stream, Jnode *node) {
  int escape = 0;
  String *str = ArrayCharAlloc();

  next(stream);
  int c = next(stream);
  while (IS_UNESCAPED(c) || IS_ESCAPE(c) || escape) {
    if (escape) {
      escape = 0;
    }
    if (IS_ESCAPE(c)) {
      escape = 1;
    }

    ArrayCharWrite(str, c);
    c = next(stream);
  }
  Assert(IS_QUOTATION_MARK(c));

  ArrayCharWrite(str, '\0');
  node->typ = NODE_TYPE_STRING;
  node->size = str->len;
  node->val = ArrayCharAtone(str);
  return 0;
}

int matchNumber(FILE *stream, Jnode *node) {
  String *str = ArrayCharAlloc();

  int c = next(stream);
  if (IS_MINUS(c)) {
    ArrayCharWrite(str, c);
    c = next(stream);
  }

  if (IS_ZERO(c)) {
    ArrayCharWrite(str, c);
    c = next(stream);
    Assert(!IS_DIGIT(c));
  } else {
    Assert(IS_DIGIT(c));
    while (IS_DIGIT(c)) {
      ArrayCharWrite(str, c);
      c = next(stream);
    }
  }

  if (IS_DECIMAL_POINT(c)) {
    ArrayCharWrite(str, c);
    c = next(stream);
    Assert(IS_DIGIT(c));
    while (IS_DIGIT(c)) {
      ArrayCharWrite(str, c);
      c = next(stream);
    }
  }

  if (IS_EXPONENTIAL(c)) {
    ArrayCharWrite(str, c);
    c = next(stream);
    if (IS_MINUS(c)) {
      ArrayCharWrite(str, c);
      c = next(stream);
    }
    Assert(IS_DIGIT(c));
    while (IS_DIGIT(c)) {
      ArrayCharWrite(str, c);
      c = next(stream);
    }
  }

  ungetc(c, stream);
  ArrayCharWrite(str, '\0');
  node->typ = NODE_TYPE_NUMBER;
  node->size = str->len;
  node->val = ArrayCharAtone(str);
  return 0;
}

int matchLiteral(FILE *stream, Jnode *node) {
  String *str = ArrayCharAlloc();

  int c = next(stream);

  if (IS_TRUE_START(c)) {
    ArrayCharWrite(str, c);
    Assert(peek(stream) == 'r');
    ArrayCharWrite(str, next(stream));
    Assert(peek(stream) == 'u');
    ArrayCharWrite(str, next(stream));
    Assert(peek(stream) == 'e');
    ArrayCharWrite(str, next(stream));
    node->typ = NODE_TYPE_TRUE;
    node->size = 4;
    goto ret;
  }

  if (IS_FALSE_START(c)) {
    ArrayCharWrite(str, c);
    Assert(peek(stream) == 'a');
    ArrayCharWrite(str, next(stream));
    Assert(peek(stream) == 'l');
    ArrayCharWrite(str, next(stream));
    Assert(peek(stream) == 's');
    ArrayCharWrite(str, next(stream));
    Assert(peek(stream) == 'e');
    ArrayCharWrite(str, next(stream));
    node->typ = NODE_TYPE_FALSE;
    node->size = 5;
    goto ret;
  }

  if (IS_NULL_START(c)) {
    ArrayCharWrite(str, c);
    Assert(peek(stream) == 'u');
    ArrayCharWrite(str, next(stream));
    Assert(peek(stream) == 'l');
    ArrayCharWrite(str, next(stream));
    Assert(peek(stream) == 'l');
    ArrayCharWrite(str, next(stream));
    node->typ = NODE_TYPE_NULL;
    node->size = 4;
    goto ret;
  }

ret:
  ArrayCharWrite(str, '\0');
  node->val = ArrayCharAtone(str);
  return 0;
}

int parseToken(FILE *stream, Jnode *node) {
  int c = peek(stream);

  if (IS_QUOTATION_MARK(c)) {
    return matchString(stream, node);
  }
  if (IS_MINUS(c) || IS_DIGIT(c)) {
    return matchNumber(stream, node);
  }

  if (IS_TRUE_START(c) || IS_FALSE_START(c) || IS_NULL_START(c)) {
    return matchLiteral(stream, node);
  }

  return 1;
}

int parseArray(FILE *stream, Jnode *node) {
  ArrayJnode *array = ArrayJnodeAlloc();

  int separator = 0;
  int c = next(stream);
  while (1) {
    c = peek(stream);
    if (IS_ARRAY_CLOSE(c)) {
      next(stream); // consume ']'
      if (separator == 1) {
        fprintf(stderr, "Error: array close after value separator\n");
        return 1;
      }
      break;
    }

    if (IS_VALUE_SEPARATOR(c)) {
      separator = 1;
      next(stream);
      c = peek(stream);
    }

    separator = 0;
    Jnode item = {0};
    if (Parse(stream, &item) != 0) {
      fprintf(stderr, "Error: parseArray\n");
      return 1;
    }
    ArrayJnodeWrite(array, item);
  }

  node->typ = NODE_TYPE_ARRAY;
  node->size = array->len;
  node->val = ArrayJnodeAtone(array);
  return 0;
}

int parseObject(FILE *stream, Jnode *node) {
  ArrayObjectKv *array = ArrayObjectKvAlloc();

  int separator = 0;
  int c = next(stream);
  while (1) {
    c = peek(stream);

    if (IS_OBJECT_CLOSE(c)) {
      next(stream); // consume '}'
      if (separator == 1) {
        fprintf(stderr, "Error: object close after value separator\n");
        return 1;
      }
      break;
    }

    if (IS_VALUE_SEPARATOR(c)) {
      separator = 1;
      next(stream);
      c = peek(stream);
    }

    separator = 0;
    Jnode key = {0};
    if (parseToken(stream, &key) != 0) {
      fprintf(stderr, "Error: parseObject\n");
      return 1;
    }

    Assert(key.typ == NODE_TYPE_STRING);
    Assert(IS_NAME_SEPARATOR(next(stream)));
    Jnode val = {0};
    if (Parse(stream, &val) != 0) {
      fprintf(stderr, "Error: parseObject\n");
      return 1;
    }

    ObjectKv kv = {key, val};
    ArrayObjectKvWrite(array, kv);
  }

  node->typ = NODE_TYPE_OBJECT;
  node->size = array->len;
  node->val = ArrayObjectKvAtone(array);
  return 0;
}

int Parse(FILE *stream, Jnode *node) {
  int c = peek(stream);

  if (IS_ARRAY_START(c)) {
    return parseArray(stream, node);
  }
  if (IS_OBJECT_START(c)) {
    return parseObject(stream, node);
  }
  if (IS_QUOTATION_MARK(c) || (IS_DIGIT(c) || IS_MINUS(c)) ||
      (IS_TRUE_START(c) || IS_FALSE_START(c) || IS_NULL_START(c))) {
    return parseToken(stream, node);
  }

  return 1;
}

void Display(Jnode *node, int level) {
  switch (node->typ) {
  case NODE_TYPE_ARRAY:
    printf("[ARRAY]");
    Jnode *array = node->val;
    for (int i = 0; i < node->size; i++) {
      Display(&array[i], 1);
    }
    break;
  case NODE_TYPE_OBJECT:
    printf("[OBJECT]\n");
    ObjectKv *kvs = node->val;
    for (int i = 0; i < node->size; i++) {
      Display(&kvs[i].key, level + 1);
      printf(": ");
      Display(&kvs[i].val, level + 1);
      printf("\n");
    }
    break;
  case NODE_TYPE_NULL:
  case NODE_TYPE_TRUE:
  case NODE_TYPE_FALSE:
  case NODE_TYPE_NUMBER:
  case NODE_TYPE_STRING:
    for (int i = 0; i < level; i++) {
      printf(" ");
    }
    // printf("%zu\n\n", node->size);
    printf("%s", (char *)node->val);
    break;
  default:
    printf("unimplemented - %d\n", node->typ);
  }
}

void Free(Jnode *node) {
  if (!node)
    return;
  switch (node->typ) {
  case NODE_TYPE_NULL:
  case NODE_TYPE_TRUE:
  case NODE_TYPE_FALSE:
  case NODE_TYPE_STRING:
  case NODE_TYPE_NUMBER:
    free(node->val);
    break;
  case NODE_TYPE_ARRAY:;
    Jnode *array = node->val;
    for (int i = 0; i < node->size; i++) {
      Free(&array[i]);
    }
    free(node->val);
    break;
  case NODE_TYPE_OBJECT:;
    ObjectKv *kvs = node->val;
    for (int i = 0; i < node->size; i++) {
      Free(&kvs[i].key);
      Free(&kvs[i].val);
    }
    free(node->val);
    break;
  }
}

int main(int argc, char *argv[static 2]) {
  char *fname = argv[1];
  FILE *stream = fopen(fname, "r");
  if (stream == NULL)
    exit(EXIT_FAILURE);

  Jnode node = {0};
  if (Parse(stream, &node) != 0)
    exit(EXIT_FAILURE);

  Display(&node, 0);

  Free(&node);
  return EXIT_SUCCESS;
}
