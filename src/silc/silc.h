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

#include <stdio.h>
#include <limits.h>
#include <assert.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Base declarations */

typedef unsigned int silc_obj;
struct silc_ctx_t;

/**
 * Counts of bits, used to represent object type information.
 * See also SILC_OBJ_INL_CONTENT_BITS
 *
 * Primary types are:
 * <ul>
 *  <li>SILC_OBJ_INL_TYPE</li>
 *  <li>SILC_OBJ_CONS_TYPE</li>
 *  <li>SILC_OBJ_OREF_TYPE</li>
 *  <li>SILC_OBJ_BREF_TYPE</li>
 * </ul>
 */
#define SILC_OBJ_TYPE_SHIFT           (3)
#define SILC_OBJ_TYPE_MASK            ((1 << SILC_OBJ_TYPE_SHIFT) - 1)

/**
 * Returns one of the type code associated with an object:
 * <ul>
 *  <li>SILC_OBJ_INL_TYPE</li>
 *  <li>SILC_OBJ_CONS_TYPE</li>
 *  <li>SILC_OBJ_OREF_TYPE</li>
 *  <li>SILC_OBJ_BREF_TYPE</li>
 * </ul>
 */
#define SILC_GET_TYPE(o)              ((o) & SILC_OBJ_TYPE_MASK)

/**
 * Count of bits per object content excluding type bits.
 * General object layout assumed to be as follows: [{..generic content..}{type bits}]
 */
#define SILC_OBJ_CONTENT_BITS         ((sizeof(silc_obj) * CHAR_BIT) - SILC_OBJ_TYPE_SHIFT)


/**
 * Simple objects written as is into the lval.
 * Subtypes: boolean, small signed int
 */
#define SILC_OBJ_INL_TYPE             (0)

#define SILC_OBJ_INL_SUBTYPE_MASK     (3)
#define SILC_OBJ_INL_SUBTYPE_SHIFT    (2)

/**
 * Returns inline object subtype.
 * Before calling this macro, the caller should make sure that object is of inline subtype.
 *
 * This macro returns one of the inline subtypes:
 * <ul>
 *  <li>SILC_OBJ_INL_SUBTYPE_NIL</li>
 *  <li>SILC_OBJ_INL_SUBTYPE_BOOL</li>
 *  <li>SILC_OBJ_INL_SUBTYPE_INT</li>
 *  <li>SILC_OBJ_INL_SUBTYPE_ERR</li>
 * </ul>
 */
#define SILC_GET_INL_SUBTYPE(inl_o)   (((inl_o) >> SILC_OBJ_TYPE_SHIFT) & SILC_OBJ_INL_SUBTYPE_MASK)

/**
 * Total count of bits in the inline object content.
 * Inline object layout: [{..inline object content..}{subtype bits}{type bits}]
 */
#define SILC_OBJ_INL_CONTENT_BITS     (SILC_OBJ_CONTENT_BITS - SILC_OBJ_INL_SUBTYPE_SHIFT)

#define SILC_OBJ_INL_SUBTYPE_NIL      (0)
#define SILC_OBJ_INL_SUBTYPE_BOOL     (1)
#define SILC_OBJ_INL_SUBTYPE_INT      (2)
/* Inline error */
#define SILC_OBJ_INL_SUBTYPE_ERR      (3)

/* Error codes */

/* Uncategorized error (internal error) */
#define SILC_ERR_INTERNAL             (500)

/* Stack access error (i.e. argument requested is out of stack frame) */
#define SILC_ERR_STACK_ACCESS         (501)

/* User called function */
#define SILC_ERR_INVALID_ARGS         (400)

/* Value is out of allowable bounds (overflow or underflow) */
#define SILC_ERR_VALUE_OUT_OF_RANGE   (401)

/* Reader errors */

