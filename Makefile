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

LIB_MAJOR = 1
LIB_MINOR = 0
LIB_VERSION = $(LIB_MAJOR).$(LIB_MINOR)


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


LIB_OBJ = hypoterm loop overlay pty shell


.PHONY: all
all: lib test

.PHONY: lib
lib: so a

.PHONY: so
so: bin/libepiterm.so.$(LIB_VERSION) bin/libepiterm.so.$(LIB_MAJOR) bin/libepiterm.so

.PHONY: a
a: bin/libepiterm.a

.PHONY: test
test: bin/test

obj/libepiterm/%.o: src/libepiterm/%.c src/libepiterm/*.h src/libepiterm.h
	@mkdir -p obj/libepiterm/
	$(CC) $(FLAGS) -fPIC -c -o $@ $< $(CPPFLAGS) $(CFLAGS)

bin/libepiterm.so.$(LIB_VERSION): $(foreach O,$(LIB_OBJ),obj/libepiterm/$(O).o)
	@mkdir -p bin
	$(CC) $(FLAGS) $(LDOPTIMISE) -shared -Wl,-soname,libepiterm.so.$(LIB_MAJOR) -o $@ $^ -lutempter $(LDFLAGS)

bin/libepiterm.so.$(LIB_MAJOR):
	@mkdir -p bin
	ln -sf libepiterm.so.$(LIB_VERSION) $@

bin/libepiterm.so:
	@mkdir -p bin
	ln -sf libepiterm.so.$(LIB_VERSION) $@

bin/libepiterm.a: $(foreach O,$(LIB_OBJ),obj/libepiterm/$(O).o)
	@mkdir -p bin
	ar rcs $@ $^

bin/test: bin/test.o
	@mkdir -p bin
	$(CC) $(FLAGS) -o $@ $^ -Lbin -lepiterm -lutempter $(CPPFLAGS) $(CFLAGS)

bin/test.o: src/test.c src/libepiterm/*.h src/*.h
	@mkdir -p obj
	$(CC) $(FLAGS) -Isrc -c -o $@ $< $(CPPFLAGS) $(CFLAGS)

.PHONY: clean
clean:
	-rm -rf -- obj bin

