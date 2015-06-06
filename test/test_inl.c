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

int main(int argc, char** argv) {
  TESTS_STARTED();
  test_nil_equivalence_to_zero();
  test_int_conversions();
  TESTS_SUCCEEDED();
  return 0;
}
