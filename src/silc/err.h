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

static inline const char* get_error_message(int code) {
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

    default:
      return "unknown error";
  }
}
