CC := gcc
CFLAGS := -std=c11 -Wall -Wextra -Wpedantic -O2 -Iinclude
TARGET := minios_tm
SOURCES := src/main.c src/process_mgr.c

all: $(TARGET)

$(TARGET): $(SOURCES) include/task.h
	$(CC) $(CFLAGS) $(SOURCES) -o $@

test: $(TARGET)
	bash tests/test_phase1.sh

clean:
	rm -f $(TARGET) *.o

.PHONY: all test clean
