CC = gcc
CFLAGS = -Wall -Wextra
DEBUG_FLAGS = -g -DDEBUG

# Default build is with debug flags
FINAL_CFLAGS = $(CFLAGS) $(DEBUG_FLAGS)

all: main

main:
	$(CC) $(FINAL_CFLAGS) main.c utils.c -o main

# Debug build with additional debug flags
debug: FINAL_CFLAGS += -ggdb3 -O0 -fsanitize=address
debug: main

# Run with gdb
gdb: debug
	gdb ./main

# Run with valgrind
valgrind: debug
	valgrind --leak-check=full \
	--show-leak-kinds=all \
	--track-origins=yes \
	--verbose \
	./main -p 4567

clean:
	rm -f main *.o

run:
	./main -p 4568

.PHONY: all clean run debug release gdb valgrind