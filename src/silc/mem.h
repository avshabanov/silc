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

#include <stdbool.h>

struct silc_mem_init_t;

typedef void (* silc_internal_oom_abort_pfn)(struct silc_mem_init_t* mem_init);

struct silc_mem_init_t {
  void*                   context; /* custom context, for callback purposes */

  int                     init_memory_size; /* initial memory size */
  int                     max_memory_size; /* maximum memory size */

  int                     init_root_vector_size; /* initial size of the root vector */

  /* function, that should be called on OOM and gracefully abort execution */
  silc_internal_oom_abort_pfn               oom_abort;

  /* custom mem alloc functions, malloc should never return null */
  void* (* alloc_mem)(size_t size);
  void (* free_mem)(void* p);
};

struct silc_mem_t {
  struct silc_mem_init_t* init;
  
  /**
   * Heap memory.
   * Layout: is as follows [{object1}{object2}{objectN}...free space...{posN}{pos2}{pos1}]
   */
  silc_obj*                 buf;

  /**
   * index of the last element in this buffer, matches first moveable reference index
   * Total size of this buffer in bytes == (last_pos_index + 1)*sizeof(silc_obj)
   */
  int                       last_pos_index;

  /** available index for object contents */
  int                       avail_index;

  /**
   * count positions (moveable references)
   */
  int                       pos_count;

  /**
   * Used for optimization purposes only.
   * This index allows to skip positions that are not vacant and every search updates this index with
   * the next unoccupied index, so that the next allocation will be the fastest possible.
   */
  int                       cached_last_occupied_pos_index;

  /*
   * Root objects marker. This object serves as a marker for object roots.
   */
  silc_obj                  root_vector;

  /** Indicates whether or not auto mark enabled (disabled by default) */
  bool                      auto_mark_enabled;
};

struct silc_mem_stats_t {
  /**
   * Total allocated memory, here and below memory measured in silc_obj,
   * i.e. in order to get size in bytes this number should be multiplied to sizeof(silc_obj):
   * <code>stats.total_memory * sizeof(silc_obj)</code>
   */
  int                       total_memory;

  /** Available memory, that can be used to store useful content (excluding service GC information) */
  int                       usable_memory;

  /** Total available memory (includes usable_memory and spare service information) */
  int                       free_memory;

  /** Count of positions (both vacant and occupied) */
  int                       pos_count;

  /** Count of free positions */
  int                       free_pos_count;
};

void silc_int_mem_init(struct silc_mem_t* new_mem, struct silc_mem_init_t* init);

void silc_int_mem_free(struct silc_mem_t* mem);

/** Triggers garbage collection */
void silc_int_mem_gc(struct silc_mem_t* mem);

void silc_int_mem_calc_stats(struct silc_mem_t* mem, struct silc_mem_stats_t* stats);

/** Special subtype code for cons pointer */
#define SILC_INT_MEM_CONS_SUBTYPE      (0)

/**
 * Allocates n silc_obj units in the context memory and initializes it if content pointer is not null.
 *
 * @param mem             Pointer to memory context
 * @param content_length  Allocated object content length; should always be 2 for CONS objects
 * @param content         Pointer to silc_obj for CONS and OREF, pointer to char for BREF objects
 * @param type            Object type, can be SILC_TYPE_OREF, SILC_TYPE_CONS and SILC_TYPE_BREF
 * @param subtype         Object subtype. Should always be SILC_INT_MEM_CONS_SUBTYPE for CONS objects
 */
silc_obj silc_int_mem_alloc(struct silc_mem_t* mem, int content_length, const void* content, int type, int subtype);

/**
 * Marks an object as a root
 */
void silc_int_mem_add_root(struct silc_mem_t* mem, silc_obj o);

struct silc_int_alloc_mode_t {
  bool auto_mark_enabled;
  int prev_size;
};

/**
 * Enables auto mark and saves previous state of root vector and auto_mark_enabled to the provided mode variable.
 * This tricky function is needed for evaluation: in order to simplify code and not mark every intermediate object
 * as being referenced from certain root, every new object will be automatically marked as root whenever evaluation
 * occurs. Once evaluation is done, the automarked objects are wiped out from the root vector and garbage collector
 * will be able to collect them (unless they are referenced from somewhere else which is perfectly valid).
 */