/* Unexpected end of file while reading file */
#define SILC_ERR_UNEXPECTED_EOF       (450)
#define SILC_ERR_UNEXPECTED_CHARACTER (451)
/* Symbol representation is too big */
#define SILC_ERR_SYMBOL_TOO_BIG       (452)

/* Evaluation errors */

/* Symbol has not been associated with a value */
#define SILC_ERR_UNRESOLVED_SYMBOL    (470)

/**
 * Converts error code to string.
 */
static inline const char* silc_err_code_to_str(int code) {
  assert(code > 0);
  switch (code) {
    case SILC_ERR_INTERNAL:
      return "internal error";

    case SILC_ERR_STACK_ACCESS:
      return "stack access error";

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

    default:
      return "unknown error";
  }
}


#define SILC_MAX_ERR_CODE             ((1<<(SILC_OBJ_INL_CONTENT_BITS)) - 1)

static inline silc_obj silc_err_from_code(int err_code) {
  assert(err_code > 0 && err_code < SILC_MAX_ERR_CODE);
  return (SILC_OBJ_INL_TYPE | (SILC_OBJ_INL_SUBTYPE_ERR << SILC_OBJ_TYPE_SHIFT) |
    (err_code << (SILC_OBJ_TYPE_SHIFT + SILC_OBJ_INL_SUBTYPE_SHIFT)));
}

/**
 * Tries to get an error code from a given object.
 * Returns negative value if this object does not represent an error.
 */
static inline int silc_try_get_err_code(silc_obj obj) {
  if ((SILC_GET_TYPE(obj) != SILC_OBJ_INL_TYPE) || (SILC_GET_INL_SUBTYPE(obj) != SILC_OBJ_INL_SUBTYPE_ERR)) {
    return -1;
  }

  return obj >> (SILC_OBJ_TYPE_SHIFT + SILC_OBJ_INL_SUBTYPE_SHIFT);
}

/**
 * Helper macro, that assigns the result of expression to the result variable.
 * Returns result immediately if expression result is an error.
 */
#define SILC_CHECKED_SET(result, expr) \
  result = (expr); \
  if (silc_try_get_err_code(result) > 0) { \
    return result; \
  }

/**
 * Represents cons cell.
 */
#define SILC_OBJ_CONS_TYPE            (1)

/**
 * Points to the sequence of objects (first element is a length (smallint), next one - type (smallint))
 */
#define SILC_OBJ_OREF_TYPE            (2)

#define SILC_OREF_SYMBOL_SUBTYPE      (10)
#define SILC_OREF_HASHTABLE_SUBTYPE   (20)

/**
 * Contains length, then sequence of bytes.
 */
#define SILC_OBJ_BREF_TYPE            (3)

#define SILC_BREF_STR_SUBTYPE         (1000)

/**
 * Represents variadic length reference type
 */

/* Base constants */

#define SILC_OBJ_FALSE_VAL            (0)
#define SILC_OBJ_TRUE_VAL             (1)

#define SILC_OBJ_NIL                  (SILC_OBJ_INL_TYPE | \
                                      (SILC_OBJ_INL_SUBTYPE_NIL << SILC_OBJ_TYPE_SHIFT))


/* Inline false value */
#define SILC_OBJ_FALSE                (SILC_OBJ_INL_TYPE | \
                                      (SILC_OBJ_INL_SUBTYPE_BOOL << SILC_OBJ_TYPE_SHIFT) | \
                                      (SILC_OBJ_FALSE_VAL << (SILC_OBJ_TYPE_SHIFT + SILC_OBJ_INL_SUBTYPE_SHIFT)))

/* Inline true value */
#define SILC_OBJ_TRUE                 (SILC_OBJ_INL_TYPE | \
                                      (SILC_OBJ_INL_SUBTYPE_BOOL << SILC_OBJ_TYPE_SHIFT) | \
                                      (SILC_OBJ_TRUE_VAL << (SILC_OBJ_TYPE_SHIFT + SILC_OBJ_INL_SUBTYPE_SHIFT)))


