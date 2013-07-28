NAME=sfqtester

CC := colorgcc
LD := gcc
CFLAGS := -Wall -Wextra -pedantic -g
LDLIBS := pthread stdc++ rt

SRC := $(shell find src/ -type f -regextype posix-extended -regex ".+\.(c|cpp)")
HDR := $(shell find src/ -type f -regextype posix-extended -regex ".+\.h")
ALL := $(SRC) $(HDR) README.md Makefile
DEF := BUFFER_SIZE=2048 BACKLOG=3
OBJ := $(filter %.o,$(SRC:src/%.c=build/c_%.o)) $(filter %.o,$(SRC:src/%.cpp=build/cpp_%.o))

.PHONY: $(NAME) all clean realclean todo
all: $(NAME)

$(NAME): $(OBJ)
	$(LD) -o $@ $^ $(addprefix -l,$(LDLIBS:-l%=%))

clean:
	-$(RM) $(OBJ)

realclean: clean
	$(RM) $(NAME)

todo:
	-@for file in $(ALL:Makefile=); do \
		fgrep -H -e TODO -e FIXME $$file; \
	done; true

build/c_%.o: src/%.c
	-@mkdir -p build
	$(CC) -std=gnu99 -x c $(CFLAGS) -o $@ -c $< $(addprefix -D,$(DEF))

build/cpp_%.o: src/%.cpp
	-@mkdir -p build
	$(CC) -std=gnu++98 -x c++ $(CFLAGS) -o $@ -c $< $(addprefix -D,$(DEF))
