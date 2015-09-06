# Copying and distribution of this file, with or without modification,
# are permitted in any medium without royalty provided the copyright
# notice and this notice are preserved.  This file is offered as-is,
# without any warranty.


PREFIX = /usr
LIB = /lib
DATA = /share
LIBDIR = $(PREFIX)$(LIB)
DATADIR = $(PREFIX)$(DATA)
DOCDIR = $(DATADIR)/doc
INFODIR = $(DATADIR)/info
LICENSEDIR = $(DATADIR)/licenses

PKGNAME = libepiterm


OPTIMISE = -Og -g

WARN = -Wall -Wextra -pedantic -Wdouble-promotion -Wformat=2 -Winit-self -Wmissing-include-dirs      \
       -Wtrampolines -Wmissing-prototypes -Wmissing-declarations -Wnested-externs                    \
       -Wno-variadic-macros -Wsync-nand -Wunsafe-loop-optimizations -Wcast-align                     \
       -Wdeclaration-after-statement -Wundef -Wbad-function-cast -Wwrite-strings -Wlogical-op        \
       -Wstrict-prototypes -Wold-style-definition -Wpacked -Wvector-operation-performance            \
       -Wunsuffixed-float-constants -Wsuggest-attribute=const -Wsuggest-attribute=noreturn           \
       -Wsuggest-attribute=format -Wnormalized=nfkc -Wshadow -Wredundant-decls -Winline -Wcast-qual  \
       -Wsign-conversion -Wstrict-overflow=5 -Wconversion -Wsuggest-attribute=pure -Wswitch-default  \
       -Wstrict-aliasing=1 -fstrict-overflow -Wfloat-equal -Wpadded -Waggregate-return               \
       -Wtraditional-conversion

FFLAGS = -fstrict-aliasing -fipa-pure-const -ftree-vrp -fstack-usage -funsafe-loop-optimizations

STD = c99

FLAGS = -std=$(STD) $(OPTIMISE) $(WARN) $(FFLAGS)


.PHONY: all
all: $(foreach O,hypoterm loop overlay pty shell,obj/libepiterm/$(O).o)

obj/libepiterm/%.o: src/libepiterm/%.c src/libepiterm/*.h
	@mkdir -p obj/libepiterm/
	$(CC) $(FLAGS) -c -o $@ $< $(CPPFLAGS) $(CFLAGS)


.PHONY: clean
clean:
	-rm -rf -- obj bin

