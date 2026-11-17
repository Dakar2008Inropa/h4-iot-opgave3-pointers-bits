#include "../include/ProjectDefines.h"
#include "../include/Uart.h"
#include <stdio.h>
#include <ctype.h>

/*
 * Opgave 3: UART-styret tilstandsmaskine der via en pointer
 * sætter/clearer én bit på en registeradresse.
 *
 * Flow fra PC (Hercules @ 9600):
 *   1) hex-adresse + ':'   fx  0025:
 *   2) bit-position 0-7    fx  5
 *   3) bit-værdi 0 eller 1 fx  1
 */

typedef enum {
    Modtag_Adresse = 0,
    Modtag_Bit_Position_I_Adresse,
    Modtag_Bit_Positon_Vaerdi
} State_t;

static State_t state = Modtag_Adresse;

/* Værdier indsamlet gennem de tre tilstande */
static uint16_t received_address = 0;
static uint8_t bit_position = 0;
static uint8_t bit_value = 0;
static volatile uint8_t *register_pointer = NULL;

/* Hex-buffer mens vi er i Modtag_Adresse */
static char address_hex[8];
static uint8_t address_hex_len = 0;

static const char *state_name(State_t s)
{
    switch (s) {
    case Modtag_Adresse:
        return "Modtag_Adresse";
    case Modtag_Bit_Position_I_Adresse:
        return "Modtag_Bit_Position_I_Adresse";
    case Modtag_Bit_Positon_Vaerdi:
        return "Modtag_Bit_Positon_Vaerdi";
    default:
        return "?";
    }
}

static void set_state(State_t next)
{
    state = next;
    printf("Tilstand: %s\r\n", state_name(state));
}

static void reset_address_input(void)
{
    address_hex_len = 0;
    address_hex[0] = '\0';
    received_address = 0;
    register_pointer = NULL;
}

static int hex_nibble(char c)
{
    if (c >= '0' && c <= '9') {
        return c - '0';
    }
    c = (char)tolower((unsigned char)c);
    if (c >= 'a' && c <= 'f') {
        return 10 + (c - 'a');
    }
    return -1;
}

static bool parse_hex_address(const char *text, uint16_t *out_addr)
{
    uint16_t value = 0;
    uint8_t digits = 0;

    if (text[0] == '0' && (text[1] == 'x' || text[1] == 'X')) {
        text += 2;
    }

    while (*text != '\0') {
        int nibble = hex_nibble(*text);
        if (nibble < 0) {
            return false;
        }
        value = (uint16_t)((value << 4) | (uint16_t)nibble);
        digits++;
        text++;
    }

    if (digits == 0 || digits > 4) {
        return false;
    }

    *out_addr = value;
    return true;
}

static void print_bit_pattern(uint8_t value)
{
    int i;

    /* Printer fx "0000 1111" — højeste bit først */
    for (i = 7; i >= 0; i--) {
        putchar((value & (1u << i)) ? '1' : '0');
        if (i == 4) {
            putchar(' ');
        }
    }
}

static void print_success_report(void)
{
    printf("Adresse: 0x%04X\r\n", received_address);
    printf("Bit position: %u\r\n", (unsigned)bit_position);
    printf("Bit vaerdi: %u\r\n", (unsigned)bit_value);
    printf("Bit moenster: ");
    print_bit_pattern(*register_pointer);
    printf("\r\n");
}

/*
 * Kun den valgte bit ændres.
 * 1 = sæt bit høj, 0 = clear bit (teksten i opgaven — ikke den byttede pseudokode).
 */
static void apply_bit_change(void)
{
    if (bit_value == 1u) {
        *register_pointer |= (uint8_t)(1u << bit_position);
    } else {
        *register_pointer &= (uint8_t)~(1u << bit_position);
    }
}

static void handle_address_char(char c)
{
    if (c == ':') {
        address_hex[address_hex_len] = '\0';
        if (!parse_hex_address(address_hex, &received_address)) {
            printf("Ugyldig adresse. Proev igen.\r\n");
            reset_address_input();
            set_state(Modtag_Adresse);
            return;
        }

        register_pointer = (volatile uint8_t *)(uintptr_t)received_address;
        address_hex_len = 0;
        set_state(Modtag_Bit_Position_I_Adresse);
        return;
    }

    if (hex_nibble(c) >= 0 || c == 'x' || c == 'X') {
        if (address_hex_len < (sizeof(address_hex) - 1u)) {
            address_hex[address_hex_len++] = c;
        }
        return;
    }

    /* Ignorer whitespace; andre tegn nulstiller adresse-indtastningen */
    if (c == ' ' || c == '\r' || c == '\n' || c == '\t') {
        return;
    }

    printf("Ugyldigt tegn i adresse. Start forfra.\r\n");
    reset_address_input();
    set_state(Modtag_Adresse);
}

static void handle_bit_position_char(char c)
{
    if (c >= '0' && c <= '7') {
        bit_position = (uint8_t)(c - '0');
        set_state(Modtag_Bit_Positon_Vaerdi);
        return;
    }

    if (c == ' ' || c == '\r' || c == '\n' || c == '\t') {
        return;
    }

    printf("Ugyldig bit-position. Tilbage til adresse.\r\n");
    reset_address_input();
    set_state(Modtag_Adresse);
}

static void handle_bit_value_char(char c)
{
    if (c == '0' || c == '1') {
        bit_value = (uint8_t)(c - '0');
        apply_bit_change();
        print_success_report();
        reset_address_input();
        set_state(Modtag_Adresse);
        return;
    }

    if (c == ' ' || c == '\r' || c == '\n' || c == '\t') {
        return;
    }

    printf("Ugyldig bit-vaerdi. Tilbage til adresse.\r\n");
    reset_address_input();
    set_state(Modtag_Adresse);
}

static void process_char(char c)
{
    switch (state) {
    case Modtag_Adresse:
        handle_address_char(c);
        break;
    case Modtag_Bit_Position_I_Adresse:
        handle_bit_position_char(c);
        break;
    case Modtag_Bit_Positon_Vaerdi:
        handle_bit_value_char(c);
        break;
    }
}

int main(void)
{
    char received;

    RS232Init();
    sei();

    printf("\r\n=== Opgave 3: Pointer + bit-operationer ===\r\n");
    printf("Send hex-adresse afsluttet med ':'  (fx 0025: eller 0x0025:)\r\n");
    printf("Derefter bit 0-7, derefter vaerdi 0 eller 1.\r\n");
    set_state(Modtag_Adresse);

    for (;;) {
        if (uart0_try_get_char(&received)) {
            process_char(received);
        }
    }
}
