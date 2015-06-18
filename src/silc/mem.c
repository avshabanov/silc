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

#include "mem.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/** How big silc_obj array should be to fit byte_count bytes? */
static inline int silc_obj_count_from_byte_count(int byte_count) {
  SILC_ASSERT(byte_count >= 0);
  return ((byte_count + sizeof(silc_obj) - 1) / sizeof(silc_obj));
}

static silc_obj * get_contents_and_mark(struct silc_mem_t * mem, silc_obj obj) {
  int pos_index = silc_int_mem_get_pos_index(mem, obj);
  silc_obj pos_fval = mem->buf[pos_index];
  silc_obj * result;

  if (pos_fval & SILC_INT_MEM_POS_GC_BIT) {
    result = NULL; /* object has already been marked */
  } else {
    /* object has not been marked, mark it and return its contents */
    mem->buf[pos_index] = pos_fval | SILC_INT_MEM_POS_GC_BIT;
    result = mem->buf + (pos_fval >> SILC_INT_MEM_POS_SHIFT);
  }

  return result;
}

static void gc_mark(struct silc_mem_t * mem, silc_obj v) {
  silc_obj * t;

  switch (SILC_GET_TYPE(v)) {
    case SILC_TYPE_CONS:
      t = get_contents_and_mark(mem, v);
      if (t == NULL) {
        break;
      }

      gc_mark(mem, t[0]); /* cons.car */
      gc_mark(mem, t[1]); /* cons.cdr */
      break;

    case SILC_TYPE_OREF: /* contents: sequence of silc_obj */
      t = get_contents_and_mark(mem, v);
      if (t == NULL) {
        break;
      }

      { /* handle nested objects */
        int size = silc_obj_to_int(t[1]);
        for (int i = 0; i < size; ++i) {
          gc_mark(mem, t[i + 2]);
        }
      }
      break;

    case SILC_TYPE_BREF: /* contents: unknown, but no GC-able things, mark and return */
      get_contents_and_mark(mem, v);
      break;
  }
}

static void mark_root_objects(struct silc_mem_t* mem) {
  /* TODO: optimize by not looking into the entire object contents */
  gc_mark(mem, mem->root_vector);
}

static void init_heap(struct silc_mem_t* mem, struct silc_mem_init_t* init) {
  int init_memory_size = init->init_memory_size;
  if (init_memory_size < 4) {
    fputs(";; Init memory size is too small\n", stderr);
    abort();
  }

  mem->buf = init->alloc_mem(sizeof(silc_obj) * init_memory_size);
  mem->last_pos_index = init_memory_size - 1;
  mem->avail_index = 0;
  mem->pos_count = 0;
  mem->cached_last_occupied_pos_index = 0;
}

/**
 * Tries to allocate n consecutive blocks (silc_obj[n]) in the heap.
 * Total size of the allocated memory is <code>n * sizeof(silc_obj)</code> bytes.
 * Returns index of position that contains index of the available sequence or -1 if heap is full.
 *
 * @param mem Managed memory context
 * @param n Count of silc_obj units to be allocated
 * @param type Type information to be propagated to pos (duplicates type info embedded into silc_obj)
 * @return Index of position index or -1 if allocation failed
 */
static int try_alloc(struct silc_mem_t * mem, int n, int type) {
  int new_pos_index = mem->pos_count;
  int new_pos_count = mem->pos_count;

  /* try find vacant position */
  for (int i = mem->cached_last_occupied_pos_index; i < mem->pos_count; ++i) {
    int pos_idx = (int) mem->buf[mem->last_pos_index - i];
    if (pos_idx == SILC_INT_MEM_FREE_POS) {
      new_pos_index = i;
      goto LPosFound;
    }
  }

  /* no vacant position, addition of a new one is required */
  ++new_pos_count;

LPosFound:
  /* optimization: start looking up for the next vacant index without reiterating over occupied cells */
  mem->cached_last_occupied_pos_index = new_pos_index + 1;

  /* calculate new available index */
  int new_avail_index = mem->avail_index + n;
  int result = -1;

  /* allocation succeeds if heap is not empty */
  if (new_avail_index <= (mem->last_pos_index - new_pos_count)) {
    /* ok, the heap has enough space to place n blocks, update heap state */
    result = new_pos_index;

    /* record currently available index */
    mem->buf[mem->last_pos_index - new_pos_index] = (mem->avail_index << SILC_INT_MEM_POS_SHIFT) | type;

    /* update position indexes counter */
    mem->pos_count = new_pos_count;

    /* update next available index */
    mem->avail_index = new_avail_index;
  }

  return result;
}