void silc_int_mem_set_auto_mark_roots(struct silc_mem_t* mem, struct silc_int_alloc_mode_t* prev_mode);

/**
 * Restores heap state, modified in {@code silc_int_mem_set_auto_mark_roots}.
 */
void silc_int_mem_restore_roots(struct silc_mem_t* mem, struct silc_int_alloc_mode_t* prev_mode);

/* General purpose memory allocators */

/* Position layout: [...index...{gc_bit}{type_bits}] */
#define SILC_INT_MEM_POS_GC_BIT           (1 << SILC_INT_TYPE_SHIFT)
#define SILC_INT_MEM_POS_SHIFT            (SILC_INT_TYPE_SHIFT + 1)
#define SILC_INT_MEM_FREE_POS             (-1)

static inline int silc_int_mem_get_pos_index(struct silc_mem_t* mem, silc_obj obj) {
  int index_offset = (int) (obj >> SILC_INT_TYPE_SHIFT);
  SILC_ASSERT(index_offset >= 0 && index_offset < mem->pos_count);
  return mem->last_pos_index - index_offset;
}

static inline silc_obj* silc_int_mem_get_contents(struct silc_mem_t* mem, silc_obj obj) {
  int pos = (int) (mem->buf[silc_int_mem_get_pos_index(mem, obj)] >> SILC_INT_MEM_POS_SHIFT);
  SILC_ASSERT(pos >= 0 && pos < mem->avail_index);
  return mem->buf + pos;
}

static inline silc_obj* silc_int_mem_get_ref(struct silc_mem_t* mem, silc_obj obj, int* subtype, int* len) {
  silc_obj* contents = silc_int_mem_get_contents(mem, obj);

  if (subtype != NULL) {
    *subtype = silc_obj_to_int(contents[0]);
    SILC_ASSERT(*subtype >= 0);
  }

  if (len != NULL) {
    *len = silc_obj_to_int(contents[1]);
  }

  return contents;
}

/**
 * Parses reference and returns its subtype or -1 if an object is not a reference.
 * Notes:
 * <ul>
 *  <li>{@code content_len} is always set if type is reference</li>
 *  <li>{@code char_content} set only if bref, otherwise null</li>
 *  <li>{@code obj_content} set only if oref, otherwise null</li>
 * </ul>
 */
static inline int silc_int_mem_parse_ref(struct silc_mem_t* mem, silc_obj obj, int* content_len,
                                         char** char_content, silc_obj** obj_content) {
  int subtype = -1;
  char* pch = NULL;
  silc_obj* po = NULL;

  switch (SILC_GET_TYPE(obj)) {
    case SILC_TYPE_CONS:
      po = silc_int_mem_get_contents(mem, obj);
      *content_len = 2;
      subtype = SILC_INT_MEM_CONS_SUBTYPE;
      break;

    case SILC_TYPE_OREF:
      po = silc_int_mem_get_ref(mem, obj, &subtype, content_len) + 2;
      break;

    case SILC_TYPE_BREF:
      pch = (char*) (silc_int_mem_get_ref(mem, obj, &subtype, content_len) + 2);
      break;

    default:
      SILC_ASSERT(!"unknown reference type");
  }

  if (char_content != NULL) {
    *char_content = pch;
  }

  if (obj_content != NULL) {
    *obj_content = po;
  }

  return subtype;
}

static inline silc_obj* silc_parse_cons(struct silc_mem_t* mem, silc_obj obj) {
  int content_length = 0;
  silc_obj* result = NULL;
  int subtype = silc_int_mem_parse_ref(mem, obj, &content_length, NULL, &result);
  SILC_ASSERT(result != NULL && content_length == 2 && subtype == SILC_INT_MEM_CONS_SUBTYPE);
  return result;
}

static inline silc_obj* silc_get_oref(struct silc_mem_t* mem, silc_obj obj, int* content_length) {
  silc_obj* result = NULL;
  int subtype = silc_int_mem_parse_ref(mem, obj, content_length, NULL, &result);
  SILC_ASSERT(result != NULL && subtype >= 0);
  return result;
}
