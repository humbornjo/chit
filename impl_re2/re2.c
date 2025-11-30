// Partial implementation of RE2 in C.
//
// Follow Thompson's NFA algorithm. For Class regular expressions,
// only single char is supported. Unicode (\p{...} or \P{...}) is
// not involved in this implementation. Greedy quantifier is not
// supported as well.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "./rune.h"

// Not all Operations defined below are used.
typedef enum op op_em;
enum op {
  OP_STAR,
  OP_PLUS,
  OP_QUEST,
  OP_REPEAT,
  OP_CLASS,
  OP_ANYCHAR,
  OP_LITERAL,
  OP_LINE_BEGIN,
  OP_LINE_END,
  OP_TEXT_BEGIN,
  OP_TEXT_END,
  OP_EMPTY_MATCH,
  OP_CONCAT,
  OP_CAPTURE,
  OP_ALTERNATE,
  OP_WORD_BOUNDARY,
  OP_NOT_WORD_BOUNDARY,

  OP_PSEUDO = 128,
  OP_CAPTURE_LEFT,
  OP_CAPTURE_RIGHT,
  OP_VERTICLE_BAR,
};

typedef struct regex regex_st;
struct regex {
  op_em op;
  int capture;
  string *runes;
  int min, max;

  int len, cap;
  regex_st **subs;
};

regex_st *regex_new(op_em op) {
  regex_st *re = malloc(sizeof(regex_st));
  re->op = op;
  re->runes = string_new();

  re->len = 0;
  re->cap = 4;
  re->subs = malloc(re->cap * sizeof(regex_st *));
  return re;
}

void regex_free(regex_st *re) {
  if (re == NULL) {
    return;
  }
  if (re->runes != NULL) {
    string_free(re->runes);
  }
  for (int i = 0; i < re->len; i++) {
    regex_free(re->subs[i]);
  }
  if (re->subs != NULL) {
    free(re->subs);
  }
  free(re);
}

void regex_append(regex_st *re, regex_st **subs) {
again:
  if (re->len == re->cap) {
    re->cap *= 2;
    re->subs = realloc(re->subs, re->cap * sizeof(regex_st *));
  }
  if (subs == NULL || *subs == NULL) {
    re->subs[re->len] = NULL;
    return;
  }
  re->subs[re->len++] = *subs;
  subs++;
  goto again;
}

regex_st *regex_from(op_em op, void *any) {
  regex_st *re = regex_new(op), *sub;
  switch (op) {
  case OP_LITERAL:
    string_append(re->runes, (char *)any);
    break;
  case OP_CAPTURE_LEFT:
    re->capture = *(int *)any;
    break;
  case OP_CAPTURE:
    regex_append(re, (regex_st *[2]){(regex_st *)any, NULL});
    break;
  case OP_CAPTURE_RIGHT:
    break;
  case OP_STAR:
  case OP_PLUS:
  case OP_QUEST:
    regex_append(re, (regex_st *[2]){(regex_st *)any, NULL});
    break;
  case OP_REPEAT:
    sub = (regex_st *)any;
    re->min = sub->min;
    re->max = sub->max;
    sub->min = sub->max = 0;
    regex_append(re, (regex_st *[2]){(regex_st *)any, NULL});
    break;
  default:
    break;
  }

  return re;
}

void regex_print(regex_st *re) {
  printf("%d", re->op);
  if (re->subs == NULL) {
    printf("\n");
    return;
  }

  if (re->op == OP_LITERAL || re->op == OP_CLASS) {
    printf(" %s\n", string_data(re->runes));
  } else {
    printf("\n");
  }
  for (int i = 0; i < re->len; i++) {
    regex_print(re->subs[i]);
  }
}

typedef struct stack stack_st;
struct stack {
  int len;
  int cap;
  regex_st **data;
};

void stack_init(stack_st *stack) {
  stack->len = 0;
  stack->cap = 4;
  stack->data = malloc(stack->cap * sizeof(regex_st *));
}

regex_st *stack_free(stack_st *stack) {
  regex_st *re = stack->data[0];
  stack->data[0] = NULL;
  for (int i = 0; i < stack->len; i++) {
    regex_free(stack->data[i]);
  }
  free(stack->data);
  return re;
}

regex_st **stack_slice(stack_st *stack, int start, int end) {
  int n = end - start;
  if (n == 0) {
    return NULL;
  }
  regex_st **ret = malloc((n + 1) * sizeof(regex_st *));
  ret[n] = NULL;

  for (int i = start; i < stack->len; i++) {
    if (i < end) {
      ret[i - start] = stack->data[i];
      continue;
    }
    stack->data[i - n] = stack->data[i];
  }
  stack->len -= n;

  return ret;
}

