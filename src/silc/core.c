/*
 * Copyright 2015 Alexander Shabanov - http://alexshabanov.com.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 *
 * You may obtain a copy of the License at http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "silc.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include "mem.h"
#include "builtins.h"

/* Helper macros */

#ifndef countof
#define countof(arr)    (sizeof(arr) / sizeof(arr[0]))
#endif


/* Forward declarations */

#include "silc.h"

/* Struct declarations */
struct silc_mem_init_t;
struct silc_mem_t;

typedef silc_obj (* silc_fn_ptr)(struct silc_funcall_t* f);

struct silc_ctx_t {
  struct silc_settings_t* settings;

  /* stack */
  silc_obj*             stack;
  int                   stack_end;  /* index of the position after last inserted element in the stack */
  int                   stack_size; /* total stack size */

  /* heap */
  struct silc_mem_init_t* mem_init;
  struct silc_mem_t*      mem;

  /* root object for service data */
  silc_obj              root_cons;
  silc_obj              sym_hash_table;

  /* builtin functions */
  silc_fn_ptr *         fn_array;
  int                   fn_count;

  /* exit code */
  int                   exit_code;
};

struct silc_settings_t {
  FILE *    out;        /* default output stream (used in print function) */
};

static silc_obj silc_hash_table(struct silc_ctx_t* c, int initial_size);

/* Low-level mem alloc functions */

static void * xmalloc(size_t size) {
  void * p = malloc(size);
  if (p == 0) {
    fputs(";; Out of memory\n", stderr);
    abort();
  }
  return p;
}

static void * xmallocz(size_t size) {
  void * p = xmalloc(size);
  memset(p, 0, size);
  return p;
}

static inline void xfree(void * p) {
  free(p);
}

/* Memory-related */

static inline silc_obj* get_oref(struct silc_mem_t* mem, silc_obj obj) {
  int content_length = 0;
  silc_obj* result = NULL;
  int subtype = silc_int_mem_parse_ref(mem, obj, &content_length, NULL, &result);
  assert(result != NULL && subtype >= 0);
  return result;
}

/* Memory management-related */

static void* root_obj_iter_start(struct silc_mem_init_t* mem_init) {
  return (void *)1;
}

static void* root_obj_iter_next(struct silc_mem_init_t* mem_init,
                                void* iter_context,
                                silc_obj** objs, int* size) {
  void* result = NULL;
  struct silc_ctx_t* c = mem_init->context;

  if (iter_context == ((void *)1)) {
    *objs = c->stack;
    *size = c->stack_end;
    result = (void*)2;
  } else if (iter_context == ((void *)2)) {
    *objs = &c->root_cons;
    *size = 1;
  }

  return result;
}

/* Service functions */

#define SILC_DEFAULT_STACK_SIZE           (1024)

#define SILC_DEFAULT_SYM_ARR_SIZE         (256)
#define SILC_DEFAULT_SYM_HT_SIZE          (512)
#define SILC_DEFAULT_SYM_HT_LOAD_FACTOR   (0.75)

#define SILC_DEFAULT_CONS_ARR_SIZE        (256)

static void oom_abort(struct silc_mem_init_t* init) {
  fputs(";; Out of heap\n", stderr);
  abort();
}

static void init_mem(struct silc_ctx_t* c) {
  struct silc_mem_init_t* init = xmallocz(sizeof(struct silc_mem_init_t));

  init->context = c;
  init->init_memory_size = 1024 * 1024;
  init->max_memory_size = 16 * 1024 * 1024;
  init->root_obj_iter_start = root_obj_iter_start;
  init->root_obj_iter_next = root_obj_iter_next;
  init->oom_abort = oom_abort;
  init->alloc_mem = xmalloc;
  init->free_mem = xfree;

  struct silc_mem_t* mem = xmallocz(sizeof(struct silc_mem_t));
  silc_int_mem_init(mem, init);

  c->mem_init = init;
  c->mem = mem;
}

static void init_globals(struct silc_ctx_t* c) {
  c->sym_hash_table = silc_hash_table(c, 8179/*prime*/);
  c->root_cons = silc_cons(c, c->sym_hash_table, SILC_OBJ_NIL);
}

