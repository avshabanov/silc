#include "test.h"
#include "test_helpers.h"

#include "silc.h"

BEGIN_TEST_METHOD(test_object_type)
  assert(SILC_TYPE_INL  != SILC_TYPE_CONS &&
         SILC_TYPE_INL  != SILC_TYPE_OREF &&
         SILC_TYPE_INL  != SILC_TYPE_BREF &&
         SILC_TYPE_CONS != SILC_TYPE_OREF &&
         SILC_TYPE_CONS != SILC_TYPE_BREF &&
         SILC_TYPE_OREF != SILC_TYPE_BREF);

  /* inline object test skipped - it is done in test_inl */

  struct silc_ctx_t * c = silc_new_context();

  silc_obj o;

  o = silc_cons(c, SILC_OBJ_NIL, SILC_OBJ_NIL);
  assert(SILC_GET_TYPE(o) == SILC_TYPE_CONS);

  o = silc_sym_from_buf(c, "s", 1);
  assert(SILC_GET_TYPE(o) == SILC_TYPE_OREF);

  /* TODO: bref test */

  silc_free_context(c);
END_TEST_METHOD()

BEGIN_TEST_METHOD(test_sym_create)
  struct silc_ctx_t * c = silc_new_context();

  silc_obj o1;
  silc_obj o2;

  o1 = silc_sym_from_buf(c, "s", 1);
  o2 = silc_sym_from_buf(c, "s", 1);
  assert(o1 == o2);

  o2 = silc_sym_from_buf(c, "ss", 2);
  assert(o1 != o2);

  char buf[10];

  /* create N symbols */
  silc_obj syms[16500];
  for (size_t n = 0; n < countof(syms); ++n) {
    size_t sz = (size_t) sprintf(buf, "s%zu", n);
    syms[n] = silc_sym_from_buf(c, buf, sz);
  }

  /* try recreate N symbols - it should result in matching symbols */
  for (size_t n = 0; n < countof(syms); ++n) {
    size_t sz = (size_t) sprintf(buf, "s%zu", n);
    silc_obj sym = silc_sym_from_buf(c, buf, sz);
    assert(syms[n] == sym);

    /* get symbol information */
    silc_obj str = SILC_OBJ_NIL;
    silc_obj assoc = silc_get_sym_info(c, sym, &str);
    assert(SILC_ERR_UNRESOLVED_SYMBOL == silc_try_get_err_code(assoc));
    assert_same_str(c, str, buf, sz);

    /* set new association */
    assoc = silc_set_sym_assoc(c, sym, silc_int_to_obj(n));
    assert(SILC_ERR_UNRESOLVED_SYMBOL == silc_try_get_err_code(assoc)); /* verify initial assoc */
  }

  /* test new assocs */
  for (size_t n = 0; n < countof(syms); ++n) {
    size_t sz = (size_t) sprintf(buf, "s%zu", n);
    silc_obj sym = silc_sym_from_buf(c, buf, sz);
    assert(syms[n] == sym);

    /* get symbol information */
    silc_obj assoc = silc_get_sym_info(c, sym, NULL);
    assert(silc_int_to_obj(n) == assoc);

    /* set new association */
    assoc = silc_set_sym_assoc(c, sym, SILC_OBJ_NIL);
    assert(silc_int_to_obj(n) == assoc); /* old assoc should be null */
  }

  silc_free_context(c);
END_TEST_METHOD()

int main(int argc, char** argv) {
  TESTS_STARTED();
  test_object_type();
  test_sym_create();
  TESTS_SUCCEEDED();
  return 0;
}
