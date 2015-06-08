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
  //fprintf(stderr, "actual_result=%s\n", buf);
  assert(0 == strcmp(buf, expected_eval_result));
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

int main(int argc, char** argv) {
  TESTS_STARTED();
  test_eval_inline();
  test_eval_empty_cons();
  test_eval_inc();
  test_eval_nested_addition();
  TESTS_SUCCEEDED();
  return 0;
}
