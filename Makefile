CC       := clang
UNIT     := ./unit
INSTALL  := install

INCLUDE  := include deps/jrutil/include
INCLUDE  := $(addprefix -I, $(INCLUDE))
CFLAGS   := -O3 $(INCLUDE)

LIB_DIRS := deps/jrutil/
LIB_DIRS := $(addprefix -L, $(LIB_DIRS))
LIBS     := jrutil
LIBS     := $(addprefix -l, $(LIBS))

BIN      := unit
SUBDIRS  := tests deps/jrutil

BUILDDIR := build
SRCDIR   := src
CSRC     := $(wildcard $(SRCDIR)/*.c)
OBJS     := $(patsubst $(SRCDIR)/%.c, $(BUILDDIR)/%.o, $(CSRC))

TESTDIR  := tests/bin

.PHONY: all
all: $(SUBDIRS) $(BIN)

$(BIN): $(OBJS)
	$(CC) $^ $(CFLAGS) $(LIB_DIRS) $(LIBS) -o $@

.PHONY: $(SUBDIRS)
$(SUBDIRS):
	$(MAKE) -C $@

.PHONY: test
test:
	@$(UNIT) $(TESTDIR)

$(BUILDDIR)/%.o: $(SRCDIR)/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

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
