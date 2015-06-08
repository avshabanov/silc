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

#include "mem.h"

static inline void dbg_silc_dump_heap_pos(struct silc_mem_t* mem) {
  /* heap state dump */
  for (int i = 0; i < mem->pos_count; ++i) {
    silc_obj fpos = mem->buf[mem->last_pos_index - i];
    fprintf(stderr, "\t[DBG] heap_pos[%d]=0x%08X (index=%d, gc_mark=%s, type=%d)\n",
      i,
      fpos,
      (int) (fpos >> SILC_INT_MEM_POS_SHIFT),
      ((fpos & SILC_INT_MEM_POS_GC_BIT) ? "yes" : "no"),
      fpos & SILC_INT_TYPE_MASK);
  }
}

static inline void dbg_silc_dump_gc_stats(struct silc_mem_t* mem, FILE* out) {
  struct silc_mem_stats_t stats = {0};
  silc_int_mem_calc_stats(mem, &stats);

  fprintf(out,
    ";; ====================================\n"
    ";; GC statistics:\n"
    ";;   Total Memory:     %8d unit(s)\n"
    ";;   Free Memory:      %8d unit(s)\n"
    ";;   Usable Memory:    %8d unit(s)\n"
    ";;   Pos Count:        %8d unit(s)\n"
    ";;   Free Pos Count:   %8d unit(s)\n"
    ";;\n",
    stats.total_memory, stats.free_memory, stats.usable_memory, stats.pos_count, stats.free_pos_count);

  /* heap dump */
  fputs(";; [DBG] Heap:\n", out);
  for (int i = 0; i < mem->pos_count; ++i) {
    silc_obj fpos = mem->buf[mem->last_pos_index - i];
    if (fpos == SILC_INT_MEM_FREE_POS) {
      fprintf(out, ";; [DBG] heap_pos[%d]=FREE\n", i);
      continue;
    }
    int obj_pos = (int) (fpos >> SILC_INT_MEM_POS_SHIFT);
    int type = SILC_GET_TYPE(fpos);
    fprintf(out, ";; [DBG] heap_pos[%d]=0x%08X, obj_index=%d, gc_mark=%s, type=%d\n",
      i,
      fpos,
      obj_pos,
      ((fpos & SILC_INT_MEM_POS_GC_BIT) ? "yes" : "no"),
      type);

    fputs(";; [DBG] object: ", out);
    silc_obj* o = mem->buf + obj_pos;
    int len;
    switch (type) {
    case SILC_TYPE_CONS:
      fprintf(out, "%X %X", o[0], o[1]);
      break;

    case SILC_TYPE_OREF:
      len = silc_obj_to_int(o[1]);
      fprintf(out, "oref subtype=%d len=%d |", silc_obj_to_int(o[0]), len);
      for (int j = 0; j < len; ++j) {
        fprintf(out, " %X", o[2 + j]);
      }
      break;

    case SILC_TYPE_BREF:
      len = silc_obj_to_int(o[1]);
      fprintf(out, "bref subtype=%d len=%d |", silc_obj_to_int(o[0]), len);
      for (int j = 0; j < len; ++j) {
        fprintf(out, " %02X", ((char *)(o + 2))[j]);
      }
      fputs(" | ", out);
      for (int j = 0; j < len; ++j) {
        char c = ((char *)(o + 2))[j];
        fprintf(out, "%c", ((c >= 32 && c < 127) ? c : '.'));
      }
      break;
    }
    fputc('\n', out);
  }
}