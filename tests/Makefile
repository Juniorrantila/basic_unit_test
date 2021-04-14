CC       := clang
UNIT     := unit

INCLUDE  := include ../include
CFLAGS   := $(addprefix -I, $(INCLUDE)) -O3

SRCDIR   := src
BUILDDIR := build
BINDIR   := bin

PSRCDIR  := ../src
PBUILDDIR:= ../build

CSRC     := $(wildcard $(SRCDIR)/*.c)
BIN      := $(patsubst $(SRCDIR)/%.test.c, $(BINDIR)/%.test.out, $(CSRC))
OBJS     := 

PSRC     := $(wildcard $(PSRCDIR)/*.c)
POBJS    := $(patsubst $(PSRCDIR)/%.c, $(PBUILDDIR)/%.o, $(PSRC))
POBJS    := $(filter-out $(PBUILDDIR)/main.o, $(POBJS))

all: $(BIN)

$(BIN): $(OBJS) $(POBJS)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $^ $(patsubst $(BINDIR)/%.test.out, $(SRCDIR)/%.test.c, $@) -o $@

$(BUILDDIR)/%.o: $(SRCDIR)/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $< -o $@

.PHONY: clean
clean:
	$(RM) $(BINDIR)/*.test.out
	$(RM) $(BINDIR)/logs/*.log
	$(RM) $(BUILDDIR)/*.o
	$(RM) *.out
