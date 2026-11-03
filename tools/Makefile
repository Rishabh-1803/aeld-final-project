CC ?= cc
CFLAGS ?= -std=c11 -O2 -Wall -Wextra -Wpedantic -Werror

TARGET := morse-ref-decoder
SOURCE := morse_ref_decoder.c

.PHONY: all test clean

all: $(TARGET)

$(TARGET): $(SOURCE)
	$(CC) $(CFLAGS) $(SOURCE) -o $(TARGET)

test: $(TARGET)
	./$(TARGET) --map ../config/morse_map.csv --self-test

clean:
	rm -f $(TARGET)
