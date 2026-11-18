# Opgave 3 — Pointere og bit-operationer (ATmega328PB)

Det her er min løsning til Embedded Opgave 3.

Idéen er, at man fra PC’en kan sende en registeradresse, vælge hvilken bit man vil pille ved, og så sætte den bit til 0 eller 1. Programmet må kun ændre den ene bit — resten af registret skal være urørt.

Jeg bruger Serial Monitor til at snakke med boardet (9600 baud), ikke Hercules.

## Hvad programmet forventer

Man sender tre ting i rækkefølge:

1. Hex-adresse + `:` — fx `0025:` eller `0x0025:`
2. Bit-position — et tal mellem `0` og `7`
3. Bit-værdi — `0` eller `1`

Når det er gjort, skriver programmet fire linjer tilbage med adresse, bit-position, bit-værdi og hele bitmønsteret på adressen.

## Hvordan jeg har lavet det

### Tilstandsmaskine

Programmet kører som en state machine med tre tilstande (lavet med en `enum`):

- `Modtag_Adresse`
- `Modtag_Bit_Position_I_Adresse`
- `Modtag_Bit_Positon_Vaerdi` (stavet som i opgaven)

Den starter i `Modtag_Adresse`. Hver gang tilstanden skifter, printer jeg navnet ud, så man i Serial Monitor kan se, hvad man skal taste næste gang.

Hvis man sender noget ugyldigt i trin 2 eller 3, hopper den bare tilbage til `Modtag_Adresse` uden at ændre noget i registret.

### UART

Jeg bruger USART0 via boardets USB. Tegnene kommer ind i et RX-interrupt, som lægger dem i en lille buffer. Selve logikken kører i `main`, hvor jeg også printer med `printf`. På den måde holder interrupt’et kort, og jeg undgår at skrive langsomt UART-output inde i ISR’en.

### Pointer

Når adressen er parse’t, sætter jeg en pointer til den:

```c
register_pointer = (volatile uint8_t *)received_address;
```

Så kan jeg læse og skrive registret gennem pointeren i stedet for at hardcode fx `PORTB`. Det er hele pointen med den del af opgaven.

### Bit-operationer

```c
if (bit_value == 1)
    *register_pointer |=  (1 << bit_position);
else
    *register_pointer &= ~(1 << bit_position);
```

`|=` sætter bit’en høj. `&= ~` clearer den. De andre bits bliver som de er.

Lige en note: i opgavens pseudokode er 0 og 1 byttet om. Jeg følger teksten i opgaven i stedet — `1` betyder høj, `0` betyder lav.

## Sådan kan man teste det (LED på PB5)

Boardets LED sidder på PORTB bit 5. Først skal pinnen sættes som output via DDRB (`0x0024`), og derefter kan man tænde/slukke via PORTB (`0x0025`).

Sæt DDRB bit 5:

```text
0024:
5
1
```

Tænd LED:

```text
0025:
5
1
```

Sluk LED:

```text
0025:
5
0
```

Adresserne har jeg fundet i ATmega328PB-databladet.

## Serial Monitor

1. Flash til Xplained Mini
2. Åbn Serial Monitor på boardets COM-port, 9600 8N1
3. Der skulle gerne stå starttekst + `Tilstand: Modtag_Adresse`
4. Send kommandoerne som ovenfor
5. Tjek at tilstandsbeskederne og de fire statuslinjer kommer som forventet

## Filer

- `src/main.c` — state machine, parse af adresse, pointer og bit-skrivning
- `src/Uart.c` / `include/Uart.h` — UART init, interrupt og buffer
- `include/ProjectDefines.h` — `F_CPU` og includes

Bygget med VS Code + MPLAB / XC8 til ATmega328PB.