stack_st stack_from(regex_st **re) {
  stack_st stack = {0};
  if (re == NULL) {
    stack_init(&stack);
    return stack;
  }

  int n = 0;
  regex_st **cpy = re;
  for (; *(cpy++); n++) {
  }

  stack.data = re;
  stack.cap = stack.len = n;
  return stack;
}

void stack_push(stack_st *stack, regex_st *re) {
  if (stack->len == stack->cap) {
    stack->cap *= 2;
    regex_st **new = realloc(stack->data, stack->cap * sizeof(regex_st *));
    if (new == NULL) {
      exit(1);
    }
    stack->data = new;
  }
  stack->data[stack->len++] = re;
}

regex_st *stack_pop(stack_st *stack) {
  if (stack->len == 0) {
    return NULL;
  }
  regex_st *re = stack->data[--stack->len];
  stack->data[stack->len] = NULL;
  return re;
}

typedef struct parser parser_t;
struct parser {
  int num_cap;
  int num_rune;
  stack_st stack;
};

regex_st *parser_collapse(op_em op, regex_st **res) {
  regex_st *re = regex_new(op);

  for (regex_st **i = res; *i; ++i) {
    if ((*i)->op == op) {
      regex_append(re, (*i)->subs);
    }
    regex_append(re, (regex_st *[2]){*i, NULL});
  }

  return re;
}

void parser_concat_literal(parser_t *parser) {
  if (parser->stack.len <= 1) {
    return;
  }

  int n = parser->stack.len;
  regex_st *re1 = parser->stack.data[n - 1];
  regex_st *re2 = parser->stack.data[n - 2];
  if (re1->op == OP_LITERAL && re2->op == OP_LITERAL) {
    string_append(re2->runes, string_data(re1->runes));
    regex_free(stack_pop(&parser->stack));
  }
}

void parser_push(parser_t *parser, regex_st *regex) {
  parser_concat_literal(parser);
  parser->num_rune += string_len(regex->runes);
  stack_push(&parser->stack, regex);
}

regex_st *parser_pop(parser_t *parser) {
  parser->num_rune -=
      string_len(parser->stack.data[parser->stack.len - 1]->runes);
  return stack_pop(&parser->stack);
}

void parser_concat(parser_t *parser) {
  parser_concat_literal(parser);

  int n = parser->stack.len;
  int i = parser->stack.len;
  for (; i > 0 && parser->stack.data[i - 1]->op < OP_PSEUDO; i--) {
  }

  regex_st **subs = stack_slice(&parser->stack, i, parser->stack.len);
  regex_st **temp = stack_slice(&parser->stack, 0, i);
  parser->stack = stack_from(temp);

  if (n - i == 0) {
    parser_push(parser, regex_from(OP_EMPTY_MATCH, NULL));
    return;
  }
  parser_push(parser, parser_collapse(OP_CONCAT, subs));
}

void parser_alternate(parser_t *parser) {
  int n = parser->stack.len;
  int i = parser->stack.len;
  for (; i > 0 && parser->stack.data[i - 1]->op < OP_PSEUDO; i--) {
  }

  regex_st **subs = stack_slice(&parser->stack, i, parser->stack.len);
  regex_st **temp = stack_slice(&parser->stack, 0, i);
  parser->stack = stack_from(temp);

  if (n - i == 0) {
    parser_push(parser, regex_from(OP_EMPTY_MATCH, NULL));
    return;
  }
  parser_push(parser, parser_collapse(OP_ALTERNATE, subs));
}

char parser_class_char(parser_t *parser, char **rc, string *class) {
  if (strlen(*rc) == 0) {
    return -1;
  }

  char ret = **rc;
  *rc += 1;
  return ret;
}

void parser_class(parser_t *parser, char **rc) {
  (*rc)++; // advance over '['
  regex_st *re = regex_new(OP_CLASS);

  int sign = +1;
  if (**rc && **rc == '^') {
    sign = -1;
    (*rc)++;
  }

  string *class = re->runes;
  do {
    char lo = parser_class_char(parser, rc, class);
    if (lo == -1) {
      regex_free(re);
      return;
    }
    char hi = lo;
    string_append(class, (char[3]){lo, hi, '\0'});
  } while (**rc != ']');

  // This will be done by outtie for-loop
  // *rc += 1; // advance over ']'

  parser_push(parser, re);
}

int parser_repeat(parser_t *parser, char *c, int *min, int *max) {
  if (!*c || *c != '{') {
    return 1;
  }

  c++;
  do {
    if (*c >= '0' && *c <= '9') {
      *min *= 10;
      *min += *c - '0';
      c++;
    } else {
      return 1;
    }
  } while (*c && *c != ',');

  c++;
  do {
    if (*c >= '0' && *c <= '9') {
      *max *= 10;
      *max += *c - '0';
      c++;
    } else {
      return 1;
    }
  } while (*c && *c != '}');

  return *c != '}';
}

