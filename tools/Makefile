CC ?= cc
CFLAGS ?= -std=c11 -O2 -Wall -Wextra -Wpedantic -Werror

TARGET := morse-ref-decoder
SOURCES := morse_ref_decoder.c morse_fsm.c morse_keymap.c morse_uinput.c
HEADERS := morse_fsm.h morse_keymap.h morse_uinput.h

KEYMAP_TEST := test-morse-keymap
KEYMAP_SOURCES := morse_keymap.c morse_keymap_test.c
KEYMAP_HEADERS := morse_keymap.h

FSM_TEST := test-morse-fsm
FSM_SOURCES := morse_fsm.c morse_fsm_test.c
FSM_HEADERS := morse_fsm.h

.PHONY: all test clean

all: $(TARGET)

$(TARGET): $(SOURCES) $(HEADERS)
	$(CC) $(CFLAGS) $(SOURCES) -o $(TARGET)

$(KEYMAP_TEST): $(KEYMAP_SOURCES) $(KEYMAP_HEADERS)
	$(CC) $(CFLAGS) $(KEYMAP_SOURCES) -o $(KEYMAP_TEST)

$(FSM_TEST): $(FSM_SOURCES) $(FSM_HEADERS)
	$(CC) $(CFLAGS) $(FSM_SOURCES) -o $(FSM_TEST)

test: $(TARGET) $(KEYMAP_TEST) $(FSM_TEST)
	./$(TARGET) --map ../config/morse_map.csv --self-test
	./$(KEYMAP_TEST)
	./$(FSM_TEST)

clean:
	rm -f $(TARGET) $(KEYMAP_TEST) $(FSM_TEST)
