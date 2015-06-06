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

#include <stdbool.h>
#include <string.h>

/* Forward declarations */

static silc_obj read_number_or_symbol(struct silc_ctx_t * c, FILE * f);
static silc_obj read_symbol_or_special(struct silc_ctx_t * c, FILE * f);
static silc_obj read_list(struct silc_ctx_t * c, FILE * f);

/* Implementation */

static inline bool is_lisp_char(int c) {
  if ((c >= 'a' && c < 'z') || (c >= 'A' && c < 'Z') || (c >= '0' && c < '9') ||
      (c == '*') || (c == '&') || (c == '-') || (c == '+') || (c == '*')) {
    return true;
  }
  return false;
}

static inline bool is_digit(int c) {
  return c >= '0' && c <= '9';
}

static inline bool is_whitespace(int c) {
  return c > 0 && c <= ' ';
}

/** Returns next non-whitespace character */
static int get_nwc(FILE * f) {
  int c;

  do {
    c = fgetc(f);
    if (c == ';') {
      c = fgetc(f);
      if (c == ';') {
        /* skip to the newline or end of file */
        do {
          c = fgetc(f);
        } while (c != '\n' && c != '\r' && c != EOF);
        continue;
      }
      ungetc(c, f);
      break;
    }
  } while (is_whitespace(c));

  return c;
}

static silc_obj read_list(struct silc_ctx_t * c, FILE * f) {
  int ch = get_nwc(f);
  if (ch == ')') {
    return SILC_OBJ_NIL;
  }

  ungetc(ch, f);
  silc_obj car = silc_read(c, f);
  if (silc_try_get_err_code(car) > 0) {
    return car; /* propagate error */
  }

  /* take first element and recursively read the rest of this list */
  return silc_cons(c, car, read_list(c, f));
}

static silc_obj read_number_or_symbol(struct silc_ctx_t * c, FILE * f) {
  int ch = fgetc(f);
  int sign;
  int absval;
  bool first_char_met;

  /* first character */
  if (ch == '-') {
    sign = -1;
    absval = 0;
    first_char_met = false;
  } else {
    sign = 1;
    absval = (ch - '0');
    first_char_met = true;
  }

  /* next characters */
  for (;; first_char_met = true) {
    ch = fgetc(f);

    if (is_digit(ch)) {
      absval = 10 * absval + (ch - '0');
      continue;
    }

    if (is_whitespace(ch) || ch == ')' || ch == EOF) {
      ungetc(ch, f);
      if (!first_char_met) {
        /* special case (minus sign) */
        char minus[] = { '-' };
        return silc_sym_from_buf(c, minus, 1);
      }
      break; /* end of number */
    }

    /* unknown character */
    return silc_err_from_code(SILC_ERR_UNEXPECTED_CHARACTER);
  }

  /* end reading */
  return silc_int_to_obj(sign < 0 ? -absval : absval);
}

static silc_obj read_symbol_or_special(struct silc_ctx_t * c, FILE * f) {
  char buf[256];
  int len = 1;
  buf[0] = fgetc(f);

  for (;;) {
    char ch = fgetc(f);
    if (is_lisp_char(ch)) {
      if (len >= sizeof(buf)) {
        return silc_err_from_code(SILC_ERR_SYMBOL_TOO_BIG);
      }

      buf[len++] = ch;
      continue;
    }

    if (is_whitespace(ch) || ch == EOF) {
      break; /* safely skip */
    }

    if (ch == ')') {
      ungetc(ch, f);
      break;
    }

    return silc_err_from_code(SILC_ERR_UNEXPECTED_CHARACTER);
  }
  
  /* special symbols? */
  if (len == 3 && 0 == memcmp("nil", buf, 3)) {
    return SILC_OBJ_NIL;
  }

  if (len == 4 && 0 == memcmp("true", buf, 4)) {
    return SILC_OBJ_TRUE;
  }

  if (len == 5 && 0 == memcmp("false", buf, 5)) {
    return SILC_OBJ_FALSE;
  }

  return silc_sym_from_buf(c, buf, len);
}

silc_obj silc_read(struct silc_ctx_t * c, FILE * f) {
  int ch = get_nwc(f);
  if (ch == EOF) {
    return silc_err_from_code(SILC_ERR_UNEXPECTED_EOF);
  }

  if (ch == '(') {
    return read_list(c, f);
  }

  if (ch == '-' || is_digit(ch)) {
    ungetc(ch, f);
    return read_number_or_symbol(c, f);
  }

  if (is_lisp_char(ch)) {
    ungetc(ch, f);
    return read_symbol_or_special(c, f);
  }

  return silc_err_from_code(SILC_ERR_UNEXPECTED_CHARACTER);
}
