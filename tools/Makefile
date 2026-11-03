CC ?= cc
CFLAGS ?= -std=c11 -O2 -Wall -Wextra -Wpedantic -Werror

TARGET := morse-ref-decoder
SOURCES := morse_ref_decoder.c morse_keymap.c morse_uinput.c
HEADERS := morse_keymap.h morse_uinput.h

KEYMAP_TEST := test-morse-keymap
KEYMAP_SOURCES := morse_keymap.c morse_keymap_test.c
KEYMAP_HEADERS := morse_keymap.h

.PHONY: all test clean

all: $(TARGET)

$(TARGET): $(SOURCES) $(HEADERS)
	$(CC) $(CFLAGS) $(SOURCES) -o $(TARGET)

$(KEYMAP_TEST): $(KEYMAP_SOURCES) $(KEYMAP_HEADERS)
	$(CC) $(CFLAGS) $(KEYMAP_SOURCES) -o $(KEYMAP_TEST)

test: $(TARGET) $(KEYMAP_TEST)
	./$(TARGET) --map ../config/morse_map.csv --self-test
	./$(KEYMAP_TEST)

clean:
	rm -f $(TARGET) $(KEYMAP_TEST)
