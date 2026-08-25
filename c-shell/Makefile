CC = gcc

CFLAGS = -Wall -Wextra -std=c2x -Iinclude -g -D_POSIX_C_SOURCE=200809L
LDLIBS = -lm

TARGET = cshell

SRC = src/main.c \
      src/shell.c \
      src/lexer.c \
      src/parser.c \
      src/execution.c \
      src/builtins.c

OBJ = $(SRC:.c=.o)

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJ) $(LDLIBS)

src/%.o: src/%.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJ) $(TARGET)

.PHONY: clean