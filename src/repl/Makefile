
include ../../silc.mk

CFLAGS += -I../src/silc
LFLAGS += ../src/silc/target/silc.a

.PHONY: clean compile

# Targets

all: compile

compile: target/silc

target/silc: $(TO)/repl.o
	$(LINKER) -o target/repl $(TO)/repl.o $(LFLAGS)


$(TO)/repl.o: repl.c
	$(CC) $(CFLAGS) -c repl.c -o $(TO)/repl.o


# Aux targets

target:
	mkdir -p $(TO)

clean:
	rm -rf target

