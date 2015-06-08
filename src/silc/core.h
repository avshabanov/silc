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

  /* Builtin functions */
  silc_fn_ptr *         fn_array;
  int                   fn_count;
};

struct silc_settings_t {
  FILE *    out;        /* default output stream (used in print function) */
};

#ifndef countof
#define countof(arr)    (sizeof(arr) / sizeof(arr[0]))
#endif
