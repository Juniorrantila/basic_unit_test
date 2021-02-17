CC			:=	clang
INCLUDE	:=
CFLAGS	:=	$(INCLUDE)
BUILDDIR	:=	build
SRCDIR	:=	src
TSRCDIR	:=	tests/src
CSRC		:=	$(wildcard $(SRCDIR)/*.c)
TSRC		:=	$(wildcard $(TSRCDIR)/*.c)
BIN		:=	a.out
TBIN		:= $(patsubst $(TSRCDIR)/%.test.c, tests/%.test, $(TSRC))
OBJS		:=	$(patsubst $(SRCDIR)/%.c, $(BUILDDIR)/%.o, $(CSRC))
TOBJS		:= $(filter-out $(BUILDDIR)/main.o, $(OBJS))

all: $(BIN)

.PHONY: test
test: $(TBIN)
	@./run_tests.sh

$(BIN): $(OBJS)
	$(CC) $(CFLAGS) $^ -o $@

$(TBIN): $(TOBJS)
	$(CC) $(CFLAGS) $^ $(patsubst tests/%.test, $(TSRCDIR)/%.test.c, $@) -o $@

$(BUILDDIR)/%.o: $(SRCDIR)/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

.PHONY: clean
clean:
	$(RM) $(BUILDDIR)/*.o
