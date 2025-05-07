# Makefile

# Toolchain
CC      := gcc
CFLAGS  := -std=c17 -Wall -Wextra -g -Iinclude

# Linker flags: math lib + define __ImageBase
LDFLAGS := -lm -Wl,--defsym,__ImageBase=0

# Directories
SRC_DIR := src
OBJ_DIR := obj
BIN     := simulate_cfs

# Sources and objects
SRCS    := $(wildcard $(SRC_DIR)/*.c)
OBJS    := $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(SRCS))

.PHONY: all clean run

# Default target
all: $(BIN)

# Link step: objects → binary
$(BIN): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

# Compile each .c → obj/%.o
# Ensures obj/ exists first
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Create obj directory if missing
$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

# Clean up everything
clean:
	rm -rf $(OBJ_DIR) $(BIN)

# Run with a sample testcase (adjust path as needed)
run: all
	./$(BIN) testcase/test02.txt