static int alloc_or_fail(struct silc_mem_t * mem, int n, int type) {
  int result;
  int gc_attempted = 0;

LTryAlloc:
  result = try_alloc(mem, n, type);
  if (result < 0) {
    if (gc_attempted) {
      mem->init->oom_abort(mem->init);
    } else {
      silc_int_mem_gc(mem);
      gc_attempted = 1;
      goto LTryAlloc;
    }
  }

  return result;
}

static void update_positions(struct silc_mem_t* mem, int pos_index, int obj_size) {
  for (int j = pos_index; j < mem->pos_count; ++j) {
    silc_obj fpos = mem->buf[mem->last_pos_index - j];
    if (fpos == SILC_INT_MEM_FREE_POS) {
      continue; /* no need to update free position cell */
    }

    int type = fpos & SILC_INT_TYPE_MASK;
    int obj_pos = fpos >> SILC_INT_MEM_POS_SHIFT;

    /* update position */
    mem->buf[mem->last_pos_index - j] = ((obj_pos - obj_size) << SILC_INT_MEM_POS_SHIFT) | type;
  }
}

#define SILC_INT_MEM_INITIAL_ROOT_VECTOR_SIZE     (1000)

static silc_obj create_root_vector(struct silc_mem_t* mem, int size) {
  silc_obj root_vector = silc_int_mem_alloc(mem, size + 2, NULL, SILC_TYPE_OREF, SILC_OREF_ROOT_VECTOR_SUBTYPE);

  silc_obj* rv = silc_get_oref(mem, root_vector, NULL);

  rv[0] = silc_int_to_obj(size); /* total capacity */
  rv[1] = SILC_OBJ_ZERO; /* current size */

  return root_vector;
}

/* External functions */

void silc_int_mem_init(struct silc_mem_t* new_mem, struct silc_mem_init_t* init) {
  new_mem->init = init;
  init_heap(new_mem, init);

  /* Alloc root vector */
  if (init->init_root_vector_size <= 0) {
    init->init_root_vector_size = SILC_INT_MEM_INITIAL_ROOT_VECTOR_SIZE;
  }
  new_mem->root_vector = create_root_vector(new_mem, init->init_root_vector_size);
}

void silc_int_mem_free(struct silc_mem_t * mem) {
  mem->init->free_mem(mem->buf);
}

void silc_int_mem_add_root(struct silc_mem_t* mem, silc_obj o) {
  /* unfold root vector */
  silc_obj* rv = silc_get_oref(mem, mem->root_vector, NULL);
  int capacity = silc_obj_to_int(rv[0]);
  int size = silc_obj_to_int(rv[1]);
  silc_obj* arr = rv + 2;

  /* update size and check that it is within the desired capacity */
  int new_size = size + 1;
  if (new_size > capacity) {
    /* resize root vector */
    int new_capacity = capacity * 2;
    silc_obj new_root_vector = create_root_vector(mem, new_capacity);
    /* copy contents */
    silc_obj* new_rv = silc_get_oref(mem, new_root_vector, NULL);
    memcpy(new_rv, rv, sizeof(silc_obj) * new_capacity);
    rv[0] = silc_int_to_obj(new_capacity);

    /* update root vector pointer and retry operation */
    mem->root_vector = new_root_vector;
    return;
  }

  /* add an object and update size */
  arr[size] = o;
  rv[1] = silc_int_to_obj(new_size);
}

void silc_int_mem_set_auto_mark_roots(struct silc_mem_t* mem, struct silc_int_alloc_mode_t* prev_mode) {
  mem->auto_mark_enabled = true;

  /* unfold root vector */
  silc_obj* rv = silc_get_oref(mem, mem->root_vector, NULL);

  /* store previous size */
  prev_mode->prev_size = silc_obj_to_int(rv[1]);
}

