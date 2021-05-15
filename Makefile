CC       := clang
UNIT     := ./unit
INSTALL  := install

INCLUDE  := include 
INCLUDE  := $(addprefix -I, $(INCLUDE))

CFLAGS   := -O3 

LIB_DIRS := libs/jrutil
LIB_DIRS := $(addprefix -L, $(LIB_DIRS))
LIBS     := jrutil
LIBS     := $(addprefix -l, $(LIBS))

BIN      := unit
SUBDIRS  := deps/jrutil
TESTDIR  := tests

BUILDDIR := build
SRCDIR   := src
CSRC     := $(wildcard $(SRCDIR)/*.c)
OBJS     := $(patsubst $(SRCDIR)/%.c, $(BUILDDIR)/%.o, $(CSRC))

.PHONY: all
all: $(SUBDIRS) $(BIN) #test

$(BIN): $(OBJS)
	$(CC) $^ $(CFLAGS) $(LIB_DIRS) $(LIBS) -o $@
	strip $@

.PHONY: $(SUBDIRS)
$(SUBDIRS):
	$(MAKE) -C $@

.PHONY: test
test: build-tests $(UNIT)
	@$(UNIT) $(TESTDIR)/bin

.PHONY: build-tests
build-tests: $(SUBDIRS) $(TESTDIR)
	$(MAKE) test -C $<

$(BUILDDIR)/%.o: $(SRCDIR)/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(INCLUDE) -c $< -o $@

.PHONY: install
install: $(BIN)
	$(INSTALL) $(BIN) /usr/local/bin

.PHONY: uninstall
uninstall:
	$(RM) /usr/local/bin/$(BIN)

.PHONY: clean
clean:
	$(RM) $(BIN)
	$(RM) $(BUILDDIR)/*.o
	$(MAKE) clean -C tests
	$(MAKE) clean -C deps/jrutil
