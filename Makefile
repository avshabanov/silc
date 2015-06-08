
.PHONY: all tests silc clean compile

all: tests repl

repl: silc
	$(MAKE) -C src/repl

tests: silc
	$(MAKE) -C test

silc:
	$(MAKE) -C src/silc

compile:
	$(MAKE) -C src/silc compile
	$(MAKE) -C test compile

clean:
	$(MAKE) -C test clean
	$(MAKE) -C src/silc clean
	$(MAKE) -C src/repl clean

