#include "test.h"
#include "test_helpers.h"

#include "silc.h"

BEGIN_TEST_METHOD(test_object_type)
  ASSERT(SILC_TYPE_INL  != SILC_TYPE_CONS &&
         SILC_TYPE_INL  != SILC_TYPE_OREF &&
         SILC_TYPE_INL  != SILC_TYPE_BREF &&
         SILC_TYPE_CONS != SILC_TYPE_OREF &&
         SILC_TYPE_CONS != SILC_TYPE_BREF &&
         SILC_TYPE_OREF != SILC_TYPE_BREF);

  /* inline object test skipped - it is done in test_inl */

  struct silc_ctx_t * c = silc_new_context();

  silc_obj o;

  o = silc_cons(c, SILC_OBJ_NIL, SILC_OBJ_NIL);
  ASSERT(SILC_GET_TYPE(o) == SILC_TYPE_CONS);

  o = silc_sym_from_buf(c, "s", 1);
  ASSERT(SILC_GET_TYPE(o) == SILC_TYPE_OREF);

  /* TODO: bref test */

  silc_free_context(c);
END_TEST_METHOD()

BEGIN_TEST_METHOD(test_sym_create)
  struct silc_ctx_t* c = silc_new_context();

  silc_obj o1;
  silc_obj o2;

  o1 = silc_sym_from_buf(c, "s", 1);
  o2 = silc_sym_from_buf(c, "s", 1);
  ASSERT(o1 == o2);

  o2 = silc_sym_from_buf(c, "ss", 2);
  ASSERT(o1 != o2);

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
    ASSERT(syms[n] == sym);

    /* get symbol information */
    silc_obj str = SILC_OBJ_NIL;
    silc_obj assoc = silc_get_sym_info(c, sym, &str);
    ASSERT(SILC_ERR_UNRESOLVED_SYMBOL == silc_try_get_err_code(assoc));
    assert_same_str(c, str, buf, sz);

    /* set new association */
    assoc = silc_set_sym_assoc(c, sym, silc_int_to_obj(n));
    ASSERT(SILC_ERR_UNRESOLVED_SYMBOL == silc_try_get_err_code(assoc)); /* verify initial assoc */
  }

  /* test new assocs */
  for (size_t n = 0; n < countof(syms); ++n) {
    size_t sz = (size_t) sprintf(buf, "s%zu", n);
    silc_obj sym = silc_sym_from_buf(c, buf, sz);
    ASSERT(syms[n] == sym);

    /* get symbol information */
    silc_obj assoc = silc_get_sym_info(c, sym, NULL);
    ASSERT(silc_int_to_obj(n) == assoc);

    /* set new association */
    assoc = silc_set_sym_assoc(c, sym, SILC_OBJ_NIL);
    ASSERT(silc_int_to_obj(n) == assoc); /* old assoc should be null */
  }

  silc_free_context(c);
END_TEST_METHOD()

BEGIN_TEST_METHOD(test_hash_table_lookup)
  struct silc_ctx_t* c = silc_new_context();

  silc_obj ht = silc_hash_table(c, 4);
  silc_obj prev;

  /* lookup for unknown object */
  ASSERT(SILC_OBJ_FALSE == silc_hash_table_get(c, ht, silc_int_to_obj(1), SILC_OBJ_FALSE));

  /* insert */
  prev = silc_hash_table_put(c, ht, silc_int_to_obj(1), silc_int_to_obj(4), SILC_OBJ_TRUE);
  ASSERT(SILC_OBJ_TRUE == prev);

  /* lookup previously inserted */
  ASSERT(silc_int_to_obj(4) == silc_hash_table_get(c, ht, silc_int_to_obj(1), SILC_OBJ_NIL));

  /* update */
  prev = silc_hash_table_put(c, ht, silc_int_to_obj(1), silc_int_to_obj(8), SILC_OBJ_TRUE);
  ASSERT(silc_int_to_obj(4) == prev);

  /* lookup previously inserted */
  ASSERT(silc_int_to_obj(8) == silc_hash_table_get(c, ht, silc_int_to_obj(1), SILC_OBJ_NIL));

  silc_free_context(c);
END_TEST_METHOD()

int main(int argc, char** argv) {
  TESTS_STARTED();
  test_object_type();
  test_sym_create();
  test_hash_table_lookup();
  TESTS_SUCCEEDED();
  return 0;
}
