
include ../../target/config.mk
include ../../silc.mk

CFLAGS 	+= -fPIC

.PHONY: clean compile

# Targets

all: compile

compile: target/silc.a

target/silc.a: $(TO)/core.o $(TO)/builtins.o $(TO)/mem.o $(TO)/print.o $(TO)/read.o
	ar -rcs target/silc.a $(TO)/*.o

$(TO)/print.o: target silc.h mem.h print.c
	$(CC) $(CFLAGS) -c print.c -o $(TO)/print.o

$(TO)/read.o: target silc.h mem.h read.c
	$(CC) $(CFLAGS) -c read.c -o $(TO)/read.o

$(TO)/core.o: target silc.h mem.h core.c
	$(CC) $(CFLAGS) -c core.c -o $(TO)/core.o

$(TO)/builtins.o: target silc.h mem.h builtins.h builtins.c
	$(CC) $(CFLAGS) -c builtins.c -o $(TO)/builtins.o

$(TO)/mem.o: target silc.h mem.h mem.c
	$(CC) $(CFLAGS) -c mem.c -o $(TO)/mem.o

# Aux targets

target:
	mkdir -p $(TO)

clean:
	rm -rf target

