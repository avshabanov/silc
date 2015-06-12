
# Generic target directory for object files
TO			    =	target/obj

CFLAGS      = -Wall -Werror -Wimplicit -pedantic -std=c99
LFLAGS      = -lm


# Debug Options
CFLAGS      += -O0 -g
LFLAGS      += -g

# Release Options
#CFLAGS      += -O3 -DNDEBUG
#LFLAGS      += -Wl,--gc-sections

CC          = gcc
LINKER      = gcc
MAKE        = make
