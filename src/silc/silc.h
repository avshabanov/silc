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

#ifdef NDEBUG
#define SILC_ASSERT(x)      ((void) (x))
#else
#include <assert.h>
#define SILC_ASSERT         assert
#endif /* /NDEBUG */

#ifdef __cplusplus
extern "C" {
#endif

/* Base declarations */

typedef unsigned int silc_obj;

/**
 * Counts of bits, used to represent object type information.
 * See also SILC_INL_INL_CONTENT_BITS
 *
 * Primary types are:
 * <ul>
 *  <li>SILC_TYPE_INL</li>
 *  <li>SILC_TYPE_CONS</li>
 *  <li>SILC_TYPE_OREF</li>
 *  <li>SILC_TYPE_BREF</li>
 * </ul>
 */
#define SILC_INT_TYPE_SHIFT           (2)
#define SILC_INT_TYPE_MASK            ((1 << SILC_INT_TYPE_SHIFT) - 1)

/**
 * Returns one of the type code associated with an object:
 * <ul>
 *  <li>SILC_TYPE_INL</li>
 *  <li>SILC_TYPE_CONS</li>
 *  <li>SILC_TYPE_OREF</li>
 *  <li>SILC_TYPE_BREF</li>
 * </ul>
 */
#define SILC_GET_TYPE(o)              ((o) & SILC_INT_TYPE_MASK)

/**
 * Count of bits per object content excluding type bits.
 * General object layout assumed to be as follows: [{..generic content..}{type bits}]
 */
#define SILC_INT_CONTENT_BIT          ((sizeof(silc_obj) * CHAR_BIT) - SILC_INT_TYPE_SHIFT)


/**
 * Simple objects written as is into the lval.
 * Subtypes: boolean, small signed int
 */
#define SILC_TYPE_INL                 (0)

#define SILC_INT_INL_SUBTYPE_SHIFT    (2)
#define SILC_INT_INL_SUBTYPE_MASK     ((1 << SILC_INT_INL_SUBTYPE_SHIFT) - 1)

/**
 * Returns inline object subtype.
 * Before calling this macro, the caller should make sure that object is of inline subtype.
 *
 * This macro returns one of the inline subtypes:
 * <ul>
 *  <li>SILC_INL_SUBTYPE_NIL</li>
 *  <li>SILC_INL_SUBTYPE_BOOL</li>
 *  <li>SILC_INL_SUBTYPE_INT</li>
 *  <li>SILC_INL_SUBTYPE_ERR</li>
 * </ul>
 */
#define SILC_GET_INL_SUBTYPE(inl_o)   (((inl_o) >> SILC_INT_TYPE_SHIFT) & SILC_INT_INL_SUBTYPE_MASK)

/** Returns content of the inline object stripped from type and subtype information */
#define SILC_GET_INL_CONTENT(content) ((content) >> (SILC_INT_TYPE_SHIFT + SILC_INT_INL_SUBTYPE_SHIFT))

/** Creates inline object along with the type and subtype information */
#define SILC_MAKE_INL_OBJECT(content, subtype)  (SILC_TYPE_INL | \
                                                ((subtype) << SILC_INT_TYPE_SHIFT) | \
                                                ((content) << (SILC_INT_TYPE_SHIFT + SILC_INT_INL_SUBTYPE_SHIFT)))

/**
 * Total count of bits in the inline object content.
 * Inline object layout: [{..inline object content..}{subtype bits}{type bits}]
 */
#define SILC_INL_INL_CONTENT_BITS     (SILC_INT_CONTENT_BIT - SILC_INT_INL_SUBTYPE_SHIFT)

#define SILC_INL_SUBTYPE_NIL          (0)
#define SILC_INL_SUBTYPE_BOOL         (1)
#define SILC_INL_SUBTYPE_INT          (2)
#define SILC_INL_SUBTYPE_ERR          (3)

/* Error codes */

/* Uncategorized error (internal error) */
#define SILC_ERR_INTERNAL             (500)

/* Stack access error (i.e. argument requested is out of stack frame) */
#define SILC_ERR_STACK_ACCESS         (501)
/* Stack exceeded */
#define SILC_ERR_STACK_OVERFLOW       (502)

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
#define SILC_ERR_NOT_A_FUNCTION       (471)

/* Not an error per se, indicates that quit is requested */
#define SILC_ERR_QUIT                 (508)

/**
 * Converts error code to string.
 */
const char* silc_err_code_to_str(int code);

#define SILC_MAX_ERR_CODE             ((1<<(SILC_INL_INL_CONTENT_BITS)) - 1)

