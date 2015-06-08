#include "silc.h"

#define TEST_BEFORE(test_id)    open_tmpfile()
#define TEST_AFTER(test_id)     close_tmpfile()
#include "test.h"

#include "test_helpers.h"

/* Helpers */

static FILE* out = NULL;

static void open_tmpfile() {
  out = tmpfile();
}

static void close_tmpfile() {
  if (out != NULL) {
    fclose(out);
    out = NULL;
  }
}

static void call_print(struct silc_ctx_t* c, silc_obj o, FILE* out) {
  silc_print(c, o, out);
  fputc(';', out);
}

BEGIN_TEST_METHOD(test_print_primitives)
  struct silc_ctx_t* c = silc_new_context();

  call_print(c, SILC_OBJ_FALSE, out);
  call_print(c, SILC_OBJ_TRUE, out);
  call_print(c, silc_int_to_obj(0), out);
  call_print(c, silc_int_to_obj(1), out);
  call_print(c, silc_int_to_obj(-1), out);

  /* Test contents */
  READ_BUF(out, buf);
  assert(0 == strcmp(buf, "false;true;0;1;-1;"));

  silc_free_context(c);
END_TEST_METHOD()

BEGIN_TEST_METHOD(test_print_small_symbol)
  struct silc_ctx_t* c = silc_new_context();

  const char* sym_str = "s";
  silc_print(c, silc_sym_from_buf(c, sym_str, strlen(sym_str)), out);

  /* Test contents */
  READ_BUF(out, buf);
  assert(0 == strcmp(buf, sym_str));

  silc_free_context(c);
END_TEST_METHOD()

BEGIN_TEST_METHOD(test_print_large_symbol)
  struct silc_ctx_t* c = silc_new_context();

  const char* sym_str = "a0123456789012345678901234567890123456789";
  silc_print(c, silc_sym_from_buf(c, sym_str, strlen(sym_str)), out);

  /* Test contents */
  READ_BUF(out, buf);
  assert(0 == strcmp(buf, sym_str));

  silc_free_context(c);
END_TEST_METHOD()

BEGIN_TEST_METHOD(test_print_cons_primitive)
  struct silc_ctx_t* c = silc_new_context();

  silc_obj cons = silc_cons(c, silc_int_to_obj(1),
                    silc_cons(c, silc_int_to_obj(2),
                      silc_cons(c, silc_int_to_obj(3),
                        SILC_OBJ_NIL)));

  silc_print(c, cons, out);

  /* Test contents */
  READ_BUF(out, buf);
  assert(0 == strcmp(buf, "(1 2 3)"));

  silc_free_context(c);
END_TEST_METHOD()

BEGIN_TEST_METHOD(test_print_cons_nested)
  struct silc_ctx_t* c = silc_new_context();

  silc_obj cons = silc_cons(c, silc_int_to_obj(1),
                    silc_cons(c, SILC_OBJ_FALSE,
                      silc_cons(c, SILC_OBJ_NIL,
                        silc_cons(c, silc_cons(c, SILC_OBJ_TRUE, SILC_OBJ_NIL),
                          SILC_OBJ_NIL))));

  silc_print(c, cons, out);

  /* Test contents */
  READ_BUF(out, buf);
  assert(0 == strcmp(buf, "(1 false nil (true))"));

  silc_free_context(c);
END_TEST_METHOD()

int main(int argc, char** argv) {
  TESTS_STARTED();
  test_print_primitives();
  test_print_small_symbol();
  test_print_large_symbol();
  test_print_cons_primitive();
  test_print_cons_nested();
  TESTS_SUCCEEDED();
  return 0;
}
