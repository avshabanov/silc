#include "test.h"
#include "mem.h"

static void* empty_root_obj_iter_start(struct silc_mem_init_t* mem_init) {
  return NULL;
}

static void* empty_root_obj_iter_next(struct silc_mem_init_t* mem_init,
                                      void* iter_context,
                                      silc_obj** objs, int* size) {
  return NULL;
}

static void oom_abort(struct silc_mem_init_t* mem_init) {
  fputs(";; " __FILE__ " - out of memory, aborting...\n", stderr);
  abort();
}

#define MEM_SIZE          (1024)

static struct silc_mem_init_t g_mem_init_empty_root_objs = {
  .context = NULL,
  .init_memory_size = MEM_SIZE,
  .max_memory_size = MEM_SIZE,
  .root_obj_iter_start = empty_root_obj_iter_start,
  .root_obj_iter_next = empty_root_obj_iter_next,
  .oom_abort = oom_abort,
  .alloc_mem = xmalloc,
  .free_mem = xfree
};

BEGIN_TEST_METHOD(test_get_initial_statistics)
  struct silc_mem_t mem = {0};
  struct silc_mem_t* m = &mem;

  silc_int_mem_init(m, &g_mem_init_empty_root_objs);

  /* Test code goes here */
  struct silc_mem_stats_t stats = {0};

  /* get statistics */
  silc_int_mem_calc_stats(m, &stats);

  assert(MEM_SIZE == stats.total_memory);
  assert(MEM_SIZE == stats.free_memory);
  assert(MEM_SIZE == stats.usable_memory);
  assert(0 == stats.pos_count);
  assert(0 == stats.free_pos_count);

  /* cleanup test objects */
  silc_int_mem_free(m);
END_TEST_METHOD()

BEGIN_TEST_METHOD(test_alloc_cons)
  struct silc_mem_t mem = {0};
  struct silc_mem_t* m = &mem;

  silc_int_mem_init(m, &g_mem_init_empty_root_objs);

  /* Test code goes here - try to allocate and check that allocation succeeeded */
  silc_obj a1[] = { silc_int_to_obj(1), SILC_OBJ_NIL };
  silc_obj o1 = silc_int_mem_alloc(m, 2, a1, SILC_TYPE_CONS, SILC_INT_MEM_CONS_SUBTYPE);

  silc_obj a2[] = { silc_int_to_obj(2), o1 };
  silc_obj o2 = silc_int_mem_alloc(m, 2, a2, SILC_TYPE_CONS, SILC_INT_MEM_CONS_SUBTYPE);

  silc_obj a3[] = { silc_int_to_obj(3), o2 };
  silc_obj o3 = silc_int_mem_alloc(m, 2, a3, SILC_TYPE_CONS, SILC_INT_MEM_CONS_SUBTYPE);

  /* test contents */
  assert(o1 != o2 && o2 != o3 && o3 != o1);

  assert(0 == memcmp(a1, silc_parse_cons(m, o1), 2 * sizeof(silc_obj)));
  assert(0 == memcmp(a2, silc_parse_cons(m, o2), 2 * sizeof(silc_obj)));
  assert(0 == memcmp(a3, silc_parse_cons(m, o3), 2 * sizeof(silc_obj)));

  /* cleanup test objects */
  silc_int_mem_free(m);
END_TEST_METHOD()


BEGIN_TEST_METHOD(test_alloc_oref)
  struct silc_mem_t mem = {0};
  struct silc_mem_t* m = &mem;

  silc_int_mem_init(m, &g_mem_init_empty_root_objs);
  silc_obj* content;
  int content_length;
  int subtype;

  /* Test code goes here - try to allocate and check that allocation succeeeded */
  silc_obj a1[] = { silc_int_to_obj(1001) };
  silc_obj o1 = silc_int_mem_alloc(m, countof(a1), a1, SILC_TYPE_OREF, 10);

  silc_obj a2[] = { silc_int_to_obj(2001), silc_int_to_obj(2002) };
  silc_obj o2 = silc_int_mem_alloc(m, countof(a2), a2, SILC_TYPE_OREF, 11);

  silc_obj a3[] = { silc_int_to_obj(3001), silc_int_to_obj(3002), silc_int_to_obj(3003) };
  silc_obj o3 = silc_int_mem_alloc(m, countof(a3), a3, SILC_TYPE_OREF, 12);

  /* test contents */
  subtype = silc_int_mem_parse_ref(m, o1, &content_length, NULL, &content);
  assert(10 == subtype && content_length == countof(a1) && 0 == memcmp(a1, content, content_length * sizeof(silc_obj)));

  subtype = silc_int_mem_parse_ref(m, o2, &content_length, NULL, &content);
  assert(11 == subtype && content_length == countof(a2) && 0 == memcmp(a2, content, content_length * sizeof(silc_obj)));

  subtype = silc_int_mem_parse_ref(m, o3, &content_length, NULL, &content);
  assert(12 == subtype && content_length == countof(a3) && 0 == memcmp(a3, content, content_length * sizeof(silc_obj)));

  /* cleanup test objects */
  silc_int_mem_free(m);
