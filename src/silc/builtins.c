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

#include "builtins.h"
#include <stdlib.h>

#define EXPECT_ARG_COUNT(f, count) \
  if (f->argc != (count)) { \
    return silc_err_from_code(SILC_ERR_INVALID_ARGS); \
  }

silc_obj silc_internal_fn_define(struct silc_funcall_t* f) {
  EXPECT_ARG_COUNT(f, 2);

  silc_obj arg_name;
  silc_obj arg_val;
  SILC_CHECKED_SET(arg_name, f->argv[0]);
  SILC_CHECKED_SET(arg_val, silc_eval(f->ctx, f->argv[1]));

  silc_set_sym_assoc(f->ctx, arg_name, arg_val);
  return arg_val;
}

silc_obj silc_internal_fn_lambda(struct silc_funcall_t* f) {
  if (f->argc < 1 || f->argc > 2) {
    return silc_err_from_code(SILC_ERR_INVALID_ARGS);
  }
  silc_obj body = SILC_OBJ_NIL;
  if (f->argc == 2) {
    SILC_CHECKED_SET(body, f->argv[1]);
  }
  return silc_define_function(f->ctx, f->argv[0], body);
}

silc_obj silc_internal_fn_print(struct silc_funcall_t* f) {
  EXPECT_ARG_COUNT(f, 1);

  silc_obj arg;
  SILC_CHECKED_SET(arg, f->argv[0]);
  silc_print(f->ctx, arg, silc_get_default_out(f->ctx));

  return SILC_OBJ_NIL;
}

silc_obj silc_internal_fn_plus(struct silc_funcall_t* f) {
  int result = 0;

  for (int i = 0; i < f->argc; ++i) {
    silc_obj arg;
    SILC_CHECKED_SET(arg, f->argv[i]);

    if ((SILC_GET_TYPE(arg) == SILC_TYPE_INL) && (SILC_GET_INL_SUBTYPE(arg) == SILC_INL_SUBTYPE_INT)) {
      result += silc_obj_to_int(arg);

      if (result > SILC_MAX_INT) {
        /* TODO: upgrade to long (or BigInteger) */
        return silc_err_from_code(SILC_ERR_VALUE_OUT_OF_RANGE);
      }
    } else {
      return silc_err_from_code(SILC_ERR_INVALID_ARGS); /* Non-incrementable argument */
    }
  }

  return silc_int_to_obj(result);
}

silc_obj silc_internal_fn_inc(struct silc_funcall_t* f) {
  EXPECT_ARG_COUNT(f, 1);

  silc_obj arg;
  SILC_CHECKED_SET(arg, f->argv[0]);

  if ((SILC_GET_TYPE(arg) == SILC_TYPE_INL) && (SILC_GET_INL_SUBTYPE(arg) == SILC_INL_SUBTYPE_INT)) {
    int val = silc_obj_to_int(arg) + 1;
    if (val > SILC_MAX_INT) {
      /* TODO: upgrade to long (or BigInteger) */
      return silc_err_from_code(SILC_ERR_VALUE_OUT_OF_RANGE);
    }

    return silc_int_to_obj(val);
  }

  return silc_err_from_code(SILC_ERR_INVALID_ARGS); /* Non-incrementable argument */
}

silc_obj silc_internal_fn_load(struct silc_funcall_t* f) {
  fputs(";; load is not implemented yet\n", stderr);
  return silc_err_from_code(SILC_ERR_INTERNAL);
}

silc_obj silc_internal_fn_gc(struct silc_funcall_t* f) {
  fputs(";; starting garbage collection...\n", stdout);
  silc_gc(f->ctx);
  fputs(";; garbage collected.\n", stdout);
  return SILC_OBJ_NIL;
}

silc_obj silc_internal_fn_quit(struct silc_funcall_t* f) {
  if (f->argc > 0) {
    silc_obj arg;
    SILC_CHECKED_SET(arg, f->argv[0]);
    if ((SILC_GET_TYPE(arg) == SILC_TYPE_INL) && (SILC_GET_INL_SUBTYPE(arg) == SILC_INL_SUBTYPE_INT)) {
      silc_set_exit_code(f->ctx, silc_obj_to_int(arg));
    }
  }
  return silc_err_from_code(SILC_ERR_QUIT);
}
