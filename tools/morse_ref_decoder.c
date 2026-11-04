#define _POSIX_C_SOURCE 200809L

#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <linux/input.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include "morse_uinput.h"
#include "morse_fsm.h"

#define ASCII_MIN 32
#define ASCII_MAX 126
#define ASCII_COUNT (ASCII_MAX - ASCII_MIN + 1)
#define CSV_FIELD_COUNT 6
#define CSV_FIELD_CAP 256
#define CSV_LINE_CAP 2048
#define MAX_MORSE_LEN 63

static const char *const expected_header[CSV_FIELD_COUNT] = {
    "ascii_decimal", "ascii_hex", "character", "label", "morse", "encoding"
};

typedef struct {
    bool present[ASCII_COUNT];
    char codes[ASCII_COUNT][MAX_MORSE_LEN + 1];
    size_t count;
    size_t min_len;
    size_t max_len;
    size_t prefix_pairs;
} MorseMap;

typedef struct {
    char symbols[MAX_MORSE_LEN + 1];
    size_t length;
} MorseDecoder;

static void decoder_reset(MorseDecoder *decoder)
{
    decoder->length = 0;
    decoder->symbols[0] = '\0';
}

/* Parse a single RFC-4180-style CSV record, including quoted commas and "" escapes. */
static int parse_csv_record(const char *line,
                            char fields[CSV_FIELD_COUNT][CSV_FIELD_CAP])
{
    const char *cursor = line;
    size_t field_count = 0;

    while (*cursor != '\0' && *cursor != '\r' && *cursor != '\n') {
        size_t output_length = 0;

        if (field_count >= CSV_FIELD_COUNT)
            return -1;

        if (*cursor == '"') {
            bool closed = false;
            cursor++;

            while (*cursor != '\0') {
                if (*cursor == '"') {
                    if (cursor[1] == '"') {
                        if (output_length + 1 >= CSV_FIELD_CAP)
                            return -1;
                        fields[field_count][output_length++] = '"';
                        cursor += 2;
                    } else {
                        cursor++;
                        closed = true;
                        break;
                    }
                } else {
                    if (output_length + 1 >= CSV_FIELD_CAP)
                        return -1;
                    fields[field_count][output_length++] = *cursor++;
                }
            }

            if (!closed)
                return -1;
            if (*cursor != ',' && *cursor != '\r' && *cursor != '\n' && *cursor != '\0')
                return -1;
        } else {
            while (*cursor != '\0' && *cursor != ',' &&
                   *cursor != '\r' && *cursor != '\n') {
                if (output_length + 1 >= CSV_FIELD_CAP)
                    return -1;
                fields[field_count][output_length++] = *cursor++;
            }
        }

        fields[field_count][output_length] = '\0';
        field_count++;

        if (*cursor == ',') {
            cursor++;
            continue;
        }
        break;
    }

    while (*cursor == '\r' || *cursor == '\n')
        cursor++;

    if (*cursor != '\0' || field_count != CSV_FIELD_COUNT)
        return -1;

    return 0;
}

static bool parse_ascii_decimal(const char *text, int *value)
{
    char *end = NULL;
    long parsed;

    errno = 0;
    parsed = strtol(text, &end, 10);
    if (errno != 0 || end == text || *end != '\0' ||
        parsed < ASCII_MIN || parsed > ASCII_MAX)
        return false;

    *value = (int)parsed;
    return true;
}

