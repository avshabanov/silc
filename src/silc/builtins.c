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

silc_obj silc_internal_fn_print(struct silc_fn_t* f) {
  if (arg_count(f->ctx) != 1) {
    return silc_err_from_code(SILC_ERR_INVALID_ARGS);
  }

  silc_print(f->ctx, arg_peek(f->ctx, 0), f->ctx->settings->out);

  return SILC_OBJ_NIL;
}

silc_obj silc_internal_fn_inc(struct silc_fn_t* f) {
  if (arg_count(f->ctx) != 1) {
    return silc_err_from_code(SILC_ERR_INVALID_ARGS);
  }

  silc_obj arg = arg_peek(f->ctx, 0);

  if ((SILC_GET_TYPE(arg) == SILC_OBJ_INL_TYPE) && (SILC_GET_INL_SUBTYPE(arg) == SILC_OBJ_INL_SUBTYPE_INT)) {
    int val = silc_obj_to_int(arg) + 1;
    if (val > SILC_MAX_INT) {
      val = -1;
    }

    return silc_int_to_obj(val);
  }

  return SILC_OBJ_ZERO; /* Non-incrementable argument */
}