END_TEST_METHOD()


BEGIN_TEST_METHOD(test_alloc_bref)
  struct silc_mem_t mem = {0};
  struct silc_mem_t* m = &mem;

  silc_int_mem_init(m, &g_mem_init_empty_root_objs);
  char* content;
  int content_length;
  int subtype;

  /* Test code goes here - try to allocate and check that allocation succeeeded */
  const char* a1 = "number one";
  silc_obj o1 = silc_int_mem_alloc(m, strlen(a1), a1, SILC_TYPE_BREF, 20);

  const char* a2 = "numero dos";
  silc_obj o2 = silc_int_mem_alloc(m, strlen(a2), a2, SILC_TYPE_BREF, 21);

  const char* a3 = "three";
  silc_obj o3 = silc_int_mem_alloc(m, strlen(a3), a3, SILC_TYPE_BREF, 22);

  /* test contents */
  subtype = silc_int_mem_parse_ref(m, o1, &content_length, &content, NULL);
  assert(20 == subtype && content_length == strlen(a1) && 0 == memcmp(a1, content, content_length));

  subtype = silc_int_mem_parse_ref(m, o2, &content_length, &content, NULL);
  assert(21 == subtype && content_length == strlen(a2) && 0 == memcmp(a2, content, content_length));

  subtype = silc_int_mem_parse_ref(m, o3, &content_length, &content, NULL);
  assert(22 == subtype && content_length == strlen(a3) && 0 == memcmp(a3, content, content_length));

  /* cleanup test objects */
  silc_int_mem_free(m);
END_TEST_METHOD()

BEGIN_TEST_METHOD(test_gc_full_cleanup)
  struct silc_mem_t mem = {0};
  struct silc_mem_t* m = &mem;

  silc_int_mem_init(m, &g_mem_init_empty_root_objs);

  /* Test code goes here - allocate a few different objects */
  silc_obj a1[] = { silc_int_to_obj(1), SILC_OBJ_NIL };
  silc_obj o1 = silc_int_mem_alloc(m, 2, a1, SILC_TYPE_CONS, SILC_INT_MEM_CONS_SUBTYPE);
  
  silc_obj a2[] = { silc_int_to_obj(2001), silc_int_to_obj(2002) };
  silc_obj o2 = silc_int_mem_alloc(m, countof(a2), a2, SILC_TYPE_OREF, 11);
  
  const char* a3 = "three";
  silc_obj o3 = silc_int_mem_alloc(m, strlen(a3), a3, SILC_TYPE_BREF, 22);

  assert(o1 != o2 && o2 != o3 && o3 != o1);

  /* check statistics */
  struct silc_mem_stats_t stats = {0};

  silc_int_mem_calc_stats(m, &stats);

  assert(3 == stats.pos_count); /* 3 objects allocated in total */
  assert(MEM_SIZE == stats.total_memory);
  assert(stats.total_memory > stats.free_memory);
  assert(stats.free_memory == stats.usable_memory);

  /* trigger gc and recheck statistics */
  silc_int_mem_gc(m);

  silc_int_mem_calc_stats(m, &stats);
  assert(0 == stats.pos_count);
  assert(0 == stats.free_pos_count);
  assert(MEM_SIZE == stats.total_memory);
  assert(stats.total_memory == stats.free_memory);
  assert(stats.free_memory == stats.usable_memory);
 
  /* cleanup test objects */
  silc_int_mem_free(m);
END_TEST_METHOD() 


struct partial_gc_t {
  silc_obj root_obj_arr[10];
};

static void* partial_gc_root_obj_iter_start(struct silc_mem_init_t* mem_init) {
  return mem_init->context;
}

static void* partial_gc_root_obj_iter_next(struct silc_mem_init_t* mem_init,
                                           void* iter_context,
                                           silc_obj** objs, int* size) {
  struct partial_gc_t * pgc = (struct partial_gc_t*) iter_context;
  *objs = pgc->root_obj_arr;
  *size = countof(pgc->root_obj_arr);
  return NULL;
}

