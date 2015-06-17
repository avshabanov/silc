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


/*******************************************************************************
 * Helper macros                                                               *
 *******************************************************************************/

#ifndef countof
#define countof(arr)    (sizeof(arr) / sizeof(arr[0]))
#endif

/*******************************************************************************
 * Forward declarations                                                        *
 *******************************************************************************/


/* Mem management */
static void * xmalloc(size_t size);
static void * xmallocz(size_t size);
static inline void xfree(void * p);


/*******************************************************************************
 * Types                                                                       *
 *******************************************************************************/


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
  silc_obj              sym_name_hash_table;

  /* builtin functions */
  silc_fn_ptr *         fn_array;
  int                   fn_count;

  /* current environment */
  silc_obj              current_env;

  /* begin function */
  silc_obj              lambda_begin;

  /* exit code */
  int                   exit_code;
};

struct silc_settings_t {
  FILE *    out;        /* default output stream (used in print function) */
};


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
  SILC_ASSERT(result != NULL && subtype >= 0);
  return result;
}

/* Memory management-related */

#define ROIS_ENTRY1   ((void*)1)
#define ROIS_ENTRY2   ((void*)2)
#define ROIS_ENTRY3   ((void*)3)

static void* root_obj_iter_start(struct silc_mem_init_t* mem_init) {
  return ROIS_ENTRY1;
}