static bool load_map(const char *path, MorseMap *map)
{
    FILE *file = NULL;
    char line[CSV_LINE_CAP];
    char fields[CSV_FIELD_COUNT][CSV_FIELD_CAP];
    size_t line_number = 0;
    size_t i;
    size_t j;

    memset(map, 0, sizeof(*map));
    map->min_len = MAX_MORSE_LEN + 1;

    file = fopen(path, "r");
    if (file == NULL) {
        fprintf(stderr, "ERROR: cannot open mapping file '%s': %s\n", path, strerror(errno));
        return false;
    }

    if (fgets(line, sizeof(line), file) == NULL) {
        fprintf(stderr, "ERROR: mapping file is empty: %s\n", path);
        fclose(file);
        return false;
    }
    line_number++;

    if (parse_csv_record(line, fields) != 0) {
        fprintf(stderr, "ERROR: malformed CSV header in %s\n", path);
        fclose(file);
        return false;
    }
    for (i = 0; i < CSV_FIELD_COUNT; i++) {
        if (strcmp(fields[i], expected_header[i]) != 0) {
            fprintf(stderr, "ERROR: unexpected CSV header in %s (column %zu)\n", path, i + 1);
            fclose(file);
            return false;
        }
    }

    while (fgets(line, sizeof(line), file) != NULL) {
        int ascii;
        char expected_hex[8];
        const char *morse;
        size_t morse_length;
        size_t index;

        line_number++;
        if (line[0] == '\n' || line[0] == '\r' || line[0] == '\0')
            continue;

        if (strchr(line, '\n') == NULL && !feof(file)) {
            fprintf(stderr, "ERROR: CSV line %zu exceeds the supported line length\n", line_number);
            fclose(file);
            return false;
        }

        if (parse_csv_record(line, fields) != 0) {
            fprintf(stderr, "ERROR: malformed CSV record at line %zu\n", line_number);
            fclose(file);
            return false;
        }

        if (!parse_ascii_decimal(fields[0], &ascii)) {
            fprintf(stderr, "ERROR: invalid printable ASCII value at line %zu\n", line_number);
            fclose(file);
            return false;
        }

        index = (size_t)(ascii - ASCII_MIN);
        if (map->present[index]) {
            fprintf(stderr, "ERROR: duplicate ASCII value %d at line %zu\n", ascii, line_number);
            fclose(file);
            return false;
        }

        if (strlen(fields[2]) != 1 || (unsigned char)fields[2][0] != (unsigned char)ascii) {
            fprintf(stderr, "ERROR: character field does not match ASCII %d at line %zu\n", ascii, line_number);
            fclose(file);
            return false;
        }

        (void)snprintf(expected_hex, sizeof(expected_hex), "0x%02X", ascii);
        if (strcmp(fields[1], expected_hex) != 0) {
            fprintf(stderr, "ERROR: hexadecimal value mismatch at line %zu (expected %s)\n",
                    line_number, expected_hex);
            fclose(file);
            return false;
        }

        if (fields[3][0] == '\0' || fields[5][0] == '\0') {
            fprintf(stderr, "ERROR: empty label or encoding description at line %zu\n", line_number);
            fclose(file);
            return false;
        }

        morse = fields[4];
        morse_length = strlen(morse);
        if (morse_length == 0 || morse_length > MAX_MORSE_LEN) {
            fprintf(stderr, "ERROR: invalid Morse sequence length at line %zu\n", line_number);
            fclose(file);
            return false;
        }
        for (j = 0; j < morse_length; j++) {
            if (morse[j] != '.' && morse[j] != '-') {
                fprintf(stderr, "ERROR: invalid Morse symbol at line %zu\n", line_number);
                fclose(file);
                return false;
            }
        }

        map->present[index] = true;
        memcpy(map->codes[index], morse, morse_length + 1);
        map->count++;
        if (morse_length < map->min_len)
            map->min_len = morse_length;
        if (morse_length > map->max_len)
            map->max_len = morse_length;
    }

    if (ferror(file)) {
        fprintf(stderr, "ERROR: read failure while loading %s\n", path);
        fclose(file);
        return false;
    }
    fclose(file);

    for (i = 0; i < ASCII_COUNT; i++) {
        if (!map->present[i]) {
            fprintf(stderr, "ERROR: mapping is missing ASCII value %zu\n", i + ASCII_MIN);
            return false;
        }
    }

    if (map->count != ASCII_COUNT) {
        fprintf(stderr, "ERROR: expected %d mapping rows, found %zu\n", ASCII_COUNT, map->count);
        return false;
    }

    for (i = 0; i < ASCII_COUNT; i++) {
        size_t len_i = strlen(map->codes[i]);
        for (j = i + 1; j < ASCII_COUNT; j++) {
            size_t len_j = strlen(map->codes[j]);
            if (strcmp(map->codes[i], map->codes[j]) == 0) {
                fprintf(stderr, "ERROR: Morse collision between ASCII %zu and %zu\n",
                        i + ASCII_MIN, j + ASCII_MIN);
                return false;
            }
            if (len_i < len_j && strncmp(map->codes[i], map->codes[j], len_i) == 0)
                map->prefix_pairs++;
            else if (len_j < len_i && strncmp(map->codes[j], map->codes[i], len_j) == 0)
                map->prefix_pairs++;
        }
    }

    return true;
}