/* Zero integer */
#define SILC_OBJ_ZERO                 (SILC_OBJ_INL_TYPE | \
                                      (SILC_OBJ_INL_SUBTYPE_INT << SILC_OBJ_TYPE_SHIFT))



/* Service functions */

struct silc_ctx_t * silc_new_context();
void silc_free_context(struct silc_ctx_t * c);


/* Helper functions */

/** Bit that designates sign in the inline int object (relative to object content) */
#define SILC_INT_SIGN_BIT             (1<<(SILC_OBJ_INL_CONTENT_BITS - 1))
#define SILC_MAX_INT                  (SILC_INT_SIGN_BIT - 1)

/** Sign bit (must be the leftmost bit in an integer) */
#define SILC_INT_OBJ_SIGN_BIT         (1 << ((sizeof(silc_obj) * CHAR_BIT) - 1))

/**
 * Tries to convert a given value to an inline object, returns error with code=SILC_ERR_VALUE_OUT_OF_RANGE
 * in case of overflow.
 */
static inline silc_obj silc_int_to_obj(int val) {
  silc_obj obj = 0;

  if (val == 0) {
    return SILC_OBJ_ZERO; /* shortcut */
  }

  int sign;
  if (val < 0) {
    sign = -1;
    obj = -val;
  } else {
    sign = 1;
    obj = val;
  }

  if (obj >= SILC_MAX_INT) {
    return silc_err_from_code(SILC_ERR_VALUE_OUT_OF_RANGE);
  }

  if (sign < 0) {
    obj |= SILC_INT_SIGN_BIT;
  }

  obj = obj << (SILC_OBJ_INL_SUBTYPE_SHIFT + SILC_OBJ_TYPE_SHIFT);
  obj |= SILC_OBJ_INL_TYPE | (SILC_OBJ_INL_SUBTYPE_INT << (SILC_OBJ_TYPE_SHIFT));

  return obj;
}

static inline int silc_obj_to_int(silc_obj o) {
  assert((SILC_GET_TYPE(o) == SILC_OBJ_INL_TYPE) && (SILC_GET_INL_SUBTYPE(o) == SILC_OBJ_INL_SUBTYPE_INT));

  int val = (int) (o >> (SILC_OBJ_TYPE_SHIFT + SILC_OBJ_INL_SUBTYPE_SHIFT));
  if (val & SILC_INT_SIGN_BIT) {
    /* negative */
    val = val & (~SILC_INT_SIGN_BIT);
    val = -val;
  }

  return val;
}

silc_obj silc_eq(struct silc_ctx_t* c, silc_obj lhs, silc_obj rhs);
silc_obj silc_hash_code(struct silc_ctx_t* c, silc_obj o);

silc_obj silc_read(struct silc_ctx_t * c, FILE * f);
void silc_print(struct silc_ctx_t* c, silc_obj o, FILE* out);

int silc_get_ref_subtype(struct silc_ctx_t* c, silc_obj o);

silc_obj silc_sym_from_buf(struct silc_ctx_t* c, const char* buf, int size);
silc_obj silc_get_sym_info(struct silc_ctx_t* c, silc_obj o, silc_obj* sym_str);
/** Sets new symbol association, returns old association */
silc_obj silc_set_sym_assoc(struct silc_ctx_t* c, silc_obj o, silc_obj new_assoc);

silc_obj silc_str(struct silc_ctx_t* c, const char* buf, int size);
int silc_get_str_chars(struct silc_ctx_t* c, silc_obj o, char* buf, int pos, int size);

silc_obj silc_cons(struct silc_ctx_t* c, silc_obj car, silc_obj cdr);
silc_obj silc_car(struct silc_ctx_t* c, silc_obj cons);
silc_obj silc_cdr(struct silc_ctx_t* c, silc_obj cons);

#ifdef __cplusplus
}
#endif
