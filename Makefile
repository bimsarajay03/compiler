CC = gcc
CFLAGS = -Wall -Wextra -Wpedantic -std=c11 -g
TARGET = parcer
SRC = main.c lexer.c parser.c semantics.c execute.c
OBJ = main.o lexer.o parser.o semantics.o execute.o

.PHONY: all debug run clean

all: debug

debug: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC)

run: $(TARGET)
	./$(TARGET) input.txt

clean:
	rm -f $(TARGET) $(OBJ)
