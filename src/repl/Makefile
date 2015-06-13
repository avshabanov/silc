include ../../target/config.mk
include ../../silc.mk

CFLAGS += -I../silc
LFLAGS += ../silc/target/silc.a

.PHONY: clean compile

# Targets

all: compile

compile: target/silc

target/silc: $(TO)/repl.o
	$(LINKER) -o target/silc $(TO)/repl.o $(LFLAGS)


$(TO)/repl.o: target repl.c
	$(CC) $(CFLAGS) -c repl.c -o $(TO)/repl.o


# Aux targets

target:
	mkdir -p $(TO)

clean:
	rm -rf target

