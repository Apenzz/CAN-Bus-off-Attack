CC      = gcc
CFLAGS  = -Wall -Wextra -std=c11 -Isrc
LDFLAGS =

SRC     = src/main.c src/ecu.c src/can_bus.c
TARGET  = bus_off_sim.out

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^

clean:
	rm -f $(TARGET)
