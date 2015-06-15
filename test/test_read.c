#include "silc.h"

#include <stdbool.h>

#define TEST_BEFORE(test_id)    open_tmpfiles()
#define TEST_AFTER(test_id)     close_tmpfiles()
#include "test.h"

#include "test_helpers.h"

static void write_and_rewind(FILE* f, const char* str) {
  fwrite(str, 1, strlen(str), f);
  fflush(f);
  fseek(f, 0, SEEK_SET);
}


static FILE* out = NULL;
static FILE* in = NULL;

static void open_tmpfiles() {
  out = tmpfile();
  in = tmpfile();
}

static void close_tmpfiles() {
  if (out != NULL) {
    fclose(out);
    out = NULL;
  }
  if (in != NULL) {
    fclose(in);
    in = NULL;
  }
}

static inline silc_obj read_obj(struct silc_ctx_t* c) {
  return silc_read(c, out, silc_err_from_code(SILC_ERR_UNEXPECTED_EOF));
}

BEGIN_TEST_METHOD(test_read_number)
  /* Given: */
  struct silc_ctx_t* c = silc_new_context();
  write_and_rewind(out, "123");

  /* When: */
  silc_obj o = not_an_error(read_obj(c));

  /* Then: */
  ASSERT(silc_int_to_obj(123) == o);
  silc_free_context(c);
END_TEST_METHOD()

BEGIN_TEST_METHOD(test_read_cons_single)
  /* Given: */
  struct silc_ctx_t* c = silc_new_context();
  write_and_rewind(out, "(1)");

  /* When: */
  silc_obj o = not_an_error(read_obj(c));

  /* Then: */
  silc_obj car = silc_car(c, o);
  silc_obj cdr = silc_cdr(c, o);
  ASSERT(silc_int_to_obj(1) == car);
  ASSERT(SILC_OBJ_NIL == cdr);

  /* Check representation */
  silc_print(c, o, in);
  READ_BUF(in, buf);
  ASSERT(0 == strcmp(buf, "(1)"));

  silc_free_context(c);
END_TEST_METHOD()

BEGIN_TEST_METHOD(test_read_cons_multiple)
  /* Given: */
  struct silc_ctx_t* c = silc_new_context();
  write_and_rewind(out, "(1 0 -1)");

  /* When: */
  silc_obj o = not_an_error(read_obj(c));

  /* Then: */
  silc_obj car = silc_car(c, o);
  silc_obj cdr = silc_cdr(c, o);
  ASSERT(silc_int_to_obj(1) == car);
  ASSERT(SILC_OBJ_NIL != cdr);

  /* Check representation */
  silc_print(c, o, in);
  READ_BUF(in, buf);
  ASSERT(0 == strcmp(buf, "(1 0 -1)"));

  silc_free_context(c);
END_TEST_METHOD()

BEGIN_TEST_METHOD(test_read_cons_nested)
  /* Given: */
  struct silc_ctx_t* c = silc_new_context();
  write_and_rewind(out, "(1 (2 3 (4)))");

  /* When: */
  silc_obj o = not_an_error(read_obj(c));

  /* Then: */
  silc_obj car = silc_car(c, o);
  silc_obj cdr = silc_cdr(c, o);
  ASSERT(silc_int_to_obj(1) == car);
  ASSERT(SILC_OBJ_NIL != cdr);

  /* Check representation */
  silc_print(c, o, in);
  READ_BUF(in, buf);
  ASSERT(0 == strcmp(buf, "(1 (2 3 (4)))"));

  silc_free_context(c);
END_TEST_METHOD()

BEGIN_TEST_METHOD(test_read_special_true)
  /* Given: */
  struct silc_ctx_t* c = silc_new_context();
  write_and_rewind(out, "true");

  /* When: */
  silc_obj o = not_an_error(read_obj(c));

  /* Then: */
  ASSERT(SILC_OBJ_TRUE == o);
  silc_free_context(c);
END_TEST_METHOD()

BEGIN_TEST_METHOD(test_read_special_false)
  /* Given: */
  struct silc_ctx_t* c = silc_new_context();
  write_and_rewind(out, "false");

  /* When: */
  silc_obj o = not_an_error(read_obj(c));

  /* Then: */
  ASSERT(SILC_OBJ_FALSE == o);
  silc_free_context(c);
END_TEST_METHOD()

BEGIN_TEST_METHOD(test_read_special_nil)
  /* Given: */
  struct silc_ctx_t* c = silc_new_context();
  write_and_rewind(out, "nil");

  /* When: */
  silc_obj o = not_an_error(read_obj(c));

  /* Then: */
  ASSERT(SILC_OBJ_NIL == o);
  silc_free_context(c);
END_TEST_METHOD()


static void helper_test_read_symbol(const char* symbol_str, bool check_assoc) {
  /* Given: */
  struct silc_ctx_t* c = silc_new_context();
  write_and_rewind(out, symbol_str);

  /* When: */
  silc_obj o = not_an_error(read_obj(c));

  /* Then: */
  silc_obj str = SILC_OBJ_NIL;
  silc_obj assoc = silc_get_sym_info(c, o, &str);
  if (check_assoc) {
    ASSERT(SILC_ERR_UNRESOLVED_SYMBOL == silc_try_get_err_code(assoc));
  }
  assert_same_str(c, str, symbol_str, strlen(symbol_str));

  silc_free_context(c);
}

BEGIN_TEST_METHOD(test_read_single_char_symbol)
  helper_test_read_symbol("s", true);
END_TEST_METHOD()

BEGIN_TEST_METHOD(test_read_multichar_symbol_with_dashes)
  helper_test_read_symbol("aaa-bb-c", true);
END_TEST_METHOD()

BEGIN_TEST_METHOD(test_read_multichar_alphanum_symbol)
  helper_test_read_symbol("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789*&+-", true);
END_TEST_METHOD()

BEGIN_TEST_METHOD(test_read_minus_sign)
  helper_test_read_symbol("-", false);
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
  test_read_single_char_symbol();
  test_read_multichar_symbol_with_dashes();
  test_read_multichar_alphanum_symbol();
  test_read_minus_sign();
  TESTS_SUCCEEDED();
  return 0;
}
