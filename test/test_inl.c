#include "test.h"
#include "silc.h"


BEGIN_TEST_METHOD(test_inline_object_subtype)
  /* make sure types are all different */
  ASSERT(SILC_INL_SUBTYPE_NIL  != SILC_INL_SUBTYPE_BOOL &&
         SILC_INL_SUBTYPE_NIL  != SILC_INL_SUBTYPE_INT &&
         SILC_INL_SUBTYPE_NIL  != SILC_INL_SUBTYPE_ERR &&
         SILC_INL_SUBTYPE_BOOL != SILC_INL_SUBTYPE_INT &&
         SILC_INL_SUBTYPE_BOOL != SILC_INL_SUBTYPE_ERR &&
         SILC_INL_SUBTYPE_INT  != SILC_INL_SUBTYPE_ERR);

  silc_obj o;

  o = SILC_OBJ_NIL;
  ASSERT(SILC_GET_TYPE(o) == SILC_TYPE_INL && SILC_GET_INL_SUBTYPE(o) == SILC_INL_SUBTYPE_NIL);

  o = SILC_OBJ_FALSE;
  ASSERT(SILC_GET_TYPE(o) == SILC_TYPE_INL && SILC_GET_INL_SUBTYPE(o) == SILC_INL_SUBTYPE_BOOL &&
    0 == SILC_GET_INL_CONTENT(o));

  o = SILC_OBJ_TRUE;
  ASSERT(SILC_GET_TYPE(o) == SILC_TYPE_INL && SILC_GET_INL_SUBTYPE(o) == SILC_INL_SUBTYPE_BOOL &&
    1 == SILC_GET_INL_CONTENT(o));

  o = SILC_OBJ_ZERO;
  ASSERT(SILC_GET_TYPE(o) == SILC_TYPE_INL && SILC_GET_INL_SUBTYPE(o) == SILC_INL_SUBTYPE_INT &&
    0 == SILC_GET_INL_CONTENT(o));
END_TEST_METHOD()

BEGIN_TEST_METHOD(test_nil_equivalence_to_zero)
  /* zero should also be equivalent to NIL */
  ASSERT(0 == SILC_OBJ_NIL);
END_TEST_METHOD()

BEGIN_TEST_METHOD(test_int_conversions)
  silc_obj o;

  o = silc_int_to_obj(0);
  ASSERT(0 == silc_obj_to_int(o));

  o = silc_int_to_obj(1);
  ASSERT(1 == silc_obj_to_int(o));

  o |= SILC_INT_OBJ_SIGN_BIT; /* change sign */
  ASSERT(-1 == silc_obj_to_int(o));

  o = silc_int_to_obj(1329);
  ASSERT(1329 == silc_obj_to_int(o));

  o = silc_int_to_obj(-231234);
  ASSERT(-231234 == silc_obj_to_int(o));

  o &= (~SILC_INT_OBJ_SIGN_BIT); /* change sign */
  ASSERT(231234 == silc_obj_to_int(o));

  o = silc_int_to_obj(-1);
  ASSERT(-1 == silc_obj_to_int(o));
END_TEST_METHOD()

BEGIN_TEST_METHOD(test_inline_errors)
  silc_obj o;

  o = silc_err_from_code(SILC_ERR_INTERNAL);
  ASSERT(silc_try_get_err_code(o) == SILC_ERR_INTERNAL);
END_TEST_METHOD()

BEGIN_TEST_METHOD(test_non_error_object_to_code_conversion)
  silc_obj o;

  o = SILC_OBJ_NIL;
  ASSERT(silc_try_get_err_code(o) < 0 && "nil to error code convesion should fail");

  o = SILC_OBJ_TRUE;
  ASSERT(silc_try_get_err_code(o) < 0 && "true to error code convesion should fail");

  o = SILC_OBJ_FALSE;
  ASSERT(silc_try_get_err_code(o) < 0 && "false to error code convesion should fail");

  o = SILC_OBJ_ZERO;
  ASSERT(silc_try_get_err_code(o) < 0 && "0 to error code convesion should fail");

  o = silc_int_to_obj(-1);
  ASSERT(silc_try_get_err_code(o) < 0 && "-1 to error code convesion should fail");
END_TEST_METHOD()



static silc_obj return_error() {
  return silc_err_from_code(SILC_ERR_INTERNAL);
}

static silc_obj do_error_op() {
  silc_obj result;
  SILC_CHECKED_SET(result, return_error());
  return (result == SILC_OBJ_NIL ? SILC_OBJ_TRUE : SILC_OBJ_FALSE);
}

BEGIN_TEST_METHOD(test_do_error_op)
  silc_obj o = do_error_op();
  ASSERT(silc_try_get_err_code(o) == SILC_ERR_INTERNAL);
END_TEST_METHOD()



static silc_obj return_nil() {
  return SILC_OBJ_NIL;
}

static silc_obj do_normal_op() {
  silc_obj result;
  SILC_CHECKED_SET(result, return_nil());
  return (result == SILC_OBJ_NIL ? SILC_OBJ_TRUE : SILC_OBJ_FALSE);
}

BEGIN_TEST_METHOD(test_do_normal_op)
  silc_obj o = do_normal_op();
  ASSERT(silc_try_get_err_code(o) < 0);
  ASSERT(o == SILC_OBJ_TRUE);
END_TEST_METHOD()


/* Entry point */
int main(int argc, char** argv) {
  TESTS_STARTED();
  test_inline_object_subtype();
  test_nil_equivalence_to_zero();
  test_int_conversions();
  test_inline_errors();
  test_non_error_object_to_code_conversion();
  test_do_error_op();
  test_do_normal_op();
  TESTS_SUCCEEDED();
  return 0;
}