static bool decoder_append(MorseDecoder *decoder, char symbol)
{
    if (symbol != '.' && symbol != '-')
        return false;
    if (decoder->length >= MAX_MORSE_LEN)
        return false;

    decoder->symbols[decoder->length++] = symbol;
    decoder->symbols[decoder->length] = '\0';
    return true;
}

static bool map_lookup(const MorseMap *map, const char *symbols, unsigned char *ascii)
{
    size_t i;

    for (i = 0; i < ASCII_COUNT; i++) {
        if (strcmp(map->codes[i], symbols) == 0) {
            *ascii = (unsigned char)(i + ASCII_MIN);
            return true;
        }
    }
    return false;
}

static bool decoder_commit(const MorseMap *map, MorseDecoder *decoder,
                           unsigned char *ascii)
{
    bool found = false;

    if (decoder->length != 0)
        found = map_lookup(map, decoder->symbols, ascii);

    /* Every commit consumes the current sequence, including invalid sequences. */
    decoder_reset(decoder);
    return found;
}

static void print_decoded(unsigned char ascii)
{
    printf("DECODED: ascii=%u hex=0x%02X char=", (unsigned int)ascii, (unsigned int)ascii);
    if (ascii == ' ') {
        puts("[SPACE]");
    } else if (ascii == '"') {
        puts("\\\"");
    } else if (ascii == '\\') {
        puts("\\\\");
    } else if (ascii == '\'') {
        puts("\\'");
    } else {
        printf("'%c'\n", (char)ascii);
    }
}

static bool run_self_test(const MorseMap *map)
{
    MorseDecoder decoder;
    unsigned char decoded = 0;
    size_t i;
    size_t j;
    size_t passed = 0;
    char invalid[MAX_MORSE_LEN + 1];

    decoder_reset(&decoder);

    for (i = 0; i < ASCII_COUNT; i++) {
        const char *expected = map->codes[i];
        decoder_reset(&decoder);

        for (j = 0; expected[j] != '\0'; j++) {
            if (!decoder_append(&decoder, expected[j])) {
                fprintf(stderr, "FAIL: could not buffer sequence for ASCII %zu\n", i + ASCII_MIN);
                return false;
            }
        }

        if (!decoder_commit(map, &decoder, &decoded) || decoded != (unsigned char)(i + ASCII_MIN)) {
            fprintf(stderr, "FAIL: decode/commit round-trip failed for ASCII %zu\n", i + ASCII_MIN);
            return false;
        }
        passed++;
    }

    /* An empty commit must be rejected and leave the decoder in the empty state. */
    decoder_reset(&decoder);
    if (decoder_commit(map, &decoder, &decoded)) {
        fputs("FAIL: empty COMMIT unexpectedly decoded a character\n", stderr);
        return false;
    }

    /* A sequence longer than every mapped code must be rejected deterministically. */
    if (map->max_len + 1 > MAX_MORSE_LEN) {
        fputs("FAIL: map is too long for the invalid-sequence self-test\n", stderr);
        return false;
    }
    memset(invalid, '.', map->max_len + 1);
    invalid[map->max_len + 1] = '\0';
    decoder_reset(&decoder);
    for (j = 0; invalid[j] != '\0'; j++) {
        if (!decoder_append(&decoder, invalid[j])) {
            fputs("FAIL: could not buffer invalid-sequence test\n", stderr);
            return false;
        }
    }
    if (decoder_commit(map, &decoder, &decoded)) {
        fputs("FAIL: unmapped sequence unexpectedly decoded\n", stderr);
        return false;
    }

    /* RESET must discard a partially entered character. */
    decoder_reset(&decoder);
    if (!decoder_append(&decoder, '.')) {
        fputs("FAIL: could not start reset test\n", stderr);
        return false;
    }
    decoder_reset(&decoder);
    if (decoder.length != 0 || decoder.symbols[0] != '\0') {
        fputs("FAIL: RESET did not clear the current sequence\n", stderr);
        return false;
    }

    printf("PASS: decoded and committed all %zu mapped ASCII characters.\n", passed);
    puts("PASS: empty COMMIT is rejected.");
    puts("PASS: an unmapped sequence is rejected and consumed.");
    puts("PASS: RESET discards a partially entered sequence.");
    printf("Map characteristics: %zu entries; sequence length min=%zu max=%zu; prefix pairs=%zu.\n",
           map->count, map->min_len, map->max_len, map->prefix_pairs);
    if (map->prefix_pairs != 0)
        puts("NOTE: the map is not prefix-free; explicit COMMIT is required to delimit characters.");
    return true;
}

