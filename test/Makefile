
include ../target/config.mk
include ../silc.mk

CFLAGS += -I../src/silc
LFLAGS += ../src/silc/target/silc.a

TEST_DEPS = target test.h test_helpers.h

.PHONY: clean compile run_gc_tests

# Targets

all: compile
	target/test_inl && target/test_gc && target/test_obj && target/test_print && target/test_read && target/test_eval

run_gc_tests: compile
	target/test_gc

compile: target/test_gc target/test_inl target/test_print target/test_obj target/test_read target/test_eval

# Test Inline Objects

target/test_inl: $(TO)/test_inl.o
	$(LINKER) -o target/test_inl $(TO)/test_inl.o $(LFLAGS)

$(TO)/test_inl.o: $(TEST_DEPS) test_inl.c
	$(CC) $(CFLAGS) -c test_inl.c -o $(TO)/test_inl.o


# Test Print Functionality

target/test_print: $(TO)/test_print.o
	$(LINKER) -o target/test_print $(TO)/test_print.o $(LFLAGS)

$(TO)/test_print.o: $(TEST_DEPS) test_print.c
	$(CC) $(CFLAGS) -c test_print.c -o $(TO)/test_print.o

# Test Symbols

target/test_obj: $(TO)/test_obj.o
	$(LINKER) -o target/test_obj $(TO)/test_obj.o $(LFLAGS)

$(TO)/test_obj.o: $(TEST_DEPS) test_obj.c
	$(CC) $(CFLAGS) -c test_obj.c -o $(TO)/test_obj.o


# Test GC

target/test_gc: $(TO)/test_gc.o
	$(LINKER) -o target/test_gc $(TO)/test_gc.o $(LFLAGS)

$(TO)/test_gc.o: $(TEST_DEPS) test_gc.c
	$(CC) $(CFLAGS) -c test_gc.c -o $(TO)/test_gc.o

# Test Reader

target/test_read: $(TO)/test_read.o
	$(LINKER) -o target/test_read $(TO)/test_read.o $(LFLAGS)

$(TO)/test_read.o: $(TEST_DEPS) test_read.c
	$(CC) $(CFLAGS) -c test_read.c -o $(TO)/test_read.o

# Test Eval

target/test_eval: $(TO)/test_eval.o
	$(LINKER) -o target/test_eval $(TO)/test_eval.o $(LFLAGS)

$(TO)/test_eval.o: $(TEST_DEPS) test_eval.c
	$(CC) $(CFLAGS) -c test_eval.c -o $(TO)/test_eval.o


# Aux targets

target:
	mkdir -p $(TO)

clean:
	rm -rf target