static silc_fn_ptr g_silc_builtin_functions[] = {
  &silc_internal_fn_print,
  &silc_internal_fn_inc,
  &silc_internal_fn_plus,
  &silc_internal_fn_quit
};

static int find_fn_pos(struct silc_ctx_t* c, silc_fn_ptr fn_ptr) {
  for (int i = 0; i < c->fn_count; ++i) {
    if (c->fn_array[i] == fn_ptr) {
      return i;
    }
  }
  return -1;
}

#define SILC_FN_SPECIAL       (1 << 1)
#define SILC_FN_BUILTIN       (1 << 2)

static void add_function(struct silc_ctx_t* c, const char* symbol_name, silc_fn_ptr fn_ptr, bool special) {
  int fn_index = find_fn_pos(c, fn_ptr);
  if (fn_index < 0) {
    fputs("Unregistered function", stderr); /* should not happen */
    abort();
  }

  int fn_flags = (special ? SILC_FN_SPECIAL : 0) | SILC_FN_BUILTIN;

  /* alloc function */
  silc_obj content[] = { silc_int_to_obj(fn_index), silc_int_to_obj(fn_flags) };
  silc_obj fn = silc_int_mem_alloc(c->mem, countof(content), content, SILC_TYPE_OREF, SILC_OREF_FUNCTION_SUBTYPE);

  /* assoc that function with a symbol */
  silc_obj sym = silc_sym_from_buf(c, symbol_name, strlen(symbol_name));
  silc_obj prev_assoc = silc_set_sym_assoc(c, sym, fn);
  assert(silc_try_get_err_code(prev_assoc) == SILC_ERR_UNRESOLVED_SYMBOL);
}

static void init_builtins(struct silc_ctx_t* c) {
  /* functions array */
  c->fn_array = g_silc_builtin_functions;
  c->fn_count = countof(g_silc_builtin_functions);

  add_function(c, "inc", &silc_internal_fn_inc, false);
  add_function(c, "+", &silc_internal_fn_plus, false);

  add_function(c, "print", &silc_internal_fn_print, false);

  add_function(c, "quit", &silc_internal_fn_quit, false);
}

struct silc_ctx_t * silc_new_context() {
  struct silc_ctx_t* c = xmallocz(sizeof(struct silc_ctx_t));

  /* settings */
  struct silc_settings_t * s = xmallocz(sizeof(struct silc_settings_t));
  s->out = stdout;
  c->settings = s;

  /* stack */
  c->stack = xmallocz(SILC_DEFAULT_STACK_SIZE * sizeof(silc_obj));
  c->stack_size = SILC_DEFAULT_STACK_SIZE;

  /* heap memory */
  init_mem(c);

  /* globals */
  init_globals(c);
  init_builtins(c);

  return c;
}

void silc_free_context(struct silc_ctx_t * c) {
  silc_int_mem_free(c->mem);
  xfree(c->mem);
  xfree(c->settings);
  xfree(c->stack);

  xfree(c);
}

void silc_gc(struct silc_ctx_t* c) {
  silc_int_mem_gc(c->mem);
}

void silc_set_exit_code(struct silc_ctx_t* c, int code) {
  c->exit_code = code;
}

int silc_get_exit_code(struct silc_ctx_t* c) {
  return c->exit_code;
}

silc_obj silc_load(struct silc_ctx_t* c, const char* file_name) {
  fprintf(stdout, ";; Loading %s...\n", file_name);
//  if (silc_try_get_err_code(o) > 0) {
//    fprintf(stderr, ";; Error while loading %s\n", file_name);
//    silc_print(c, o, stderr);
//    fputc('\n', stderr);
//  } else {
//    fprintf(stdout, ";; Loaded %s\n", file_name);
//  }
  fprintf(stderr, ";; Error while loading %s: \n", file_name);

  /* TODO: implement */
  return silc_err_from_code(SILC_ERR_INTERNAL);
}

/**
 * Converts error code to string.
 */
