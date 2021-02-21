CC       := clang
INCLUDE  := include
CFLAGS   := $(addprefix -I, $(INCLUDE)) -O3
BUILDDIR := build
SRCDIR   := src
TDIR		:= tests
TSRCDIR  := tests/src
CSRC     := $(wildcard $(SRCDIR)/*.c)
TSRC     := $(wildcard $(TSRCDIR)/*.c)
BIN      := unit
UNIT		:= ./unit
TBIN     := $(patsubst $(TSRCDIR)/%.test.c, tests/%.test.out, $(TSRC))
OBJS     := $(patsubst $(SRCDIR)/%.c, $(BUILDDIR)/%.o, $(CSRC))
TOBJS    := $(filter-out $(BUILDDIR)/main.o, $(OBJS))
INSTALL 	:= install

all: $(BIN)

install: $(BIN)
	$(INSTALL) $(BIN) /usr/local/bin

uninstall:
	$(RM) /usr/local/bin/$(BIN)

.PHONY: test
test: $(TBIN) $(UNIT)
	@$(UNIT) $(TDIR)

$(BIN): $(OBJS)
	$(CC) $(CFLAGS) $^ -o $@

$(TBIN): $(TOBJS)
	$(CC) $(CFLAGS) $^ $(patsubst tests/%.test.out, $(TSRCDIR)/%.test.c, $@) -o $@

$(BUILDDIR)/%.o: $(SRCDIR)/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

.PHONY: clean
clean:
	$(RM) $(BUILDDIR)/*.o
	$(RM) *.o
	$(RM) tests/*.out
	$(RM) *.log
	$(RM) $(TDIR)/*.log
