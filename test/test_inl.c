#include "test.h"
#include "silc.h"

BEGIN_TEST_METHOD(test_nil_equivalence_to_zero)
  /* zero should also be equivalent to NIL */
  assert(0 == SILC_OBJ_NIL);
END_TEST_METHOD()

BEGIN_TEST_METHOD(test_int_conversions)
  silc_obj o;

  o = silc_int_to_obj(0);
  assert(0 == silc_obj_to_int(o));

  o = silc_int_to_obj(1);
  assert(1 == silc_obj_to_int(o));

  o = silc_int_to_obj(1329);
  assert(1329 == silc_obj_to_int(o));

  o = silc_int_to_obj(-231234);
  assert(-231234 == silc_obj_to_int(o));

  o &= (~SILC_INT_OBJ_SIGN_BIT); /* change sign */
  assert(231234 == silc_obj_to_int(o));

  o = silc_int_to_obj(-1);
  assert(-1 == silc_obj_to_int(o));
END_TEST_METHOD()

BEGIN_TEST_METHOD(test_inline_errors)
  silc_obj o;

  o = silc_err_from_code(SILC_ERR_INTERNAL);
  assert(silc_try_get_err_code(o) == SILC_ERR_INTERNAL);
END_TEST_METHOD()

BEGIN_TEST_METHOD(test_non_error_object_to_code_conversion)
  silc_obj o;

  o = SILC_OBJ_NIL;
  assert(silc_try_get_err_code(o) < 0 && "nil to error code convesion should fail");

  o = SILC_OBJ_TRUE;
  assert(silc_try_get_err_code(o) < 0 && "true to error code convesion should fail");

  o = SILC_OBJ_FALSE;
  assert(silc_try_get_err_code(o) < 0 && "false to error code convesion should fail");

  o = SILC_OBJ_ZERO;
  assert(silc_try_get_err_code(o) < 0 && "0 to error code convesion should fail");

  o = silc_int_to_obj(-1);
  assert(silc_try_get_err_code(o) < 0 && "-1 to error code convesion should fail");
END_TEST_METHOD()

int main(int argc, char** argv) {
  TESTS_STARTED();
  test_nil_equivalence_to_zero();
  test_int_conversions();
  test_inline_errors();
  test_non_error_object_to_code_conversion();
  TESTS_SUCCEEDED();
  return 0;
}
