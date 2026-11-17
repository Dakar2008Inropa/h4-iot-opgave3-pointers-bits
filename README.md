# IoT H4 — Opgave 3: Pointere og bit-operationer

Program til **ATmega328PB Xplained Mini**, der via UART (Hercules) lader dig pege på en registeradresse, vælge én bit (0–7) og sætte den til 0 eller 1 — uden at røre de andre bits.

## Opgaven kort

Fra PC’en sender du tre ting i rækkefølge:

1. En **hex-adresse** afsluttet med `:` (fx `0025:` eller `0x0025:`)
2. En **bit-position** `0`–`7`
3. En **bit-værdi** `0` eller `1`

Programmet bruger en **pointer** til adressen og ændrer kun den valgte bit. Derefter printer det fire statuslinjer på UART.

## Sådan virker løsningen

### 1) Tilstandsmaskine (enum)

Programmet er altid i præcis én tilstand:

| Tilstand | Hvad forventes |
|----------|----------------|
| `Modtag_Adresse` | Hex-cifre + `:` |
| `Modtag_Bit_Position_I_Adresse` | Ét ciffer `0`–`7` |
| `Modtag_Bit_Positon_Vaerdi` | `0` eller `1` |

Tilstandene styres med en `enum` i `main.c`. Ved **hvert** skift skrives tilstandsnavnet på UART, så du i Hercules kan se, hvad du skal taste næste gang.

Ugyldigt input i trin 2 eller 3 sender programmet tilbage til `Modtag_Adresse` **uden** at ændre registret.

### 2) UART med interrupt + buffer

- USART0 kører **9600 baud, 8N1** (samme som board-USB / mEDBG).
- **RX-interrupt** lægger hvert tegn i en lille ringbuffer (kort ISR).
- **main-loop** henter tegn fra bufferen og kører tilstandsmaskinen.
- Al `printf` sker i main — ikke inde i interrupt.

Det betyder: tegnene går ikke tabt, mens vi printer, og interrupt-koden forbliver kort.

### 3) Pointer til registeradressen

Når adressen er parse’t (fx `0x0025` = PORTB), sættes:

```c
register_pointer = (volatile uint8_t *)received_address;
```

Derefter betyder `*register_pointer` “værdien i det register”. Det er den centrale pointe i opgaven: vi skriver ikke `PORTB = ...` hardkodet — vi går via den adresse, PC’en sendte.

### 4) Bit-operationer (kun én bit)

```c
if (bit_value == 1)
    *register_pointer |=  (1 << bit_position);   /* sæt bit høj  */
else
    *register_pointer &= ~(1 << bit_position);   /* clear bit    */
```

- `|=` sætter valgte bit til 1, resten uændret
- `&= ~` sætter valgte bit til 0, resten uændret

**Bemærk:** I opgavens pseudokode er 0/1 byttet om. Vi følger **teksten** i opgaven: `1` = høj, `0` = lav.

### 5) Statusprint efter succes

```text
Adresse: 0x0025
Bit position: 5
Bit vaerdi: 1
Bit moenster: 0010 0000
```

Bitmønsteret er hele byten på adressen efter ændringen (mellemrum mellem de to nibbles).

## Eksempel: tænd board-LED (PB5)

Boardets LED sidder typisk på **PORTB bit 5**.

Først gør pinnen til output (DDRB = `0x0024`):

```text
0024:
5
1
```

Derefter sæt port-bitten (PORTB = `0x0025`):

```text
0025:
5
1
```

Sluk igen:

```text
0025:
5
0
```

Adresserne kommer fra ATmega328PB-databladet (I/O memory map).

## Test med Hercules

1. Flash programmet til Xplained Mini.
2. Åbn Hercules Serial → boardets COM-port → **9600**, 8N1.
3. Du skal se starttekst + `Tilstand: Modtag_Adresse`.
4. Send kommandoerne ovenfor (ét tegn ad gangen eller hele adressen med `:`).
5. Tjek at tilstandsbeskeder og de fire statuslinjer stemmer.
6. Prøv bevidst ugyldigt input (fx bit `9` eller værdi `2`) og se at I lander tilbage i `Modtag_Adresse` uden ændring.

## Filer

| Fil | Indhold |
|-----|---------|
| `src/main.c` | Enum, tilstandsmaskine, hex-parse, pointer, bit-ops, print |
| `src/Uart.c` | USART0 init, RX-interrupt + ringbuffer, `printf` |
| `include/Uart.h` | UART API |
| `include/ProjectDefines.h` | `F_CPU`, AVR-includes |

## Byg / flash

Projektet er lavet til **VS Code + MPLAB / XC8** på ATmega328PB (samme toolchain som de øvrige H4 IoT-øvelser). Tilføj `src/main.c` og `src/Uart.c` i dit MPLAB/VS Code-projekt, build, og flash via board-USB.

## Kort “mundtlig” forklaring

Hvis du skal forklare løsningen højt:

1. Vi har en **state machine** med tre trin styret af en enum.
2. Tegn kommer ind via **UART-interrupt** til en buffer; main behandler dem.
3. Adressen bliver til en **pointer**, så vi kan skrive direkte i registret.
4. Med `|=` / `&= ~` ændrer vi **kun én bit**.
5. UART fortæller hele tiden hvilken tilstand vi er i, og printer resultatet bagefter.
