CC = avr-gcc
CFLAGS = -std=c99 -Os -Wall -Werror
BIN = ./bin/
SOURCE = ./src/
HEADER = /usr/share/arduino/hardware/arduino/cores/arduino
NAMES = hub_mega encoder_reader encoder_reader_m mini_sub backup

LIST = $(patsubst %, $(BIN)/%, NAMES)

all: $(LIST)

$(BIN)/%: $(SOURCE)%.c
	$(CC) $< $(CFLAGS) -c $(HEADER) -o $@ $(LIBS)
