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

#include "core.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mem.h"
#include "builtins.h"

/* Forward declarations */

static silc_obj silc_hash_table(struct silc_ctx_t* c, int initial_size);

/* Mem alloc functions */

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

/* Memory management-related */

static void* root_obj_iter_start(struct silc_mem_init_t* mem_init) {
  return (void *)1;
}

static void* root_obj_iter_next(struct silc_mem_init_t* mem_init,
                                void* iter_context,
                                silc_obj** objs, size_t* size) {
  void* result = NULL;
  struct silc_ctx_t* c = mem_init->context;

  if (iter_context == ((void *)1)) {
    *objs = c->stack;
    *size = c->stack_last;
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
  silc_internal_init_mem(mem, init);

  c->mem_init = init;
  c->mem = mem;
}

static void init_globals(struct silc_ctx_t* c) {
  c->sym_hash_table = silc_hash_table(c, 8179/*prime*/);
  c->root_cons = silc_cons(c, c->sym_hash_table, SILC_OBJ_NIL);
}

static void init_builtins(struct silc_ctx_t* c) {
  /**/
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
  silc_internal_free_mem(c->mem);
  xfree(c->mem);
  xfree(c->settings);
  xfree(c->stack);

  xfree(c);
}

/*
 * Helper functions
 */

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
      subtype = silc_parse_ref(c->mem, lhs, &lhs_len, &lhs_pb, &lhs_po);
      if (silc_parse_ref(c->mem, rhs, &rhs_len, &rhs_pb, &rhs_po) != subtype) {
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
      subtype = silc_parse_ref(c->mem, lhs, &lhs_len, &lhs_pb, &lhs_po);
      if (silc_parse_ref(c->mem, rhs, &rhs_len, &rhs_pb, &rhs_po) != subtype) {
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
  return silc_parse_ref(c->mem, o, NULL, NULL, NULL);
}

/*
 * Hash table
 */

static silc_obj silc_hash_table(struct silc_ctx_t* c, int initial_size) {
  silc_obj result = silc_alloc_obj(c->mem, initial_size + 1, NULL, SILC_TYPE_OREF, SILC_OREF_HASHTABLE_SUBTYPE);
  silc_get_oref_contents(c->mem, result)[0] = silc_int_to_obj(0); /* element count */
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
  int subtype = silc_parse_ref(c->mem, o, &len, &byte_contents, &obj_contents);
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
  int subtype = silc_parse_ref(c->mem, str, &len, &char_content, NULL);
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
  int hash_table_subtype = silc_parse_ref(c->mem, c->sym_hash_table, &hash_table_size, NULL, &hash_table_contents);

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
  silc_obj result = silc_alloc_obj(c->mem, 3, contents, SILC_TYPE_OREF, SILC_OREF_SYMBOL_SUBTYPE);

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
  int subtype = silc_parse_ref(c->mem, o, &len, NULL, &obj_contents);
  if (subtype != SILC_OREF_SYMBOL_SUBTYPE) {
    return SILC_ERR_INVALID_ARGS;
  }

  assert(len == 3 && obj_contents != NULL);
  silc_obj old_assoc = obj_contents[2];
  obj_contents[2] = new_assoc;
  return old_assoc;
}

silc_obj silc_str(struct silc_ctx_t* c, const char* buf, int size) {
  return silc_alloc_obj(c->mem, size, buf, SILC_TYPE_BREF, SILC_BREF_STR_SUBTYPE);
}

int silc_get_str_chars(struct silc_ctx_t* c, silc_obj o, char* buf, int pos, int size) {
  int len = 0;
  char* char_content = NULL;
  silc_obj* obj_content = NULL;
  int result = 0;
  if (silc_parse_ref(c->mem, o, &len, &char_content, &obj_content) == SILC_BREF_STR_SUBTYPE) {
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
  return silc_alloc_obj(c->mem, 2, contents, SILC_TYPE_CONS, SILC_CONS_SUBTYPE);
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
