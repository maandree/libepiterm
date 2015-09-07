# Copying and distribution of this file, with or without modification,
# are permitted in any medium without royalty provided the copyright
# notice and this notice are preserved.  This file is offered as-is,
# without any warranty.


PREFIX = /usr
LIB = /lib
DATA = /share
INCLUDE = /include
LIBDIR = $(PREFIX)$(LIB)
DATADIR = $(PREFIX)$(DATA)
INCLUDEDIR = $(PREFIX)$(INCLUDE)
DOCDIR = $(DATADIR)/doc
INFODIR = $(DATADIR)/info
LICENSEDIR = $(DATADIR)/licenses
PKGCONFIGDIR = $(LIBDIR)/pkgconfig

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


EXT_LIBS = -lutempter
EXT_CFLAGS =

LIB_OBJ = hypoterm loop overlay pty shell



.PHONY: default
default: base

.PHONY: base
base: lib

.PHONY: all
all: lib test

.PHONY: lib
lib: so a pc

.PHONY: so
so: bin/libepiterm.so.$(LIB_VERSION) bin/libepiterm.so.$(LIB_MAJOR) bin/libepiterm.so

.PHONY: a
a: bin/libepiterm.a

.PHONY: pc
pc: bin/libepiterm.pc

.PHONY: test
test: bin/test

obj/libepiterm/%.o: src/libepiterm/%.c src/libepiterm/*.h src/libepiterm.h
	@mkdir -p obj/libepiterm/
	$(CC) $(FLAGS) -fPIC -c -o $@ $< $(EXT_CFLAGS) $(CPPFLAGS) $(CFLAGS)

bin/libepiterm.so.$(LIB_VERSION): $(foreach O,$(LIB_OBJ),obj/libepiterm/$(O).o)
	@mkdir -p bin
	$(CC) $(FLAGS) $(LDOPTIMISE) -shared -Wl,-soname,libepiterm.so.$(LIB_MAJOR) -o $@ $^ $(EXT_LIBS) $(LDFLAGS)

bin/libepiterm.so.$(LIB_MAJOR):
	@mkdir -p bin
	ln -sf libepiterm.so.$(LIB_VERSION) $@

bin/libepiterm.so:
	@mkdir -p bin
	ln -sf libepiterm.so.$(LIB_VERSION) $@

bin/libepiterm.a: $(foreach O,$(LIB_OBJ),obj/libepiterm/$(O).o)
	@mkdir -p bin
	ar rcs $@ $^

bin/libepiterm.pc: src/libepiterm.pc.in
	@mkdir -p bin
	cp $< $@
	sed -i 's:@LIBDIR@:$(LIBDIR):g' $@
	sed -i 's:@INCLUDEDIR@:$(INCLUDEDIR):g' $@
	sed -i 's:@VERSION@:$(LIB_VERSION):g' $@
	sed -i 's:@LIBS@:$(EXT_LIBS):g' $@
	sed -i 's:@CFLAGS@:$(EXT_CFLAGS):g' $@

bin/test: bin/test.o
	@mkdir -p bin
	$(CC) $(FLAGS) -o $@ $^ -Lbin -lepiterm $(EXT_LIBS) $(CPPFLAGS) $(CFLAGS)

bin/test.o: src/test.c src/libepiterm/*.h src/*.h
	@mkdir -p obj
	$(CC) $(FLAGS) -Isrc -c -o $@ $< $(EXT_CFLAGS) $(CPPFLAGS) $(CFLAGS)

.PHONY: install
install: install-base

.PHONY: install-all
install-all: install-base

.PHONY: install-base
install-base: install-lib install-copyright

.PHONY: install-lib
install-lib: install-h install-so install-a install-pc

.PHONY: install-copyright
install-copyright: install-copying install-license

.PHONY: install-h
install-h:
	install -dm755 -- "$(DESTDIR)$(INCLUDEDIR)"
	install -dm755 -- "$(DESTDIR)$(INCLUDEDIR)/libepiterm"
	install -m644 -- src/libepiterm.h "$(DESTDIR)$(INCLUDEDIR)/libepiterm.h"
	$(foreach H,$(LIB_OBJ),install -m644 -- src/libepiterm/$(H).h "$(DESTDIR)$(INCLUDEDIR)/libepiterm/$(H).h" &&) true

.PHONY: install-so
install-so: bin/libepiterm.so.$(LIB_VERSION)
	install -dm755 -- "$(DESTDIR)$(LIBDIR)"
	install -m755 bin/libepiterm.so.$(LIB_VERSION) -- "$(DESTDIR)$(LIBDIR)/libepiterm.so.$(LIB_VERSION)"
	ln -sf libepiterm.so.$(LIB_VERSION) -- "$(DESTDIR)$(LIBDIR)/libepiterm.so.$(LIB_MAJOR)"
	ln -sf libepiterm.so.$(LIB_VERSION) -- "$(DESTDIR)$(LIBDIR)/libepiterm.so"

.PHONY: install-a
install-a: bin/libepiterm.a
	install -dm755 -- "$(DESTDIR)$(LIBDIR)"
	install -m644 bin/libepiterm.a -- "$(DESTDIR)$(LIBDIR)/libepiterm.a"

.PHONY: install-pc
install-pc: bin/libepiterm.pc
	install -dm755 -- "$(DESTDIR)$(PKGCONFIGDIR)"
	install -m644 bin/libepiterm.pc -- "$(DESTDIR)$(PKGCONFIGDIR)/libepiterm.pc"

.PHONY: install-copying
install-copying:
	install -dm755 -- "$(DESTDIR)$(LICENSEDIR)/$(PKGNAME)"
	install -m644 COPYING -- "$(DESTDIR)$(LICENSEDIR)/$(PKGNAME)/COPYING"

.PHONY: install-license
install-license:
	install -dm755 -- "$(DESTDIR)$(LICENSEDIR)/$(PKGNAME)"
	install -m644 LICENSE -- "$(DESTDIR)$(LICENSEDIR)/$(PKGNAME)/LICENSE"

.PHONY: uninstall
uninstall:
	-rm -- "$(DESTDIR)$(INCLUDEDIR)/libepiterm.h"
	-rm -- $(foreach H,$(LIB_OBJ),"$(DESTDIR)$(INCLUDEDIR)/libepiterm/$(H).h")
	-rmdir -- "$(DESTDIR)$(INCLUDEDIR)/libepiterm"
	-rm -- "$(DESTDIR)$(LIBDIR)/libepiterm.so.$(LIB_VERSION)"
	-rm -- "$(DESTDIR)$(LIBDIR)/libepiterm.so.$(LIB_MAJOR)"
	-rm -- "$(DESTDIR)$(LIBDIR)/libepiterm.so"
	-rm -- "$(DESTDIR)$(LIBDIR)/libepiterm.a"
	-rm -- "$(DESTDIR)$(PKGCONFIGDIR)/libepiterm.pc"
	-rm -- "$(DESTDIR)$(LICENSEDIR)/$(PKGNAME)/COPYING"
	-rm -- "$(DESTDIR)$(LICENSEDIR)/$(PKGNAME)/LICENSE"
	-rmdir -- "$(DESTDIR)$(LICENSEDIR)/$(PKGNAME)"

.PHONY: clean
clean:
	-rm -rf obj bin