const char* silc_err_code_to_str(int code) {
  assert(code > 0);
  switch (code) {
    case SILC_ERR_INTERNAL:
      return "internal error";

    case SILC_ERR_STACK_ACCESS:
      return "stack access error";

    case SILC_ERR_STACK_OVERFLOW:
      return "stack overflow";

    case SILC_ERR_INVALID_ARGS:
      return "invalid arguments";

    case SILC_ERR_VALUE_OUT_OF_RANGE:
      return "value is out of range";

    case SILC_ERR_UNEXPECTED_EOF:
      return "unexpected end of file";

    case SILC_ERR_UNEXPECTED_CHARACTER:
      return "unexpected character";

    case SILC_ERR_SYMBOL_TOO_BIG:
      return "symbol string is too big";

    case SILC_ERR_UNRESOLVED_SYMBOL:
      return "unresolved symbol";

    case SILC_ERR_NOT_A_FUNCTION:
      return "object is not a function";

    default:
      return "unknown error";
  }
}

/*
 * Helper functions
 */

FILE* silc_get_default_out(struct silc_ctx_t * c) {
  return c->settings->out;
}

silc_obj silc_eq(struct silc_ctx_t* c, silc_obj lhs, silc_obj rhs) {
  if (lhs == rhs) {
    return SILC_OBJ_TRUE;
  }

  int type = SILC_GET_TYPE(lhs);
  if (SILC_GET_TYPE(rhs) != type) {
    /* different types */
    return SILC_OBJ_FALSE;
  }

  int subtype;

  int lhs_len = 0;
  silc_obj* lhs_po = NULL;
  char* lhs_pb = NULL;

  silc_obj* rhs_po = NULL;
  char* rhs_pb = NULL;
  int rhs_len = 0;

  switch (type) {
    case SILC_TYPE_INL:
      return SILC_OBJ_FALSE; /* expected literal match for inline types */


    case SILC_TYPE_CONS:
      lhs_po = silc_parse_cons(c->mem, lhs);
      rhs_po = silc_parse_cons(c->mem, rhs);
      if (SILC_OBJ_TRUE == silc_eq(c, lhs_po[0], rhs_po[0])) {
        return silc_eq(c, lhs_po[1], rhs_po[1]);
      }
      return SILC_OBJ_FALSE;

    case SILC_TYPE_OREF:
      subtype = silc_int_mem_parse_ref(c->mem, lhs, &lhs_len, &lhs_pb, &lhs_po);
      if (silc_int_mem_parse_ref(c->mem, rhs, &rhs_len, &rhs_pb, &rhs_po) != subtype) {
        return SILC_OBJ_FALSE;
      }

      if (subtype == SILC_OREF_SYMBOL_SUBTYPE) {
        return SILC_OBJ_FALSE; /* literal match expected for symbols */
      }

      /* general purpose match */
      if (lhs_len != rhs_len) {
        return SILC_OBJ_FALSE;
      }

      assert(lhs_po != NULL && rhs_po != NULL);
      for (int i = 0; i < lhs_len; ++lhs_len) {
        if (SILC_OBJ_TRUE != silc_eq(c, lhs_po[i], rhs_po[i])) {
          return SILC_OBJ_FALSE;
        }
      }
      return SILC_OBJ_TRUE;

    case SILC_TYPE_BREF:
      subtype = silc_int_mem_parse_ref(c->mem, lhs, &lhs_len, &lhs_pb, &lhs_po);
      if (silc_int_mem_parse_ref(c->mem, rhs, &rhs_len, &rhs_pb, &rhs_po) != subtype) {
        return SILC_OBJ_FALSE;
      }

      /* general purpose match */
      if (lhs_len != rhs_len) {
        return SILC_OBJ_FALSE;
      }

      assert(lhs_pb != NULL && rhs_pb != NULL);
      return (0 == memcmp(lhs_pb, rhs_pb, lhs_len)) ? SILC_OBJ_TRUE : SILC_OBJ_FALSE;
  }

  assert(!"Not implemented");
  return SILC_OBJ_FALSE;
}

