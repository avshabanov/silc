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

  return result;
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

silc_obj silc_internal_fn_quit(struct silc_funcall_t* f) {
  int code = 0;
  if (f->argc > 0) {
    silc_obj arg1 = f->argv[0];
    if (SILC_GET_INL_SUBTYPE(arg1) == SILC_INL_SUBTYPE_INT) {
      code = silc_obj_to_int(arg1);
    }
  }
  exit(code);
  return SILC_OBJ_NIL;
}
