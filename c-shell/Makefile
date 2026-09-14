CC = gcc

CFLAGS = -std=c23 -D_POSIX_C_SOURCE=200809L -D_XOPEN_SOURCE=700 \
         -Wall -Wextra -Werror -Wno-unused-parameter -fno-asm \
         -Iinclude -g
LDLIBS = -lm

TARGET = shell.out

SRC = src/main.c \
      src/shell.c \
      src/lexer.c \
      src/parser.c \
      src/execution.c \
      src/builtins.c \
      src/jobs.c

OBJ = $(SRC:.c=.o)

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJ) $(LDLIBS)

src/%.o: src/%.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJ) $(TARGET)

.PHONY: all clean