static void run_interactive(const MorseMap *map)
{
    MorseDecoder decoder;
    char line[128];

    decoder_reset(&decoder);
    puts("MorseMouse reference decoder (mapping-only prototype)");
    puts("Enter '.', '-', DOT, DASH, COMMIT, RESET, or QUIT; one token per line.");
    puts("COMMIT is an explicit character boundary. This program does not read mouse events.");

    while (fgets(line, sizeof(line), stdin) != NULL) {
        char *token = line;
        char *end;
        size_t i;

        while (*token != '\0' && isspace((unsigned char)*token))
            token++;
        end = token + strlen(token);
        while (end > token && isspace((unsigned char)end[-1]))
            *--end = '\0';

        for (i = 0; token[i] != '\0'; i++)
            token[i] = (char)toupper((unsigned char)token[i]);

        if (strcmp(token, "QUIT") == 0 || strcmp(token, "EXIT") == 0)
            break;
        if (strcmp(token, "RESET") == 0) {
            decoder_reset(&decoder);
            puts("RESET: current sequence discarded.");
            continue;
        }
        if (strcmp(token, ".") == 0 || strcmp(token, "DOT") == 0) {
            if (!decoder_append(&decoder, '.')) {
                puts("ERROR: sequence buffer is full; current sequence reset.");
                decoder_reset(&decoder);
            } else {
                printf("BUFFER: %s\n", decoder.symbols);
            }
            continue;
        }
        if (strcmp(token, "-") == 0 || strcmp(token, "DASH") == 0) {
            if (!decoder_append(&decoder, '-')) {
                puts("ERROR: sequence buffer is full; current sequence reset.");
                decoder_reset(&decoder);
            } else {
                printf("BUFFER: %s\n", decoder.symbols);
            }
            continue;
        }
        if (strcmp(token, "COMMIT") == 0) {
            unsigned char ascii;
            char rejected[MAX_MORSE_LEN + 1];

            (void)snprintf(rejected, sizeof(rejected), "%s", decoder.symbols);
            if (decoder_commit(map, &decoder, &ascii)) {
                print_decoded(ascii);
            } else if (rejected[0] == '\0') {
                puts("ERROR: COMMIT ignored because the sequence is empty.");
            } else {
                printf("ERROR: unmapped Morse sequence '%s'; buffer cleared.\n", rejected);
            }
            continue;
        }

        puts("ERROR: unknown token. Use '.', '-', DOT, DASH, COMMIT, RESET, or QUIT.");
    }
}

#define BITS_PER_LONG (sizeof(unsigned long) * 8U)
#define BIT_WORD(bit) ((size_t)(bit) / BITS_PER_LONG)
#define BIT_MASK(bit) (1UL << ((size_t)(bit) % BITS_PER_LONG))
#define BIT_WORDS(max_bit) (((size_t)(max_bit) + 1U + BITS_PER_LONG - 1U) / BITS_PER_LONG)

static bool bit_is_set(const unsigned long *bits, unsigned int bit)
{
    return (bits[BIT_WORD(bit)] & BIT_MASK(bit)) != 0;
}

