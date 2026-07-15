CC := gcc

SANITIZERS := -fsanitize=address,undefined

CFLAGS := -std=c23 -g -Wall -Wextra -Wpedantic -Wshadow -Wconversion -Wsign-conversion -Wcast-qual -Wwrite-strings \
          -Wformat=2 -Wundef -Wstrict-prototypes -Wmissing-prototypes -Wold-style-definition -Wimplicit-fallthrough \
          -Wduplicated-cond -Wduplicated-branches -Wlogical-op -Wcast-align -Wvla -fstack-protector-strong \
          -fno-omit-frame-pointer $(SANITIZERS)

LDFLAGS += $(SANITIZERS)
CPPFLAGS := $(addprefix -I,$(shell find include -type d))
CPPFLAGS += $(shell pkg-config --cflags x11)
LDLIBS += $(shell pkg-config --libs x11)

TARGET := bin/quire

SRC := $(shell find src -name '*.c')
OBJ := $(patsubst src/%.c,build/%.o,$(SRC))

.PHONY: all clean run

all: $(TARGET)

$(TARGET): $(OBJ)
	@mkdir -p $(dir $@)
	@$(CC) $(LDFLAGS) $(OBJ) -o $@ $(LDLIBS)

build/%.o: src/%.c
	@mkdir -p $(dir $@)
	@$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

run: all
	@./$(TARGET)

clean:
	@rm -rf build bin/quire