int silc_get_ref_subtype(struct silc_ctx_t* c, silc_obj o) {
  return silc_int_mem_parse_ref(c->mem, o, NULL, NULL, NULL);
}

silc_obj silc_define_function(struct silc_ctx_t* c, silc_obj arg_list, silc_obj body) {
  if (SILC_GET_TYPE(arg_list) != SILC_TYPE_CONS) {
    return silc_err_from_code(SILC_ERR_INVALID_ARGS);
  }

  silc_obj content[35] = { silc_int_to_obj(-1), SILC_OBJ_ZERO, body };
  int count = 3;

  silc_obj cdr = arg_list;
  for (;;) {
    if (cdr == SILC_OBJ_NIL) {
      break;
    }

    if (SILC_GET_TYPE(cdr) != SILC_TYPE_CONS) {
      return silc_err_from_code(SILC_ERR_INVALID_ARGS);
    }

    silc_obj* cdr_contents = silc_parse_cons(c->mem, cdr);
    silc_obj car = cdr_contents[0];
    cdr = cdr_contents[1];

    int new_count = count + 1;
    if (new_count >= countof(content)) {
      return silc_err_from_code(SILC_ERR_INVALID_ARGS);
    }

    if (silc_get_ref_subtype(c, car) != SILC_OREF_SYMBOL_SUBTYPE) {
      return silc_err_from_code(SILC_ERR_INVALID_ARGS);
    }

    content[count] = car;
    count = new_count;
  }

  /* alloc function */
  return silc_int_mem_alloc(c->mem, count, content, SILC_TYPE_OREF, SILC_OREF_FUNCTION_SUBTYPE);
}

/*
 * Hash table
 */

static silc_obj silc_hash_table(struct silc_ctx_t* c, int initial_size) {
  silc_obj result = silc_int_mem_alloc(c->mem, initial_size + 1, NULL, SILC_TYPE_OREF, SILC_OREF_HASHTABLE_SUBTYPE);
  get_oref(c->mem, result)[0] = silc_int_to_obj(0); /* element count */
  return result;
}

static inline silc_obj str_hash_code(const char* buf, int size) {
  int result = 0;
  for (int i = 0; i < size; ++i) {
    result = 31 * result + buf[i];
  }
  return silc_int_to_obj(result % SILC_MAX_INT);
}

/* Sym */

static silc_obj get_sym_info(struct silc_ctx_t* c, silc_obj o, silc_obj* sym_str, silc_obj* hash_code) {
  int len = 0;
  silc_obj* obj_contents = NULL;
  char* byte_contents = NULL;
  int subtype = silc_int_mem_parse_ref(c->mem, o, &len, &byte_contents, &obj_contents);
  if (subtype != SILC_OREF_SYMBOL_SUBTYPE) {
    return SILC_OBJ_NIL;
  }
  assert(len == 3 && obj_contents != NULL);

  if (hash_code != NULL) {
    *hash_code = obj_contents[0];
  }
  if (sym_str != NULL) {
    *sym_str = obj_contents[1];
  }
  return obj_contents[2];
}


/*
 * Object-specific functions (allocations, etc)
 */

static int compare_str(struct silc_ctx_t* c, silc_obj str, const char* buf, int size) {
  int len = 0;
  char* char_content = NULL;
  int subtype = silc_int_mem_parse_ref(c->mem, str, &len, &char_content, NULL);
  assert(SILC_BREF_STR_SUBTYPE == subtype);

  int size_diff = len - size;
  if (size_diff != 0) {
    return size_diff;
  }

  return memcmp(buf, char_content, size);
}
 
static int is_same_sym_str(struct silc_ctx_t* c, silc_obj sym, const char* buf, int size, silc_obj hash_code) {
  silc_obj sym_str = SILC_OBJ_NIL;
  silc_obj sym_hash_code = SILC_OBJ_ZERO;
  get_sym_info(c, sym, &sym_str, &sym_hash_code);

  if (sym_hash_code != hash_code) {
    return 0;
  }

  return compare_str(c, sym_str, buf, size) == 0;
}