static bool sync_evdev_button_state(int fd, MorseFsm *fsm)
{
    unsigned long key_bits[BIT_WORDS(KEY_MAX)];

    memset(key_bits, 0, sizeof(key_bits));

    if (ioctl(fd, EVIOCGKEY(sizeof(key_bits)), key_bits) < 0)
        return false;

    morse_fsm_sync_buttons(
        fsm,
        bit_is_set(key_bits, BTN_LEFT),
        bit_is_set(key_bits, BTN_RIGHT));

    return true;
}

static bool verify_mouse_device(int fd, char *name, size_t name_capacity)
{
    unsigned long event_bits[BIT_WORDS(EV_MAX)];
    unsigned long key_bits[BIT_WORDS(KEY_MAX)];

    memset(event_bits, 0, sizeof(event_bits));
    memset(key_bits, 0, sizeof(key_bits));

    if (ioctl(fd, EVIOCGNAME(name_capacity), name) < 0) {
        perror("ERROR: EVIOCGNAME");
        return false;
    }

    if (ioctl(fd, EVIOCGBIT(0, sizeof(event_bits)), event_bits) < 0) {
        perror("ERROR: EVIOCGBIT event types");
        return false;
    }

    if (!bit_is_set(event_bits, EV_KEY)) {
        fprintf(stderr, "ERROR: '%s' does not advertise EV_KEY events.\n", name);
        return false;
    }

    if (ioctl(fd, EVIOCGBIT(EV_KEY, sizeof(key_bits)), key_bits) < 0) {
        perror("ERROR: EVIOCGBIT key codes");
        return false;
    }

    if (!bit_is_set(key_bits, BTN_LEFT) ||
        !bit_is_set(key_bits, BTN_RIGHT) ||
        !bit_is_set(key_bits, BTN_MIDDLE)) {
        fprintf(stderr,
                "ERROR: '%s' must advertise BTN_LEFT, BTN_RIGHT, and BTN_MIDDLE.\n",
                name);
        return false;
    }

    return true;
}

static void append_event_symbol(MorseDecoder *decoder, char symbol,
                                const char *button_name)
{
    if (!decoder_append(decoder, symbol)) {
        printf("INPUT: %s down -> %s, but sequence buffer is full; resetting.\n",
               button_name, symbol == '.' ? "DOT" : "DASH");
        decoder_reset(decoder);
        return;
    }

    printf("INPUT: %s down -> %s\n", button_name, symbol == '.' ? "DOT" : "DASH");
    printf("BUFFER: %s\n", decoder->symbols);
    fflush(stdout);
}


static bool commit_event(const MorseMap *map,
                         MorseDecoder *decoder,
                         MorseUinputKeyboard *keyboard,
                         bool enable_uinput)
{
    unsigned char ascii;
    char rejected[MAX_MORSE_LEN + 1];

    (void)snprintf(rejected, sizeof(rejected), "%s", decoder->symbols);

    if (decoder_commit(map, decoder, &ascii)) {
        puts("INPUT: BTN_MIDDLE down -> COMMIT");
        print_decoded(ascii);

        if (enable_uinput) {
            if (morse_uinput_emit_ascii(keyboard, ascii) < 0) {
                fprintf(stderr,
                        "ERROR: failed to emit ASCII %u: %s\n",
                        (unsigned int)ascii, strerror(errno));
                return false;
            }

            printf("UINPUT: emitted ASCII %u successfully.\n",
                   (unsigned int)ascii);
        }
    } else if (rejected[0] == '\0') {
        puts("INPUT: BTN_MIDDLE down -> COMMIT; "
             "ignored because sequence is empty.");
    } else {
        puts("INPUT: BTN_MIDDLE down -> COMMIT");
        printf("ERROR: unmapped Morse sequence '%s'; buffer cleared.\n",
               rejected);
    }

    fflush(stdout);
    return true;
}

static volatile sig_atomic_t stop_requested = 0;

static void handle_stop_signal(int signal_number)
{
    (void)signal_number;
    stop_requested = 1;
}

