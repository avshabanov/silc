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

#pragma once

#include "silc.h"

/* Struct declarations */
struct silc_mem_init_t;
struct silc_mem_t;

/* Functions support */

struct silc_fn_t;

typedef silc_obj (* silc_pfn)(struct silc_fn_t* ft);

struct silc_fn_t {
  struct silc_ctx_t *   ctx; /* back reference to the owning context */
  silc_pfn              pfn;
  int                   flags;
};

#define SILC_FN_SPEC_FORM     (1<<1)

struct silc_ctx_t {
  struct silc_settings_t* settings;

  /* stack */
  silc_obj*             stack;
  size_t                stack_frame_start;
  size_t                stack_last; /* function arguments count == stack_last-stack_frame_start */
  size_t                stack_size; /* total stack size */

  /* heap */
  struct silc_mem_init_t* mem_init;
  struct silc_mem_t*      mem;

  /* root object for service data */
  silc_obj              root_cons;
  silc_obj              sym_hash_table;
};

struct silc_settings_t {
  FILE *    out;        /* default output stream (used in print function) */
};

/* Internal helpers */

static inline silc_obj arg_peek(struct silc_ctx_t * c, size_t offset) {
  size_t pos = c->stack_last - offset;
  if (pos >= c->stack_size || pos < c->stack_frame_start) {
    assert(!"requested argument is out of stack frame");
    return silc_err_from_code(SILC_ERR_STACK_ACCESS);
  }

  return c->stack[pos];
}

static inline silc_obj arg_push(struct silc_ctx_t * c, silc_obj o) {
  if (c->stack_last >= c->stack_size) {
    assert(!"provided argument is out of stack");
    return silc_err_from_code(SILC_ERR_STACK_ACCESS);
  }

  c->stack[++c->stack_last] = o;
  return o;
}

static inline size_t arg_count(struct silc_ctx_t * c) {
  return c->stack_last - c->stack_frame_start;
}

static inline size_t str_hash_code_len(const char * buf, size_t buf_size) {
  size_t result = 0;
  size_t i;
  for (i = 0; i < buf_size; ++i) {
    result += 31 * ((size_t) buf[i]);
  }
  return result;
}