silc_obj silc_sym_from_buf(struct silc_ctx_t* c, const char* buf, int size) {
  int hash_table_size = 0;
  silc_obj* hash_table_contents;
  int hash_table_subtype = silc_int_mem_parse_ref(c->mem, c->sym_hash_table, &hash_table_size, NULL, &hash_table_contents);

  assert(hash_table_subtype == SILC_OREF_HASHTABLE_SUBTYPE && hash_table_size > 0 && hash_table_contents != NULL);

  int hash_table_count = silc_obj_to_int(hash_table_contents[0]);
  silc_obj* hash_table = hash_table_contents + 1;

  /* lookup and optional insert */
  silc_obj hash_code_obj = str_hash_code(buf, size);
  int pos = silc_obj_to_int(hash_code_obj) % hash_table_size;

  silc_obj* pc = NULL;
  silc_obj hash_table_cell = hash_table[pos];
  for (silc_obj cell = hash_table_cell; cell != SILC_OBJ_NIL; cell = pc[1]) {
    pc = silc_parse_cons(c->mem, cell); /* go to next */
    silc_obj other_sym = pc[0];
    if (is_same_sym_str(c, other_sym, buf, size, hash_code_obj)) {
      return other_sym;
    }
  }

  /* create new entry */
  silc_obj contents[] = {
    hash_code_obj,              /* [0] symbol string's hash code */
    silc_str(c, buf, size),     /* [1] symbol string */
    silc_err_from_code(SILC_ERR_UNRESOLVED_SYMBOL) /* [2] assoc (initially unresolved) */
  };
  silc_obj result = silc_int_mem_alloc(c->mem, 3, contents, SILC_TYPE_OREF, SILC_OREF_SYMBOL_SUBTYPE);

  /* insert that entry to the hash table */
  hash_table[pos] = silc_cons(c, result, hash_table_cell);
  /* update count */
  hash_table[0] = silc_int_to_obj(hash_table_count + 1);

  return result;
}

silc_obj silc_get_sym_info(struct silc_ctx_t* c, silc_obj o, silc_obj* sym_str) {
  return get_sym_info(c, o, sym_str, NULL);
}

silc_obj silc_set_sym_assoc(struct silc_ctx_t* c, silc_obj o, silc_obj new_assoc) {
  int len = 0;
  silc_obj* obj_contents = NULL;
  int subtype = silc_int_mem_parse_ref(c->mem, o, &len, NULL, &obj_contents);
  if (subtype != SILC_OREF_SYMBOL_SUBTYPE) {
    return SILC_ERR_INVALID_ARGS;
  }

  assert(len == 3 && obj_contents != NULL);
  silc_obj old_assoc = obj_contents[2];
  obj_contents[2] = new_assoc;
  return old_assoc;
}

silc_obj silc_str(struct silc_ctx_t* c, const char* buf, int size) {
  return silc_int_mem_alloc(c->mem, size, buf, SILC_TYPE_BREF, SILC_BREF_STR_SUBTYPE);
}

int silc_get_str_chars(struct silc_ctx_t* c, silc_obj o, char* buf, int pos, int size) {
  int len = 0;
  char* char_content = NULL;
  silc_obj* obj_content = NULL;
  int result = 0;
  if (silc_int_mem_parse_ref(c->mem, o, &len, &char_content, &obj_content) == SILC_BREF_STR_SUBTYPE) {
    assert(len >= 0 && char_content != NULL);
    result = ((pos + size) < len ? size : len - pos);
    if (result > 0 && char_content != NULL) {
      memcpy(buf, char_content + pos, result);
    } else {
      result = 0;
    }
  }
  return result;
}

silc_obj silc_cons(struct silc_ctx_t* c, silc_obj car, silc_obj cdr) {
  silc_obj contents[] = { car, cdr };
  return silc_int_mem_alloc(c->mem, 2, contents, SILC_TYPE_CONS, SILC_INT_MEM_CONS_SUBTYPE);
}