void silc_int_mem_gc(struct silc_mem_t * mem) {
  mark_root_objects(mem);

  /* iterate over all the available objects and compactify memory */
  /* start from the last element and move to the first to minimize amount of moved memory */
  int can_shrink_pos_count = 1;
  int initial_pos = mem->pos_count - 1;
  for (int i = initial_pos; i >= 0; --i) {
    int index_pos = mem->last_pos_index - i;
    silc_obj pos_fval = mem->buf[index_pos];
    /* is this position is free, i.e. points to nothing? */
    if (pos_fval == SILC_INT_MEM_FREE_POS) {
      if (can_shrink_pos_count) {
        --mem->pos_count;
      }
      continue;
    }

    /* is this position belongs to the object marked as being referenced from the GC roots? */
    if (pos_fval & SILC_INT_MEM_POS_GC_BIT) {
      /* reset GC bit and continue */
      mem->buf[index_pos] &= ~SILC_INT_MEM_POS_GC_BIT;
      can_shrink_pos_count = 0; /* pos_count can no longer be shrinked */
      continue;
    }

    /* ok, this object is not referenced from GC roots and thus it is eligible for garbage collection */
    int fpos = mem->buf[index_pos];

    /* mark position entry as free */
    mem->buf[index_pos] = SILC_INT_MEM_FREE_POS;

    if (can_shrink_pos_count) {
      --mem->pos_count; /* shrink position if possible */
    }

    /* calculate object size and move memory */
    int obj_index = fpos >> SILC_INT_MEM_POS_SHIFT;
    silc_obj* obj_mem = mem->buf + obj_index;
    int obj_size = -1;
    switch (fpos & SILC_INT_TYPE_MASK) {
      case SILC_TYPE_CONS:
        obj_size = 2;
        break;

      case SILC_TYPE_OREF:
        obj_size = 2 + silc_obj_to_int(obj_mem[1]);
        break;

      case SILC_TYPE_BREF:
        obj_size = 2 + silc_obj_count_from_byte_count(silc_obj_to_int(obj_mem[1]));
        break;
    }

    /* paranoid check - this error shouldn't happen */
    if (obj_size < 0) {
      fputs(";; Internal error: unable to calculate object size (unrecognized object type)\n", stderr);
      abort();
      return;
    }

    /* move memory starting from the object end to available index and update positions */
    int mem_size = mem->avail_index - obj_index + obj_size;
    memmove(obj_mem, obj_mem + obj_size, mem_size * sizeof(silc_obj));
    update_positions(mem, i, obj_size);
    mem->avail_index -= obj_size;
  }
}

void silc_int_mem_calc_stats(struct silc_mem_t* mem, struct silc_mem_stats_t* stats) {
  stats->total_memory = mem->last_pos_index + 1;
  stats->pos_count = mem->pos_count;
  stats->usable_memory = stats->total_memory - mem->pos_count - mem->avail_index;

  /* calc free memory */
  int free_pos_count = 0;
  for (int i = 0; i < mem->pos_count; ++i) {
    if (mem->buf[mem->last_pos_index - i] == SILC_INT_MEM_FREE_POS) {
      ++free_pos_count;
    }
  }
  stats->free_memory = stats->usable_memory + free_pos_count;
  stats->free_pos_count = free_pos_count;
}

silc_obj silc_int_mem_alloc(struct silc_mem_t* mem, int content_length, const void* content, int type, int subtype) {
  int pos_index = -1;
  silc_obj* p_layout;

  switch (type) {
    case SILC_TYPE_CONS:
      SILC_ASSERT(content_length == 2 && subtype == SILC_INT_MEM_CONS_SUBTYPE);
      pos_index = alloc_or_fail(mem, 2, type);
      p_layout = mem->buf + (mem->buf[mem->last_pos_index - pos_index] >> SILC_INT_MEM_POS_SHIFT);
      if (content != NULL) {
        p_layout[0] = ((silc_obj *) content)[0]; /* car */
        p_layout[1] = ((silc_obj *) content)[1]; /* cdr */
      } else {
        p_layout[0] = SILC_OBJ_NIL;
        p_layout[1] = SILC_OBJ_NIL;
      }
      break;

    case SILC_TYPE_OREF:
      SILC_ASSERT(content_length >= 0);
      pos_index = alloc_or_fail(mem, 2 + content_length, type);
      p_layout = mem->buf + (mem->buf[mem->last_pos_index - pos_index] >> SILC_INT_MEM_POS_SHIFT);
      *p_layout++ = silc_int_to_obj(subtype);
      *p_layout++ = silc_int_to_obj(content_length);
      if (content_length > 0) {
        if (content != NULL) {
          memcpy(p_layout, content, content_length * sizeof(silc_obj));
        } else {
          memset(p_layout, 0, content_length * sizeof(silc_obj)); /* initializes contents with NILs */
        }
      }
      break;

    case SILC_TYPE_BREF:
      SILC_ASSERT(content_length >= 0);
      pos_index = alloc_or_fail(mem, 2 + silc_obj_count_from_byte_count(content_length), type);
      p_layout = mem->buf + (mem->buf[mem->last_pos_index - pos_index] >> SILC_INT_MEM_POS_SHIFT);
      *p_layout++ = silc_int_to_obj(subtype);
      *p_layout++ = silc_int_to_obj(content_length);
      if (content_length > 0) {
        if (content != NULL) {
          memcpy(p_layout, content, content_length);
        } else {
          memset(p_layout, 0, content_length); /* initializes contents with zeroes */
        }
      }
      break;

    default:
      SILC_ASSERT(!"Unknown object type");
  }

  return pos_index < 0 ? SILC_OBJ_NIL : ((((silc_obj) pos_index) << SILC_INT_TYPE_SHIFT) | type);
}
