#include "../include/Uart.h"

#define RX_BUFFER_SIZE 32

static volatile char rx_buffer[RX_BUFFER_SIZE];
static volatile uint8_t rx_head = 0;
static volatile uint8_t rx_tail = 0;

static int uart_putch(char ch, FILE *stream)
{
    (void)stream;
    while (!(UCSR0A & (1 << UDRE0))) {
    }
    UDR0 = ch;
    return 0;
}

static FILE uart_output = FDEV_SETUP_STREAM(uart_putch, NULL, _FDEV_SETUP_WRITE);

void RS232Init(void)
{
    UBRR0L = (unsigned char)(MyUBRR & 0xff);
    UBRR0H = (unsigned char)(MyUBRR >> 8);
#ifdef UART_NORMAL_MODE
    UCSR0A = 0x00;
#else
    UCSR0A = (1 << U2X0);
#endif
    /* RX + TX + RX-interrupt (USART0) */
    UCSR0B = (1 << RXEN0) | (1 << TXEN0) | (1 << RXCIE0);
    UCSR0C = (1 << UCSZ01) | (1 << UCSZ00);

    stdout = &uart_output;
}

bool uart0_try_get_char(char *out)
{
    uint8_t head;
    uint8_t tail;

    if (out == NULL) {
        return false;
    }

    cli();
    head = rx_head;
    tail = rx_tail;
    if (head == tail) {
        sei();
        return false;
    }

    *out = rx_buffer[tail];
    rx_tail = (uint8_t)((tail + 1u) % RX_BUFFER_SIZE);
    sei();
    return true;
}

ISR(USART0_RX_vect)
{
    char received = UDR0;
    uint8_t next = (uint8_t)((rx_head + 1u) % RX_BUFFER_SIZE);

    /* Drop tegn hvis bufferen er fuld — ISR skal være kort. */
    if (next != rx_tail) {
        rx_buffer[rx_head] = received;
        rx_head = next;
    }
}
