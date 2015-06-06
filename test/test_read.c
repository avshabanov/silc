#include "core.h"
#include "builtins.h"

#define TEST_BEFORE(test_id)    open_tmpfile()
#define TEST_AFTER(test_id)     close_tmpfile()
#include "test.h"

#include "test_helpers.h"

static void write_and_rewind(FILE* f, const char* str) {
  fwrite(str, 1, strlen(str), f);
  fflush(f);
  fseek(f, 0, SEEK_SET);
}


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

BEGIN_TEST_METHOD(test_read_number)
  /* Given: */
  struct silc_ctx_t* c = silc_new_context();
  write_and_rewind(out, "123");

  /* When: */
  silc_obj o = not_an_error(silc_read(c, out));

  /* Then: */
  assert(silc_int_to_obj(123) == o);
  silc_free_context(c);
END_TEST_METHOD()

BEGIN_TEST_METHOD(test_read_cons_single)
  /* Given: */
  struct silc_ctx_t* c = silc_new_context();
  write_and_rewind(out, "(1)");

  /* When: */
  silc_obj o = not_an_error(silc_read(c, out));

  /* Then: */
  silc_obj car = silc_car(c, o);
  silc_obj cdr = silc_cdr(c, o);
  assert(silc_int_to_obj(1) == car);
  assert(SILC_OBJ_NIL == cdr);

  silc_free_context(c);
END_TEST_METHOD()

BEGIN_TEST_METHOD(test_read_cons_multiple)
  /* Given: */
  struct silc_ctx_t* c = silc_new_context();
  write_and_rewind(out, "(1 2 3)");

  /* When: */
  silc_obj o = not_an_error(silc_read(c, out));

  /* Then: */
  silc_obj car = silc_car(c, o);
  silc_obj cdr = silc_cdr(c, o);
  assert(silc_int_to_obj(1) == car);
  assert(SILC_OBJ_NIL != cdr);

  silc_free_context(c);
END_TEST_METHOD()

BEGIN_TEST_METHOD(test_read_cons_nested)
  /* Given: */
  struct silc_ctx_t* c = silc_new_context();
  write_and_rewind(out, "(1 (2 3 (4)))");

  /* When: */
  silc_obj o = not_an_error(silc_read(c, out));

  /* Then: */
  silc_obj car = silc_car(c, o);
  silc_obj cdr = silc_cdr(c, o);
  assert(silc_int_to_obj(1) == car);
  assert(SILC_OBJ_NIL != cdr);

  silc_free_context(c);
END_TEST_METHOD()

BEGIN_TEST_METHOD(test_read_special_true)
  /* Given: */
  struct silc_ctx_t* c = silc_new_context();
  write_and_rewind(out, "true");

  /* When: */
  silc_obj o = not_an_error(silc_read(c, out));

  /* Then: */
  assert(SILC_OBJ_TRUE == o);
  silc_free_context(c);
END_TEST_METHOD()

BEGIN_TEST_METHOD(test_read_special_false)
  /* Given: */
  struct silc_ctx_t* c = silc_new_context();
  write_and_rewind(out, "false");

  /* When: */
  silc_obj o = not_an_error(silc_read(c, out));

  /* Then: */
  assert(SILC_OBJ_FALSE == o);
  silc_free_context(c);
END_TEST_METHOD()

BEGIN_TEST_METHOD(test_read_special_nil)
  /* Given: */
  struct silc_ctx_t* c = silc_new_context();
  write_and_rewind(out, "nil");

  /* When: */
  silc_obj o = not_an_error(silc_read(c, out));

  /* Then: */
  assert(SILC_OBJ_NIL == o);
  silc_free_context(c);
END_TEST_METHOD()

int main(int argc, char** argv) {
  TESTS_STARTED();
  test_read_number();
  test_read_cons_single();
  test_read_cons_multiple();
  test_read_cons_nested();
  test_read_special_true();
  test_read_special_false();
  test_read_special_nil();
  //test_read_list_of_specials();
  TESTS_SUCCEEDED();
  return 0;
}
