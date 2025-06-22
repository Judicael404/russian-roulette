# Compiler and flags
CC = gcc
CFLAGS = -Wall -O2
LDFLAGS = -lX11 -lm -lgif

# Output
TARGET = roulette

# Source files
SRCS = main.c wheel.c ui.c dirlist.c shell_executor.c gif_handler.c
OBJS = $(SRCS:.c=.o)

# Default target
all: $(TARGET)

# Link the final executable
$(TARGET): $(OBJS)
	$(CC) -o $@ $^ $(LDFLAGS)

# Compile each .c file to .o
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Clean up build artifacts
clean:
	rm -f $(OBJS) $(TARGET)

.PHONY: all clean