static inline silc_obj get_cons_cell(struct silc_ctx_t* c, silc_obj cons, int pos) {
  silc_obj result;
  if (SILC_GET_TYPE(cons) == SILC_TYPE_CONS) {
    silc_obj* contents = silc_parse_cons(c->mem, cons);
    result = contents[pos];
  } else {
    result = SILC_OBJ_NIL;
  }
  return result;
}

silc_obj silc_car(struct silc_ctx_t* c, silc_obj cons) {
  return get_cons_cell(c, cons, 0);
}

silc_obj silc_cdr(struct silc_ctx_t* c, silc_obj cons) {
  return get_cons_cell(c, cons, 1);
}

/*
 * Evaluation
 */

static silc_obj push_arguments(struct silc_ctx_t* c, silc_obj cdr, bool special_form) {
  while (cdr != SILC_OBJ_NIL) {
    silc_obj car;

    /* get next element */
    if (SILC_GET_TYPE(cdr) == SILC_TYPE_CONS) {
      silc_obj* contents = silc_parse_cons(c->mem, cdr);
      car = contents[0];
      cdr = contents[1];
    } else {
      /* dotted form */
      car = cdr;
      cdr = SILC_OBJ_NIL;
    }

    /* evaluate if this is not a special form */
    if (!special_form) {
      car = silc_eval(c, car);
    }

    /* calculate new stack pointer position */
    int new_stack_end = c->stack_end + 1;
    if (new_stack_end >= c->stack_size) {
      return silc_err_from_code(SILC_ERR_STACK_OVERFLOW);
    }

    /* insert element to the stack and update last element index */
    c->stack[c->stack_end] = car;
    c->stack_end = new_stack_end;
  }
  return SILC_OBJ_NIL;
}

static silc_obj eval_cons(struct silc_ctx_t* c, silc_obj cons) {
  /* Parse cons */
  silc_obj* cons_contents = silc_parse_cons(c->mem, cons);

  /* Get CAR and try evaluate it to function */
  silc_obj fn = silc_eval(c, cons_contents[0]);

  /* Try parse as a function */
  int len = 0;
  silc_obj* fn_contents = NULL;
  int subtype = silc_int_mem_parse_ref(c->mem, fn, &len, NULL, &fn_contents);
  if (subtype != SILC_OREF_FUNCTION_SUBTYPE) {
    return silc_err_from_code(SILC_ERR_NOT_A_FUNCTION);
  }

  /* parse function */
  int fn_pos = silc_obj_to_int(fn_contents[0]);
  int fn_flags = silc_obj_to_int(fn_contents[1]);
  assert(fn_pos >= 0 && fn_pos < c->fn_count);
  silc_fn_ptr fn_ptr = c->fn_array[fn_pos];

  /* save stack state */
  int prev_end = c->stack_end;

  /* put arguments to the function stack */
  silc_obj result = push_arguments(c, cons_contents[1], fn_flags & SILC_FN_SPECIAL);
  if (silc_try_get_err_code(result) < 0) {
    /* ok, now prepare function call context */
    struct silc_funcall_t funcall = {
      .ctx = c,
      .argc = c->stack_end - prev_end,
      .argv = c->stack + prev_end
    };
    assert(funcall.argc >= 0);

    /* call that function */
    result = fn_ptr(&funcall);
  }

  /* Restore stack state */
  c->stack_end = prev_end;

  return result;
}

static silc_obj eval_oref(struct silc_ctx_t* c, silc_obj o) {
  int len = 0;
  silc_obj* contents = NULL;
  int subtype = silc_int_mem_parse_ref(c->mem, o, &len, NULL, &contents);
  if (subtype != SILC_OREF_SYMBOL_SUBTYPE) {
    return o; /* Non-symbolic objects evaluate to themselves */
  }

  /* object is a symbol, we need to return an association */
  return contents[2];
}

silc_obj silc_eval(struct silc_ctx_t* c, silc_obj o) {
  switch (SILC_GET_TYPE(o)) {
    case SILC_TYPE_CONS:
      return eval_cons(c, o);

    case SILC_TYPE_OREF:
      return eval_oref(c, o);
  }
  return o; /* all the other object types evaluate to themselves */
}
