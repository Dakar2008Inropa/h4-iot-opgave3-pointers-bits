#ifndef UART_H_
#define UART_H_

#include "ProjectDefines.h"
#include <stdio.h>

#define BAUD 9600UL

#define UART_NORMAL_MODE
#ifdef UART_NORMAL_MODE
#define MyUBRR (unsigned int)(F_CPU / (16UL * BAUD) - 1UL)
#else
#define MyUBRR (unsigned int)(F_CPU / (8UL * BAUD) - 1UL)
#endif

void RS232Init(void);
bool uart0_try_get_char(char *out);

#endif