static inline silc_obj silc_err_from_code(int err_code) {
  SILC_ASSERT(err_code > 0 && err_code < SILC_MAX_ERR_CODE);
  return SILC_MAKE_INL_OBJECT(err_code, SILC_INL_SUBTYPE_ERR);
}

/**
 * Tries to get an error code from a given object.
 * Returns negative value if this object does not represent an error.
 */
static inline int silc_try_get_err_code(silc_obj obj) {
  if ((SILC_GET_TYPE(obj) != SILC_TYPE_INL) || (SILC_GET_INL_SUBTYPE(obj) != SILC_INL_SUBTYPE_ERR)) {
    return -1;
  }

  return SILC_GET_INL_CONTENT(obj);
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
#define SILC_TYPE_CONS                (1)

/**
 * Points to the sequence of objects (first element is a length (smallint), next one - type (smallint))
 */
#define SILC_TYPE_OREF                (2)

#define SILC_OREF_SYMBOL_SUBTYPE      (10)
#define SILC_OREF_HASHTABLE_SUBTYPE   (20)
#define SILC_OREF_FUNCTION_SUBTYPE    (21)
#define SILC_OREF_ENVIRONMENT_SUBTYPE (22)

/**
 * Contains length, then sequence of bytes.
 */
#define SILC_TYPE_BREF                (3)

#define SILC_BREF_STR_SUBTYPE         (1000)

/**
 * Represents variadic length reference type
 */

/* Base constants */

#define SILC_OBJ_NIL                  SILC_MAKE_INL_OBJECT(0, SILC_INL_SUBTYPE_NIL)

/* Inline false value */
#define SILC_OBJ_FALSE                SILC_MAKE_INL_OBJECT(0, SILC_INL_SUBTYPE_BOOL)

/* Inline true value */
#define SILC_OBJ_TRUE                 SILC_MAKE_INL_OBJECT(1, SILC_INL_SUBTYPE_BOOL)

/* Zero integer */
#define SILC_OBJ_ZERO                 SILC_MAKE_INL_OBJECT(0, SILC_INL_SUBTYPE_INT)



/* Interpreter context */

struct silc_ctx_t;

struct silc_funcall_t {
  /** Calling context */
  struct silc_ctx_t* ctx;

  /** Count of arguments */
  int argc;

  /** Arguments vector */
  silc_obj* argv;
};

/* Service functions */

struct silc_ctx_t * silc_new_context();
void silc_free_context(struct silc_ctx_t * c);


/* Helper functions */

/** Bit that designates sign in the inline int object (relative to object content) */
#define SILC_INT_SIGN_BIT             (1<<(SILC_INL_INL_CONTENT_BITS - 1))
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

  obj = obj << (SILC_INT_INL_SUBTYPE_SHIFT + SILC_INT_TYPE_SHIFT);
  obj |= SILC_TYPE_INL | (SILC_INL_SUBTYPE_INT << (SILC_INT_TYPE_SHIFT));

  return obj;
}

static inline int silc_obj_to_int(silc_obj o) {
  SILC_ASSERT((SILC_GET_TYPE(o) == SILC_TYPE_INL) && (SILC_GET_INL_SUBTYPE(o) == SILC_INL_SUBTYPE_INT));

  int val = SILC_GET_INL_CONTENT(o);
  if (val & SILC_INT_SIGN_BIT) {
    /* negative */
    val = val & (~SILC_INT_SIGN_BIT);
    val = -val;
  }

  return val;
}

/** Triggers manual garbage collection. */
void silc_gc(struct silc_ctx_t* c);

/** Tries to load contents of a given file */
silc_obj silc_load(struct silc_ctx_t* c, const char* file_name);

silc_obj silc_define_function(struct silc_ctx_t* c, silc_obj arg_list, silc_obj body);

silc_obj silc_get_lambda_begin(struct silc_ctx_t* c);

void silc_set_exit_code(struct silc_ctx_t* c, int code);
int silc_get_exit_code(struct silc_ctx_t* c);

silc_obj silc_eq(struct silc_ctx_t* c, silc_obj lhs, silc_obj rhs);
silc_obj silc_hash_code(struct silc_ctx_t* c, silc_obj o);

FILE* silc_set_default_out(struct silc_ctx_t * c, FILE* f);
FILE* silc_get_default_out(struct silc_ctx_t * c);

silc_obj silc_read(struct silc_ctx_t * c, FILE * f, silc_obj eof);
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

/**
 * This is the most important function. It evaluates the given argument in a given context.
 * Returns evaluation result that caller should check for an error.
 */
silc_obj silc_eval(struct silc_ctx_t* c, silc_obj o);

#ifdef __cplusplus
}
#endif