static int run_evdev(const MorseMap *map,
                     const char *device_path,
                     bool enable_uinput)
{
    int fd = -1;
    char device_name[256] = {0};
    MorseDecoder decoder;
    MorseFsm fsm;
    MorseUinputKeyboard keyboard;
    struct input_event events[16];
    bool dropping_events = false;
    bool keyboard_open = false;
    int result = EXIT_FAILURE;

    memset(&keyboard, 0, sizeof(keyboard));
    keyboard.fd = -1;

    fd = open(device_path, O_RDONLY | O_CLOEXEC);
    if (fd < 0) {
        fprintf(stderr, "ERROR: cannot open input device '%s': %s\n",
                device_path, strerror(errno));
        return EXIT_FAILURE;
    }

    if (!verify_mouse_device(fd, device_name, sizeof(device_name))) {
        goto cleanup;
    }

    if (enable_uinput) {
        if (morse_uinput_open(&keyboard) < 0) {
            fprintf(stderr, "ERROR: cannot create virtual keyboard: %s\n",
                    strerror(errno));
            goto cleanup;
        }

        keyboard_open = true;
    }

    decoder_reset(&decoder);
    morse_fsm_init(&fsm);

    printf("Listening to %s (%s).\n", device_path, device_name);
    puts("Left down = DOT; right down = DASH; middle down = COMMIT.");
    puts("Release events update button state; key-repeat events are ignored.");

    if (enable_uinput) {
        puts("Virtual keyboard output is ENABLED.");
        puts("Decoded characters will be sent through /dev/uinput.");
    } else {
        puts("Virtual keyboard output is disabled; decoded characters go to stdout.");
    }

    puts("Press Ctrl+C to stop.");
    fflush(stdout);

    for (;;) {
        ssize_t bytes = read(fd, events, sizeof(events));
        size_t count;
        size_t i;

        if (bytes < 0) {
            if (errno == EINTR) {
                if (stop_requested) {
                    result = EXIT_SUCCESS;
                    break;
                }
                continue;
            }

            fprintf(stderr, "ERROR: read from '%s' failed: %s\n",
                    device_path, strerror(errno));
            break;
        }

        if (bytes == 0) {
            fprintf(stderr, "ERROR: input device '%s' reached end of file.\n",
                    device_path);
            break;
        }

        if ((size_t)bytes % sizeof(events[0]) != 0) {
            fprintf(stderr,
                    "ERROR: received a partial input_event record from '%s'.\n",
                    device_path);
            break;
        }

        count = (size_t)bytes / sizeof(events[0]);

        for (i = 0; i < count; i++) {
            const struct input_event *event = &events[i];

            if (event->type == EV_SYN && event->code == SYN_DROPPED) {
                decoder_reset(&decoder);
                morse_fsm_resync_begin(&fsm);
                dropping_events = true;
                puts("WARNING: evdev queue overflow (SYN_DROPPED); "
                     "sequence reset; synchronizing button state at SYN_REPORT.");
                fflush(stdout);
                continue;
            }

            if (dropping_events) {
                if (event->type == EV_SYN && event->code == SYN_REPORT) {
                    if (!sync_evdev_button_state(fd, &fsm)) {
                        fprintf(stderr,
                                "ERROR: EVIOCGKEY resynchronization failed: %s\n",
                                strerror(errno));
                        goto cleanup;
                    }

                    dropping_events = false;
                    puts("INPUT: evdev button state synchronized.");
                    fflush(stdout);
                }
                continue;
            }

            if (event->type != EV_KEY)
                continue;

            MorseFsmButton button;
            MorseFsmAction action;

            if (event->code == BTN_LEFT) {
                button = MORSE_FSM_BUTTON_LEFT;
            } else if (event->code == BTN_RIGHT) {
                button = MORSE_FSM_BUTTON_RIGHT;
            } else if (event->code == BTN_MIDDLE) {
                button = MORSE_FSM_BUTTON_MIDDLE;
            } else {
                continue;
            }

            action = morse_fsm_handle(&fsm, button, event->value);

            switch (action) {
            case MORSE_FSM_ACTION_APPEND_DOT:
                append_event_symbol(&decoder, '.', "BTN_LEFT");
                break;

            case MORSE_FSM_ACTION_APPEND_DASH:
                append_event_symbol(&decoder, '-', "BTN_RIGHT");
                break;

            case MORSE_FSM_ACTION_COMMIT:
                if (!commit_event(map, &decoder, &keyboard, enable_uinput))
                    goto cleanup;
                break;

            case MORSE_FSM_ACTION_CANCEL:
                decoder_reset(&decoder);
                puts("INPUT: overlapping BTN_LEFT/BTN_RIGHT presses -> "
                     "sequence cleared; chord lockout active.");
                fflush(stdout);
                break;

            case MORSE_FSM_ACTION_NONE:
            case MORSE_FSM_ACTION_IGNORE:
            default:
                break;
            }
        }
    }

cleanup:
    if (keyboard_open) {
        if (morse_uinput_close(&keyboard) < 0) {
            fprintf(stderr, "ERROR: virtual keyboard cleanup failed: %s\n",
                    strerror(errno));
            result = EXIT_FAILURE;
        }
    }

    if (fd >= 0)
        close(fd);

    return result;
}

