CC      = gcc
CFLAGS  = -Wall -Wextra -std=c11 -Iinclude -D_POSIX_C_SOURCE=200809L
LDFLAGS =

SRC     = src/main.c src/ecu.c src/can_bus.c
INCLUDE = include/ecu.h include/can_bus.h
TARGET  = bus_off_sim.out

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(SRC) $(INCLUDE)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $(SRC)

clean:
	rm -f $(TARGET)
