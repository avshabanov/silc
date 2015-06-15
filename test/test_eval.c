#include "silc.h"

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

static void assert_eval_result(struct silc_ctx_t* c, const char* input, const char* expected_eval_result) {
  /* Given: */
  write_and_rewind(out, input);

  /* When: */
  silc_obj result = not_an_error(silc_eval(c, silc_read(c, out, silc_err_from_code(SILC_ERR_UNEXPECTED_EOF))));

  /* Then: */
  silc_print(c, result, in);
  READ_BUF(in, buf);
  if (0 != strcmp(buf, expected_eval_result)) {
    fprintf(stderr, "[eval] actual_result=%s, expected_result=%s\n", buf, expected_eval_result);
    ASSERT(!"results are not matching");
  }
}

BEGIN_TEST_METHOD(test_eval_inline)
  struct silc_ctx_t* c = silc_new_context();
  assert_eval_result(c, "1", "1");
  silc_free_context(c);
END_TEST_METHOD()

BEGIN_TEST_METHOD(test_eval_empty_cons)
  struct silc_ctx_t* c = silc_new_context();
  assert_eval_result(c, "()", "nil");
  silc_free_context(c);
END_TEST_METHOD()

BEGIN_TEST_METHOD(test_eval_inc)
  struct silc_ctx_t* c = silc_new_context();
  assert_eval_result(c, "(inc 0)", "1");
  silc_free_context(c);
END_TEST_METHOD()

BEGIN_TEST_METHOD(test_eval_nested_addition)
  struct silc_ctx_t* c = silc_new_context();
  assert_eval_result(c, "(inc (inc (inc 0)))", "3");
  silc_free_context(c);
END_TEST_METHOD()

BEGIN_TEST_METHOD(test_eval_define)
  struct silc_ctx_t* c = silc_new_context();
  assert_eval_result(c, "(define b (+ 2 (inc 1)))", "4");
  silc_free_context(c);
END_TEST_METHOD()

BEGIN_TEST_METHOD(test_eval_begin)
  struct silc_ctx_t* c = silc_new_context();
  assert_eval_result(c, "(begin 1 2 (+ 3 1))", "4");
  silc_free_context(c);
END_TEST_METHOD()

BEGIN_TEST_METHOD(test_eval_multi_define)
  struct silc_ctx_t* c = silc_new_context();
  assert_eval_result(c, "(begin (define b (+ 2 (inc 1))) (define a 1) (+ a b))", "5");
  silc_free_context(c);
END_TEST_METHOD()

BEGIN_TEST_METHOD(test_eval_cons)
  struct silc_ctx_t* c = silc_new_context();
  assert_eval_result(c, "(cons 1 (cons 2 (cons 3 nil)))", "(1 2 3)");
  silc_free_context(c);
END_TEST_METHOD()

BEGIN_TEST_METHOD(test_eval_binary_arithmetic_operations)
  struct silc_ctx_t* c = silc_new_context();
  assert_eval_result(c, "(cons (+ 1 2) (cons (- 4 3) (cons (/ 10 5) (cons (* 2 4) nil))))", "(3 1 2 8)");
  silc_free_context(c);
END_TEST_METHOD()

BEGIN_TEST_METHOD(test_eval_empty_lambda)
  struct silc_ctx_t* c = silc_new_context();
  assert_eval_result(c, "((lambda ()))", "nil");
  silc_free_context(c);
END_TEST_METHOD()

BEGIN_TEST_METHOD(test_eval_ret1_lambda)
  struct silc_ctx_t* c = silc_new_context();
  assert_eval_result(c, "((lambda () 1))", "1");
  silc_free_context(c);
END_TEST_METHOD()

BEGIN_TEST_METHOD(test_eval_argval_lambda)
  struct silc_ctx_t* c = silc_new_context();
  assert_eval_result(c, "((lambda (a) (inc a)) 1)", "2");
  silc_free_context(c);
END_TEST_METHOD()

BEGIN_TEST_METHOD(test_eval_restore_args)
  struct silc_ctx_t* c = silc_new_context();
  assert_eval_result(
    c,
    "(begin (define a 10) (define b 1) (define cf (lambda (a b) (+ (* 2 a) b))) (+ (cf 1000 100) a (* 8 b)))",
    "2118");
  silc_free_context(c);
END_TEST_METHOD()

BEGIN_TEST_METHOD(test_eval_gc)
  struct silc_ctx_t* c = silc_new_context();
  silc_set_default_out(c, in);

  write_and_rewind(out, "(gc)");
  silc_obj result = not_an_error(silc_eval(c, silc_read(c, out, silc_err_from_code(SILC_ERR_UNEXPECTED_EOF))));

  ASSERT(SILC_OBJ_NIL == result);

  silc_free_context(c);
END_TEST_METHOD()

BEGIN_TEST_METHOD(test_eval_nonfunction)
  struct silc_ctx_t* c = silc_new_context();

  write_and_rewind(out, "(1)");
  silc_obj result = silc_eval(c, silc_read(c, out, silc_err_from_code(SILC_ERR_UNEXPECTED_EOF)));

  ASSERT(SILC_ERR_NOT_A_FUNCTION == silc_try_get_err_code(result));

  silc_free_context(c);
END_TEST_METHOD()

BEGIN_TEST_METHOD(test_eval_unresolved_sym)
  struct silc_ctx_t* c = silc_new_context();

  write_and_rewind(out, "(unknownsymbol)");
  silc_obj result = silc_eval(c, silc_read(c, out, silc_err_from_code(SILC_ERR_UNEXPECTED_EOF)));

  ASSERT(SILC_ERR_UNRESOLVED_SYMBOL == silc_try_get_err_code(result));

  silc_free_context(c);
END_TEST_METHOD()

int main(int argc, char** argv) {
  TESTS_STARTED();
  test_eval_inline();
  test_eval_empty_cons();
  test_eval_inc();
  test_eval_nested_addition();
  test_eval_define();
  test_eval_begin();
  test_eval_multi_define();
  test_eval_cons();
  test_eval_binary_arithmetic_operations();
  test_eval_empty_lambda();
  test_eval_ret1_lambda();
  test_eval_argval_lambda();
  test_eval_restore_args();
  test_eval_gc();
  test_eval_nonfunction();
  test_eval_unresolved_sym();
  TESTS_SUCCEEDED();
  return 0;
}
