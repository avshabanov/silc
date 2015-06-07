#pragma once

#include "silc.h"
#include <assert.h>
#include <string.h>

#include "test.h"

/* assertions */

static inline void assert_same_str(struct silc_ctx_t* c, silc_obj str, const char* buf, int size) {
  char* sym_buf = xmalloc(size);
  int read = silc_get_str_chars(c, str, sym_buf, 0, size);

  assert(read == size && "string length does not match");
  assert(0 == memcmp(sym_buf, buf, size) && "string contents does not match");

  xfree(sym_buf);
}

static inline silc_obj not_an_error(silc_obj o) {
  int code = silc_try_get_err_code(o);
  if (code < 0) {
    return o; /* ok */
  }

  fprintf(stderr, "Returned object is an error, code=%d, message=%s\n", code, silc_err_code_to_str(code));
  abort();

  return o; /* should not come here */
}
