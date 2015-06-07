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
#include "mem.h"
#include "core.h"

static void print_unknown(silc_obj o, FILE* out) {
  fprintf(out, "<Unknown#0x%X>", o);
}

static void print_inl(silc_obj o, FILE* out) {
  switch (SILC_GET_INL_SUBTYPE(o)) {
    case SILC_OBJ_INL_SUBTYPE_NIL:
      fputs("nil", out);
      break;

    /* Boolean type */
    case SILC_OBJ_INL_SUBTYPE_BOOL:
      switch (o) {
        case SILC_OBJ_TRUE:
          fputs("true", out);
          break;
        case SILC_OBJ_FALSE:
          fputs("false", out);
          break;
        default:
          print_unknown(o, out);
          break;
      }
      break;

    case SILC_OBJ_INL_SUBTYPE_INT:
      {
        int val = silc_obj_to_int(o);
        fprintf(out, "%d", val);
      }
      break;
  }
}

static void print_str_contents(struct silc_ctx_t* c, silc_obj str, FILE* out) {
  char buf[32];
  for (int pos = 0;;) {
    int read = silc_get_str_chars(c, str, buf, pos, sizeof(buf));
    fwrite(buf, 1, read, out);
    if (read < sizeof(buf)) {
      break;
    }
    pos += read;
  }
}

static void print_cons(struct silc_ctx_t* c, silc_obj o, FILE* out) {
  fputc('(', out);

  for (silc_obj cdr = o;;) {
    silc_print(c, silc_car(c, cdr), out);
    cdr = silc_cdr(c, cdr);

    if (cdr == SILC_OBJ_NIL) {
      break;
    }

    if (SILC_GET_TYPE(cdr) == SILC_OBJ_CONS_TYPE) {
      fputc(' ', out);
      continue;
    }

    fputs(" . ", out);
    silc_print(c, cdr, out);
    break;
  }

  fputc(')', out);
}

static void print_oref(struct silc_ctx_t* c, silc_obj o, FILE* out) {
  switch (silc_get_ref_subtype(c, o)) {
    case SILC_OREF_SYMBOL_SUBTYPE:
      {
        silc_obj str = SILC_OBJ_NIL;
        silc_get_sym_info(c, o, &str);
        print_str_contents(c, str, out);
      }
      break;

    default:
      {
        int len = 0;
        silc_obj* content = NULL;
        int subtype = silc_parse_ref(c->mem, o, &len, NULL, &content);
        fprintf(out, "<?oref{subtype=%d, len=%d, content=", subtype, len);
        for (int i = 0; i < len; ++i) {
          fprintf(out, " 0x%X", content[i]);
        }
        fputs("}>", out);
      }
  }
}

static void print_bref(struct silc_ctx_t* c, silc_obj o, FILE* out) {
  switch (silc_get_ref_subtype(c, o)) {
    case SILC_BREF_STR_SUBTYPE:
      {
        fputc('"', out);
        print_str_contents(c, o, out);
        fputc('"', out);
      }
      break;

    default:
      {
        int len = 0;
        char* content = NULL;
        int subtype = silc_parse_ref(c->mem, o, &len, &content, NULL);
        fprintf(out, "<?bref{subtype=%d, len=%d, content=", subtype, len);
        for (int i = 0; i < len; ++i) {
          fprintf(out, " 0x%02X", content[i]);
        }
        fputs("}>", out);
      }
  }
}

void silc_print(struct silc_ctx_t* c, silc_obj o, FILE* out) {
  switch (o & SILC_OBJ_TYPE_MASK) {
    /* Inline type */
    case SILC_OBJ_INL_TYPE:
      print_inl(o, out);
      break;

    case SILC_OBJ_CONS_TYPE:
      print_cons(c, o, out);
      break;

    case SILC_OBJ_OREF_TYPE:
      print_oref(c, o, out);
      break;

    case SILC_OBJ_BREF_TYPE:
      print_bref(c, o, out);
      break;

    default:
      print_unknown(o, out);
  }
}
