#ifndef OLED_LIBRERIA_H
#define OLED_LIBRERIA_H

/*
 * =============================================================
 * OLED_libreria.h - Driver I2C + SSD1306 para PIC18F4550
 * Pines I2C  : RB1 = SCL  |  RB0 = SDA
 * =============================================================
 */

#include <xc.h>

#ifndef _XTAL_FREQ
    #define _XTAL_FREQ  8000000UL
#endif

#define I2C_BAUDRATE    19          /* 100 kHz @ 8 MHz */

#define OLED_ADDR       0x78
#define OLED_CMD        0x00
#define OLED_DATA       0x40
#define OLED_WIDTH      128
#define OLED_PAGES      8

/* Prototipos I2C */
void I2C_Init(void);
void I2C_Ready(void);
unsigned char I2C_Start(unsigned char addr);
unsigned char I2C_Write(unsigned char data);
void I2C_Stop(void);

/* Prototipos OLED */
void OLED_Init(void);
void OLED_Comando(unsigned char cmd);
void OLED_Dato(unsigned char dato);
void OLED_Clear(void);
void OLED_SetCursor(unsigned char pagina, unsigned char col);
void OLED_Char(unsigned char c);
void OLED_String(unsigned char pagina, unsigned char col, const char *texto);
void OLED_Int(unsigned char pagina, unsigned char col, unsigned int valor);
void OLED_BorrarTexto(unsigned char pagina, unsigned char col, unsigned char longitud);

#endif /* OLED_LIBRERIA_H */