static void* root_obj_iter_next(struct silc_mem_init_t* mem_init,
                                void* iter_context,
                                silc_obj** objs, int* size) {
  void* result = NULL;
  struct silc_ctx_t* c = mem_init->context;

  if (iter_context == ROIS_ENTRY1) {
    *objs = c->stack;
    *size = c->stack_end;
    result = ROIS_ENTRY2;
  } else if (iter_context == ROIS_ENTRY2) {
    *objs = &c->root_cons;
    *size = 1;
    result = ROIS_ENTRY3;
  } else if (iter_context == ROIS_ENTRY3) {
    *objs = &c->current_env;
    *size = 1;
    result = NULL; /* no more roots */
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
  c->sym_name_hash_table = silc_hash_table(c, 8179/*prime*/);
  c->root_cons = silc_cons(c, c->sym_name_hash_table, SILC_OBJ_NIL);
}

static silc_fn_ptr g_silc_builtin_functions[] = {
  &silc_internal_fn_load,
  &silc_internal_fn_define,
  &silc_internal_fn_lambda,
  &silc_internal_fn_print,
  &silc_internal_fn_cons,
  &silc_internal_fn_inc,
  &silc_internal_fn_plus,
  &silc_internal_fn_minus,
  &silc_internal_fn_div,
  &silc_internal_fn_mul,
  &silc_internal_fn_begin,
  &silc_internal_fn_gc,
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

/*
 * Hash table functions
 */

static int obj_hash_code(silc_obj o, int table_size) {
  return (((o >> SILC_INT_TYPE_SHIFT) % table_size) + table_size) % table_size;
}

static bool are_same_objects(silc_obj lhs, silc_obj rhs) {
  return lhs == rhs;
}

static silc_obj* lookup_hash_table_cell(struct silc_ctx_t* c, silc_obj hash_table, silc_obj key) {
  int size = 0;
  silc_obj* contents;
  int subtype = silc_int_mem_parse_ref(c->mem, hash_table, &size, NULL, &contents);
  SILC_ASSERT(subtype == SILC_OREF_HASHTABLE_SUBTYPE && size > 0 && contents != NULL);

  /* get position in a hash table */
  int pos = 1 + obj_hash_code(key, size - 1);

  /* cell found, now iterate over the  */
  return contents + pos;
}

silc_obj silc_hash_table(struct silc_ctx_t* c, int initial_size) {
  silc_obj result = silc_int_mem_alloc(c->mem, initial_size + 1, NULL, SILC_TYPE_OREF, SILC_OREF_HASHTABLE_SUBTYPE);
  get_oref(c->mem, result)[0] = silc_int_to_obj(0); /* element count */
  return result;
}

silc_obj silc_hash_table_get(struct silc_ctx_t* c, silc_obj hash_table, silc_obj key, silc_obj not_found_val) {
  silc_obj* entry_list_ptr = lookup_hash_table_cell(c, hash_table, key);

  for (silc_obj cell = *entry_list_ptr; cell != SILC_OBJ_NIL;) {
    silc_obj* cell_contents = silc_parse_cons(c->mem, cell);
    silc_obj cur_entry = cell_contents[0]; /* get current hash table entry */

    silc_obj* cur_entry_contents = silc_parse_cons(c->mem, cur_entry);
    if (are_same_objects(key, cur_entry_contents[0])) {
      return cur_entry_contents[1]; /* return value */
    }

    cell = cell_contents[1]; /* go to next cell */
  }

  return not_found_val;
}

silc_obj silc_hash_table_put(struct silc_ctx_t* c, silc_obj hash_table, silc_obj key, silc_obj value, silc_obj not_found_val) {
  silc_obj* entry_list_ptr = lookup_hash_table_cell(c, hash_table, key);

  for (silc_obj cell = *entry_list_ptr; cell != SILC_OBJ_NIL;) {
    silc_obj* cell_contents = silc_parse_cons(c->mem, cell);
    silc_obj cur_entry = cell_contents[0]; /* get current hash table entry */

    silc_obj* cur_entry_contents = silc_parse_cons(c->mem, cur_entry);
    if (are_same_objects(key, cur_entry_contents[0])) {
      silc_obj existing_value = cur_entry_contents[1];
      cur_entry_contents[1] = value; /* override old value in place */
      return existing_value;
    }

    cell = cell_contents[1]; /* go to next cell */
  }

  /* no entry found, insert a new one */
  silc_obj new_entry = silc_cons(c, key, value);
  *entry_list_ptr = silc_cons(c, new_entry, *entry_list_ptr);
  return not_found_val;
}

/*
 * Function argument list checking utility
 */

static silc_obj check_args(struct silc_ctx_t* c, silc_obj arg_list) {
  if (SILC_GET_TYPE(arg_list) != SILC_TYPE_CONS && arg_list != SILC_OBJ_NIL) {
    return silc_err_from_code(SILC_ERR_INVALID_ARGS);
  }

  for (silc_obj cdr = arg_list; cdr != SILC_OBJ_NIL;) {
    if (SILC_GET_TYPE(cdr) != SILC_TYPE_CONS) {
      return silc_err_from_code(SILC_ERR_INVALID_ARGS);
    }

    silc_obj* cdr_contents = silc_parse_cons(c->mem, cdr);
    silc_obj car = cdr_contents[0];
    cdr = cdr_contents[1];

    if (silc_get_ref_subtype(c, car) != SILC_OREF_SYMBOL_SUBTYPE) {
      return silc_err_from_code(SILC_ERR_INVALID_ARGS);
    }
  }

  return SILC_OBJ_NIL; /* OK */
}

/*
 * Environment creation
 */

/*
 * Function definition utilities
 */

#define SILC_FN_SPECIAL       (1 << 1)
#define SILC_FN_BUILTIN       (1 << 2)

static silc_obj create_function(struct silc_ctx_t* c,
                                int flags,
                                silc_obj prev_env,
                                silc_fn_ptr fn_ptr,
                                silc_obj arg_list,
                                silc_obj body) {
  silc_obj body_entry;
  if (fn_ptr != NULL) {
    SILC_ASSERT(body == SILC_OBJ_NIL || !"Function body should be null if fn_ptr is not null");
    int fn_index = find_fn_pos(c, fn_ptr);
    if (fn_index < 0) {
      fputs("Unregistered function\n", stderr); /* should not happen */
      abort();
    }
    body_entry = silc_int_to_obj(fn_index); /* no body per se, function index */
  } else {
    body_entry = body;
  }

  SILC_CHECKED_DECLARE(arg_check, check_args(c, arg_list));
  SILC_CHECKED_DECLARE(env, prev_env);

  /* alloc function */
  silc_obj content[] = {
    silc_int_to_obj(flags), /* function flags */
    env,                    /* function environment */
    body_entry,             /* function body */
    arg_list                /* function arguments */
  };

  return silc_int_mem_alloc(c->mem, countof(content), content, SILC_TYPE_OREF, SILC_OREF_FUNCTION_SUBTYPE);
}

static silc_obj add_builtin_function(struct silc_ctx_t* c, const char* symbol_name, silc_fn_ptr fn_ptr, bool special) {
  /* create function */
  silc_obj fn = create_function(c, (special ? SILC_FN_SPECIAL : 0) | SILC_FN_BUILTIN, SILC_OBJ_NIL, fn_ptr,
    SILC_OBJ_NIL, SILC_OBJ_NIL);
  int errcode = silc_try_get_err_code(fn);
  if (errcode > 0) {
    fprintf(stderr, ";; FATAL: unable to register function %s, errcode=%d\n", symbol_name, errcode);
    abort();
  }

  /* assoc that function with a symbol */
  silc_obj sym = silc_sym_from_buf(c, symbol_name, strlen(symbol_name));
  silc_obj prev_assoc = silc_set_sym_assoc(c, sym, fn);
  SILC_ASSERT(silc_try_get_err_code(prev_assoc) == SILC_ERR_UNRESOLVED_SYMBOL);

  return fn;
}

static void init_builtins(struct silc_ctx_t* c) {
  /* functions array */
  c->fn_array = g_silc_builtin_functions;
  c->fn_count = countof(g_silc_builtin_functions);

  add_builtin_function(c, "inc", &silc_internal_fn_inc, false);

  add_builtin_function(c, "+", &silc_internal_fn_plus, false);
  add_builtin_function(c, "-", &silc_internal_fn_minus, false);
  add_builtin_function(c, "/", &silc_internal_fn_div, false);
  add_builtin_function(c, "*", &silc_internal_fn_mul, false);

  add_builtin_function(c, "cons", &silc_internal_fn_cons, false);
  add_builtin_function(c, "print", &silc_internal_fn_print, false);

  add_builtin_function(c, "load", &silc_internal_fn_load, false);

  add_builtin_function(c, "define", &silc_internal_fn_define, true);
  add_builtin_function(c, "lambda", &silc_internal_fn_lambda, true);

  c->lambda_begin = add_builtin_function(c, "begin", &silc_internal_fn_begin, false);

  add_builtin_function(c, "gc", &silc_internal_fn_gc, false);
  add_builtin_function(c, "quit", &silc_internal_fn_quit, false);
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

silc_obj silc_get_lambda_begin(struct silc_ctx_t* c) {
  return c->lambda_begin;
}

/**
 * Converts error code to string.
 */
const char* silc_err_code_to_str(int code) {
  SILC_ASSERT(code > 0);
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

FILE* silc_set_default_out(struct silc_ctx_t * c, FILE* f) {
  FILE* old = c->settings->out;
  c->settings->out = f;
  return old;
}

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

      SILC_ASSERT(lhs_po != NULL && rhs_po != NULL);
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

      SILC_ASSERT(lhs_pb != NULL && rhs_pb != NULL);
      return (0 == memcmp(lhs_pb, rhs_pb, lhs_len)) ? SILC_OBJ_TRUE : SILC_OBJ_FALSE;
  }

  SILC_ASSERT(!"Not implemented");
  return SILC_OBJ_FALSE;
}

int silc_get_ref_subtype(struct silc_ctx_t* c, silc_obj o) {
  return silc_int_mem_parse_ref(c->mem, o, NULL, NULL, NULL);
}

silc_obj silc_define_function(struct silc_ctx_t* c, silc_obj arg_list, silc_obj body) {
  return create_function(c, 0, c->current_env, NULL, arg_list, body);
}

/*
 * String hash code calculation
 */

static inline int calc_hash_code(const char* buf, int size, int modulo) {
  int result = 0;
  for (int i = 0; i < size; ++i) {
    result = 31 * result + buf[i];
  }
  return ((result % modulo) + modulo) % modulo;
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
  SILC_ASSERT(len == 3 && obj_contents != NULL);

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
  SILC_ASSERT(SILC_BREF_STR_SUBTYPE == subtype);

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
  int hash_table_subtype = silc_int_mem_parse_ref(c->mem, c->sym_name_hash_table, &hash_table_size, NULL, &hash_table_contents);

  SILC_ASSERT(hash_table_subtype == SILC_OREF_HASHTABLE_SUBTYPE && hash_table_size > 0 && hash_table_contents != NULL);

  int hash_table_count = silc_obj_to_int(hash_table_contents[0]);

  /* lookup and optional insert */
  int hash_code = calc_hash_code(buf, size, SILC_MAX_INT);
  SILC_ASSERT((hash_code >= 0) && (hash_code < SILC_MAX_INT));

  int pos_modulo = hash_table_size - 1; // here and below: 1 is a service information size
  int pos = 1 + (hash_code % pos_modulo);
  silc_obj hash_code_obj = silc_int_to_obj(hash_code);

  silc_obj* pc = NULL;
  silc_obj hash_table_cell = hash_table_contents[pos];
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
  hash_table_contents[pos] = silc_cons(c, result, hash_table_cell);

  /* update count */
  hash_table_contents[0] = silc_int_to_obj(hash_table_count + 1);

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

  SILC_ASSERT(len == 3 && obj_contents != NULL);
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
  int result = 0;
  if (silc_int_mem_parse_ref(c->mem, o, &len, &char_content, NULL) == SILC_BREF_STR_SUBTYPE) {
    SILC_ASSERT(len >= 0 && char_content != NULL);
    result = ((pos + size) < len ? size : len - pos);
    if (result > 0 && char_content != NULL) {
      memcpy(buf, char_content + pos, result);
    } else {
      result = 0;
    }
  }
  return result;
}

silc_obj silc_byte_buf(struct silc_ctx_t* c, int byte_len) {
  return silc_int_mem_alloc(c->mem, byte_len, NULL, SILC_TYPE_BREF, SILC_BREF_BUFFER_SUBTYPE);
}

int silc_byte_buf_get(struct silc_ctx_t* c, silc_obj o, char** result) {
  int len = 0;
  char* char_content = NULL;
  if (silc_int_mem_parse_ref(c->mem, o, &len, &char_content, NULL) == SILC_BREF_BUFFER_SUBTYPE) {
    SILC_ASSERT(char_content != NULL && len >= 0);
    *result = char_content;
    return len;
  }
  return -1;
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

#if 0
static silc_obj create_env(struct silc_ctx_t* c, silc_obj name_val_pairs, silc_obj prev_env) {
  if (name_val_pairs == SILC_OBJ_NIL) {
    /* optimization: if function takes no args, it does not need a new enviroment, it can reuse an old one */
    return prev_env;
  }

  /* alloc function */
  silc_obj content[] = {
    prev_env,             /* [0] function flags */
    name_val_pairs        /* [1] saved name:argument pairs */
  };

  silc_obj result = silc_int_mem_alloc(c->mem, countof(content), content, SILC_TYPE_OREF, SILC_OREF_ENVIRONMENT_SUBTYPE);
  return result;
}
#endif

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

static silc_obj call_builtin(struct silc_ctx_t* c, silc_obj* cons_contents, silc_obj* fn_contents, bool special) {
  SILC_ASSERT(SILC_OBJ_NIL == fn_contents[1] && /* environment should always be null for builtin functions */
              SILC_OBJ_NIL == fn_contents[3] /* builtin function arglist should also be null */);

  /* save stack state */
  int prev_end = c->stack_end;

  /* put arguments to the function stack */
  silc_obj result = push_arguments(c, cons_contents[1], special);
  if (silc_try_get_err_code(result) < 0) {
    int fn_pos = silc_obj_to_int(fn_contents[2]); /* function position */
    SILC_ASSERT(fn_pos >= 0 && fn_pos < c->fn_count);
    silc_fn_ptr fn_ptr = c->fn_array[fn_pos];

    /* ok, now prepare function call context */
    struct silc_funcall_t funcall = {
      .ctx = c,
      .argc = c->stack_end - prev_end,
      .argv = c->stack + prev_end
    };
    SILC_ASSERT(funcall.argc >= 0);

    /* call that function */
    result = fn_ptr(&funcall);
  }

  /* Restore stack state */
  c->stack_end = prev_end;

  return result;
}

//static silc_obj append_args(struct silc_ctx_t* c, silc_obj arg_names, silc_obj prev_arg_value_pairs) {
//  silc_obj result = prev_arg_value_pairs;
//  for (silc_obj it = arg_names; it != SILC_OBJ_NIL; it = silc_cdr(c, it)) {
//    silc_obj arg_name = silc_car(c, it);
//    silc_obj arg_value = silc_get_sym_info(c, arg_name, NULL);
//
//    result = silc_cons(c, silc_cons(c, arg_name, arg_value), result);
//  }
//  return result;
//}

static void restore_args(struct silc_ctx_t* c, silc_obj arg_value_pairs) {
  for (silc_obj it = arg_value_pairs; it != SILC_OBJ_NIL; it = silc_cdr(c, it)) {
    silc_obj entry = silc_car(c, it);
    silc_obj arg_name = silc_car(c, entry);
    silc_obj arg_value = silc_cdr(c, entry);

    silc_set_sym_assoc(c, arg_name, arg_value);
  }
}

static silc_obj call_lambda(struct silc_ctx_t* c, silc_obj arg_values, silc_obj* fn_contents) {
  silc_obj result = SILC_OBJ_NIL;
  silc_obj prev_env = c->current_env;
  silc_obj arg_names = fn_contents[3];
  silc_obj saved_arg_value_pairs = SILC_OBJ_NIL;

  /* prepare environment */
  silc_obj new_env = fn_contents[1]; /* use this function's environment as a base */

  /* assoc args */
  silc_obj val_it = arg_values;
  for (silc_obj it = arg_names; it != SILC_OBJ_NIL; it = silc_cdr(c, it)) {
    silc_obj sym = silc_car(c, it);

    /* assoc arg */
    if (val_it == SILC_OBJ_NIL) {
      result = silc_err_from_code(SILC_ERR_INVALID_ARGS); /* invalid number of args */
      goto LRestore;
    }
    silc_obj eval_arg = silc_eval(c, silc_car(c, val_it));
    if (silc_try_get_err_code(eval_arg) >= 0) {
      result = eval_arg;
      goto LRestore;
    }

    new_env = silc_cons(c, silc_cons(c, sym, eval_arg), new_env);

    /* go to next value */
    val_it = silc_cdr(c, val_it);
  }

  /* override environment */
  c->current_env = new_env; /* update environment */

  /* set associations */
  /* TODO: FIX it - this code does not work properly with argument overrides - e.g. (lambda (a) (lambda (a) 1)) */
  for (silc_obj it = new_env; it != SILC_OBJ_NIL; it = silc_cdr(c, it)) {
    silc_obj entry = silc_car(c, it);
    silc_obj arg_name = silc_car(c, entry);
    silc_obj arg_value = silc_cdr(c, entry);

    /* save previous value */
    silc_obj prev_arg_value = silc_get_sym_info(c, arg_name, NULL);
    saved_arg_value_pairs = silc_cons(c, silc_cons(c, arg_name, prev_arg_value), saved_arg_value_pairs);

    /* ...and associate it with a new one */
    silc_set_sym_assoc(c, arg_name, arg_value);
  }

  /* eval function body */
  result = silc_eval(c, fn_contents[2]);

LRestore:
  restore_args(c, saved_arg_value_pairs);
  c->current_env = prev_env; /* restore environment */

  return result;
}

static silc_obj eval_cons(struct silc_ctx_t* c, silc_obj cons) {
  /* Parse cons */
  silc_obj* cons_contents = silc_parse_cons(c->mem, cons);

  /* Get CAR and try evaluate it to function */
  SILC_CHECKED_DECLARE(fn, silc_eval(c, cons_contents[0]));
  if (SILC_GET_TYPE(fn) != SILC_TYPE_OREF) {
    return silc_err_from_code(SILC_ERR_NOT_A_FUNCTION);
  }

  int len = 0;
  silc_obj* fn_contents = NULL;
  int subtype = silc_int_mem_parse_ref(c->mem, fn, &len, NULL, &fn_contents);
  if (subtype != SILC_OREF_FUNCTION_SUBTYPE) {
    return silc_err_from_code(SILC_ERR_NOT_A_FUNCTION);
  }
  SILC_ASSERT(4 == len); /* see create_function */

  /* parse function */
  int fn_flags = silc_obj_to_int(fn_contents[0]);

  silc_obj result;
  if (fn_flags & SILC_FN_BUILTIN) {
    result = call_builtin(c, cons_contents, fn_contents, fn_flags & SILC_FN_SPECIAL);
  } else {
    result = call_lambda(c, cons_contents[1], fn_contents);
  }

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