int parser_swap_vertbar(parser_t *parser) {
  int n = parser->stack.len;
  if (n < 2) {
    return 0;
  }

  regex_st *re1 = parser->stack.data[n - 1];
  regex_st *re2 = parser->stack.data[n - 2];

  if (re2->op == OP_VERTICLE_BAR) {
    parser->stack.data[n - 1] = re2;
    parser->stack.data[n - 2] = re1;
    return 1;
  }
  return 0;
}

void parser_init(parser_t *parser) { stack_init(&parser->stack); }

regex_st *parser_free(parser_t *parser) {
  parser_concat(parser);
  if (parser_swap_vertbar(parser)) {
    parser_pop(parser);
  }

  parser_alternate(parser);
  return stack_free(&parser->stack);
}

typedef struct parse_error parse_error_st;
struct parse_error {
  char *msg;
};

char PARSE_ERROR_INVALID_REPEAT[] = "invalid repeat";
char PARSE_ERROR_INVALID_CAPTURE[] = "invalid capture";

regex_st *parse(char *pattern, parse_error_st *err) {
  if (pattern == NULL || err == NULL) {
    return NULL;
  }
  parser_t parser = {0};
  parser_init(&parser);
  for (char *c = pattern; *c; c++) {
    switch (*c) {
    default:
      parser_push(&parser, regex_from(OP_LITERAL, (char[2]){*c, '\0'}));
      break;
    case '.':
      parser_push(&parser, regex_from(OP_ANYCHAR, NULL));
      break;
    case '^':
      parser_push(&parser, regex_from(OP_LINE_BEGIN, NULL));
      break;
    case '$':
      parser_push(&parser, regex_from(OP_LINE_END, NULL));
      break;
    case '[':
      parser_class(&parser, &c);
      break;
    case '(':
      ++parser.num_cap;
      parser_push(&parser, regex_from(OP_CAPTURE_LEFT, &parser.num_cap));
      break;
    case ')':
      parser_concat(&parser);
      if (parser_swap_vertbar(&parser)) {
        parser_pop(&parser);
      }
      if (parser.stack.len < 2) {
        err->msg = PARSE_ERROR_INVALID_CAPTURE;
        goto unexpected;
      }
      {
        regex_st *re1 = parser_pop(&parser);
        regex_st *re2 = parser_pop(&parser);
        if (re2->op != OP_CAPTURE_LEFT) {
          err->msg = PARSE_ERROR_INVALID_CAPTURE;
          goto unexpected;
        }
        re2->op = OP_CAPTURE;
        regex_append(re2, (regex_st *[2]){re1, NULL});
        parser_push(&parser, regex_from(OP_CAPTURE, re2));
      }
      break;
    case '|':
      parser_concat(&parser);
      if (!parser_swap_vertbar(&parser)) {
        parser_push(&parser, regex_from(OP_VERTICLE_BAR, NULL));
      }
      break;
    case '*':
    case '+':
    case '?':
      if (parser.stack.len == 0) {
        err->msg = PARSE_ERROR_INVALID_REPEAT;
        goto unexpected;
      }
      {
        op_em op;
        switch (*c) {
        case '*':
          op = OP_STAR;
          break;
        case '+':
          op = OP_PLUS;
          break;
        case '?':
          op = OP_QUEST;
          break;
        };
        parser_push(&parser, regex_from(op, parser_pop(&parser)));
      }
      break;
    case '{':
      if (parser.stack.len == 0) {
        err->msg = PARSE_ERROR_INVALID_REPEAT;
        goto unexpected;
      }
      {
        int min = 0, max = 0;
        if (parser_repeat(&parser, c, &min, &max) != 0) {
          parser_push(&parser, regex_from(OP_LITERAL, (char[2]){*c, '\0'}));
        } else {
          for (; *c != '}'; c++) {
          }
          regex_st *sub = parser_pop(&parser);
          sub->min = min;
          sub->max = max;
          parser_push(&parser, regex_from(OP_REPEAT, sub));
        }
      }
      break;
    case '\\':
      c++;
      switch (*c) {
      case 'A':
        parser_push(&parser, regex_from(OP_TEXT_BEGIN, NULL));
        goto endofescape;
      case 'b':
        parser_push(&parser, regex_from(OP_WORD_BOUNDARY, NULL));
        goto endofescape;
      case 'B':
        parser_push(&parser, regex_from(OP_NOT_WORD_BOUNDARY, NULL));
        goto endofescape;
      case 'z':
        parser_push(&parser, regex_from(OP_TEXT_END, NULL));
        goto endofescape;
      case '\0':
        goto endofescape;
      }
      parser_push(&parser, regex_from(OP_LITERAL, (char[2]){*c, '\0'}));
    endofescape:
      break;
    }
  }
  return parser_free(&parser);

unexpected:
  regex_free(parser_free(&parser));
  return NULL;
}

