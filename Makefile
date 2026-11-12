CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -Iinclude
TARGET = minios_tm
SRCS = src/main.c src/process_mgr.c

all: $(TARGET)

$(TARGET): $(SRCS) include/task.h
	$(CC) $(CFLAGS) $(SRCS) -o $(TARGET)

clean:
	rm -f $(TARGET)

.PHONY: all clean
