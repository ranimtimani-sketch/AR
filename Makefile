CC ?= cc
CFLAGS ?= -Wall -Wextra -Wpedantic -std=c11
DBGFLAGS ?= -g -O0
TARGET := dots_boxes
SRC := main.c game.c bot.c

.PHONY: all clean debug run valgrind gdb

all: $(TARGET)

$(TARGET): $(SRC) game.h bot.h
	$(CC) $(CFLAGS) $(SRC) -o $(TARGET)

debug: CFLAGS += $(DBGFLAGS)
debug: clean $(TARGET)

run: $(TARGET)
	./$(TARGET)

valgrind: debug
	valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes ./$(TARGET)

gdb: debug
	gdb ./$(TARGET)

clean:
	rm -f $(TARGET)