typedef enum inst inst_em;
enum inst {
  INST_ANY,
  INST_CHAR,
  INST_EMPTY,
  INST_CAPTURE,
  INST_ALTERNATE,
  INST_ALTMATCH,
  INST_MATCH,
};

typedef struct code code_st;
struct code {
  inst_em inst;
  int arg;
  string *runes;
  code_st *out;
  code_st *out1;
};

code_st *code_from(inst_em inst, code_st *out) {
  code_st *code = malloc(sizeof(code_st));
  memset(code, 0, sizeof(code_st));
  code->inst = inst;
  code->out = out;
  code->runes = string_new();
  return code;
}

typedef struct ptrlist ptrlist_st;
struct ptrlist {
  code_st *code;
  ptrlist_st *next;
};

void ptrlist_append(ptrlist_st *l, ptrlist_st *l1) {
  ptrlist_st *oldl = l;
  while (l->next)
    l = l->next;
  l->next = l1;
}

typedef struct frag frag_st;
struct frag {
  code_st *start;
  ptrlist_st *out;
};

void frag_patch(ptrlist_st *out, code_st *code) {
  ptrlist_st *next;
  for (; out->next; out = next) {
    next = out->next;
    out->code = code;
  }
}

void frag_concat(frag_st *f, frag_st f1) {
  if (f->start == NULL) {
    f->start = f1.start;
    f->out = f1.out;
    return;
  }
  frag_patch(f->out, f1.start);
  f->out = f1.out;
}

void frag_alternate(frag_st *f, frag_st f1) {
  if (f->start == NULL) {
    f->start = f1.start;
    f->out = f1.out;
    return;
  }
  code_st *code = code_from(INST_ALTERNATE, NULL);
  code->out = f->start;
  code->out1 = f1.start;

  f->start = code;
  ptrlist_append(f->out, f1.out);
}

frag_st _compile(regex_st *re) {
  frag_st frag = {0};

  switch (re->op) {
  case OP_CONCAT:
    for (int i = 0; i < re->len; i++) {
      frag_concat(&frag, _compile(re->subs[i]));
    }
    break;
  case OP_LITERAL:
    for (int i = 0; i < string_len(re->runes); i++) {
      frag_st f = {.start = code_from(INST_CHAR, NULL)};
      f.out = (ptrlist_st *)(&f.start->out);
      string_append(f.start->runes, (char[2]){string_data(re->runes)[i], '\0'});
      frag_concat(&frag, f);
    }
    break;
  // case OP_CLASS:
  //   code_st *code = code_from(INST_CLASS, re->runes);
  //   frag->start = code;
  //   frag->out = (ptrlist_st *)(&code.out);
  //   break;
  case OP_ALTERNATE:
    for (int i = 0; i < re->len; i++) {
      frag_alternate(&frag, _compile(re->subs[i]));
    }
    break;
  // case OP_PLUS:
  //   _compile(re->subs[0]) frag_concat(&frag, );
  // case OP_STAR:
  //   frag_star(frag, _compile(re->subs[0]));
  //   break;
  // case OP_QUEST:
  //   frag_quest(frag, _compile(re->subs[0]));
  //   break;
  // case OP_REPEAT:
  //   frag_repeat(frag, _compile(re->subs[0]), re->min, re->max);
  //   break;
  // case OP_CAPTURE:
  //   frag_capture(frag, _compile(re->subs[0]));
  //   break;
  default: // default matches cases that never happen, fail fast
    exit(EXIT_FAILURE);
  }
  return frag;
}

code_st *compile(regex_st *re) {
  frag_st frag = _compile(re);
  if (frag.start == NULL) {
    return NULL;
  }
  frag_patch(frag.out, code_from(INST_MATCH, NULL));
  return frag.start;
}

// int match(prog_t *prog, char *text);

int main(int argc, char **argv) {
  // char *pattrn = "\\\\a|(bc)|de|f[ab]ghi*k+m{1,2}";
  char *pattrn = "abc";

  parse_error_st error = {0};
  regex_st *re = parse(pattrn, &error);
  if (re == NULL) {
    printf("Parse Error: %s\n", error.msg);
    exit(EXIT_FAILURE);
  }
  regex_print(re);

  code_st *prog = compile(re);
  if (prog == NULL) {
    printf("Compile Error\n");
    exit(EXIT_FAILURE);
  }
  regex_free(re);

  // match(prog_t *prog, char *text);
  return 0;
}