static void print_usage(const char *program)
{
    printf("Usage: %s [--map CSV_PATH] [--self-test | --interactive | --evdev DEVICE [--uinput]]\n", program);
    puts("  --map PATH       Mapping CSV (default: config/morse_map.csv)");
    puts("  --self-test      Round-trip all 95 entries and test boundary/error behavior");
    puts("  --interactive    Read DOT/DASH/COMMIT/RESET tokens from standard input");
    puts("  --evdev DEVICE   Read BTN_LEFT/BTN_RIGHT/BTN_MIDDLE events from a Linux evdev node");
    puts("  --uinput         Also emit decoded characters through a virtual keyboard");
    puts("  --help           Show this help");
}

int main(int argc, char **argv)
{
    const char *map_path = "config/morse_map.csv";
    bool self_test = false;
    bool interactive = false;
    const char *evdev_path = NULL;
    bool enable_uinput = false;
    MorseMap map;
    int i;

    for (i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--map") == 0) {
            if (i + 1 >= argc) {
                fputs("ERROR: --map requires a path\n", stderr);
                return EXIT_FAILURE;
            }
            map_path = argv[++i];
        } else if (strcmp(argv[i], "--self-test") == 0) {
            self_test = true;
        } else if (strcmp(argv[i], "--interactive") == 0) {
            interactive = true;
        } else if (strcmp(argv[i], "--evdev") == 0) {
            if (i + 1 >= argc) {
                fputs("ERROR: --evdev requires a device path\n", stderr);
                return EXIT_FAILURE;
            }
            evdev_path = argv[++i];
        } else if (strcmp(argv[i], "--uinput") == 0) {
            enable_uinput = true;
        } else if (strcmp(argv[i], "--help") == 0) {
            print_usage(argv[0]);
            return EXIT_SUCCESS;
        } else {
            fprintf(stderr, "ERROR: unknown argument '%s'\n", argv[i]);
            print_usage(argv[0]);
            return EXIT_FAILURE;
        }
    }

    if ((self_test ? 1 : 0) + (interactive ? 1 : 0) + (evdev_path != NULL ? 1 : 0) > 1) {
        fputs("ERROR: choose only one of --self-test, --interactive, or --evdev\n", stderr);
        return EXIT_FAILURE;
    }

    if (enable_uinput && evdev_path == NULL) {
        fputs("ERROR: --uinput requires --evdev DEVICE\n", stderr);
        return EXIT_FAILURE;
    }

    if (!load_map(map_path, &map))
        return EXIT_FAILURE;

    printf("Loaded %zu printable ASCII mappings from %s.\n", map.count, map_path);

    if (self_test)
        return run_self_test(&map) ? EXIT_SUCCESS : EXIT_FAILURE;

    if (evdev_path != NULL) {
        struct sigaction action;

        memset(&action, 0, sizeof(action));
        action.sa_handler = handle_stop_signal;
        sigemptyset(&action.sa_mask);

        if (sigaction(SIGINT, &action, NULL) < 0 ||
            sigaction(SIGTERM, &action, NULL) < 0) {
            perror("ERROR: sigaction");
            return EXIT_FAILURE;
        }

        stop_requested = 0;
        return run_evdev(&map, evdev_path, enable_uinput);
    }

    run_interactive(&map);
    return EXIT_SUCCESS;
}