BEGIN_TEST_METHOD(test_gc_partial_cleanup)
  struct silc_mem_t mem = {0};
  struct silc_mem_t* m = &mem;

  struct partial_gc_t pgc = {.root_obj_arr = {0}};
  struct silc_mem_init_t init;
  memcpy(&init, &g_mem_init_empty_root_objs, sizeof(struct silc_mem_init_t));
  init.context = &pgc;
  init.root_obj_iter_start = partial_gc_root_obj_iter_start;
  init.root_obj_iter_next = partial_gc_root_obj_iter_next;

  silc_int_mem_init(m, &init);

  /* Test code goes here - allocate a few different objects */
  const char* a1 = "three";
  silc_obj o1 = silc_int_mem_alloc(m, strlen(a1), a1, SILC_TYPE_BREF, 101);

  silc_obj a2[] = { o1, silc_int_to_obj(2002) };
  silc_obj o2 = silc_int_mem_alloc(m, countof(a2), a2, SILC_TYPE_OREF, 102);

  silc_obj a3[] = { silc_int_to_obj(3), SILC_OBJ_NIL };
  silc_obj o3 = silc_int_mem_alloc(m, 2, a3, SILC_TYPE_CONS, SILC_INT_MEM_CONS_SUBTYPE);

  silc_obj o4 = silc_int_mem_alloc(m, 1, NULL, SILC_TYPE_BREF, 104);

  silc_obj a5[] = { o4, SILC_OBJ_NIL };
  silc_obj o5 = silc_int_mem_alloc(m, 2, a5, SILC_TYPE_CONS, SILC_INT_MEM_CONS_SUBTYPE);

  const char* a6 = "number six";
  silc_obj o6 = silc_int_mem_alloc(m, strlen(a6), a6, SILC_TYPE_BREF, 106);

  silc_obj a7[] = { o6, o5 };
  silc_obj o7 = silc_int_mem_alloc(m, 2, a7, SILC_TYPE_CONS, SILC_INT_MEM_CONS_SUBTYPE);

  silc_obj a8[] = { o1, o6, o7 };
  silc_obj o8 = silc_int_mem_alloc(m, countof(a8), a8, SILC_TYPE_OREF, 108);

  silc_obj objs[] = { o1, o2, o3, o4, o5, o6, o7, o8 };
  for (int i = 0; i < countof(objs); ++i) {
    silc_obj* obj_content = NULL;
    char* char_content = NULL;
    int len = 0;
    int subtype = silc_int_mem_parse_ref(m, objs[i], &len, &char_content, &obj_content);
    assert(subtype >= 0);
    assert((len > 0) && (char_content != NULL || obj_content != NULL));
  }

  /* check statistics */
  struct silc_mem_stats_t stats = {0};

  silc_int_mem_calc_stats(m, &stats);

  assert(8 == stats.pos_count); /* 3 objects allocated in total */
  assert(MEM_SIZE == stats.total_memory);
  assert(stats.total_memory > stats.free_memory);
  assert(stats.free_memory == stats.usable_memory);

  /* trigger gc and recheck statistics */
  pgc.root_obj_arr[0] = o1;
  pgc.root_obj_arr[1] = o7;
  silc_int_mem_gc(m);

  silc_int_mem_calc_stats(m, &stats);
  assert(7 == stats.pos_count);
  assert(2 == stats.free_pos_count); /* 7-2 == 5: o7, o6, o5, o4, o1 should be reachable */
  assert(MEM_SIZE == stats.total_memory);

  silc_obj objs2[] = { o7, o6, o5, o4, o1 };
  for (int i = 0; i < countof(objs2); ++i) {
    silc_obj* obj_content = NULL;
    char* char_content = NULL;
    int len = 0;
    int subtype = silc_int_mem_parse_ref(m, objs2[i], &len, &char_content, &obj_content);
    assert(subtype >= 0);
    assert((len > 0) && (char_content != NULL || obj_content != NULL));
  }

  /* cleanup test objects */
  silc_int_mem_free(m);
END_TEST_METHOD()


int main(int argc, char** argv) {
  TESTS_STARTED();
  test_get_initial_statistics();
  test_alloc_cons();
  test_alloc_oref();
  test_alloc_bref();
  test_gc_full_cleanup();
  test_gc_partial_cleanup();
  TESTS_SUCCEEDED();
  return 0;
}
