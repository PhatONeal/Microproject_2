#include <xc.h>
#include <stdio.h>
#include "OLED_Libreria.h"

void I2C_Init(void) {
    SSPSTAT = 0x80; 
    SSPCON1 = 0x28; // I2C Master Mode
    SSPADD = 19;    // Baud rate para 100kHz a 8MHz
}

void OLED_Comando(unsigned char cmd) {
    SSPCON2bits.SEN = 1; while(SSPCON2bits.SEN);
    SSPBUF = 0x78; while(SSPSTATbits.BF || SSPCON2bits.ACKSTAT);
    SSPBUF = 0x00; while(SSPSTATbits.BF || SSPCON2bits.ACKSTAT);
    SSPBUF = cmd;  while(SSPSTATbits.BF || SSPCON2bits.ACKSTAT);
    SSPCON2bits.PEN = 1; while(SSPCON2bits.PEN);
}

void OLED_Dato(unsigned char dato) {
    SSPCON2bits.SEN = 1; while(SSPCON2bits.SEN);
    SSPBUF = 0x78; while(SSPSTATbits.BF || SSPCON2bits.ACKSTAT);
    SSPBUF = 0x40; while(SSPSTATbits.BF || SSPCON2bits.ACKSTAT);
    SSPBUF = dato; while(SSPSTATbits.BF || SSPCON2bits.ACKSTAT);
    SSPCON2bits.PEN = 1; while(SSPCON2bits.PEN);
}

void OLED_Init(void) {
    OLED_Comando(0xAE); // Display OFF
    OLED_Comando(0x8D); OLED_Comando(0x14); // Charge pump ON
    OLED_Comando(0xAF); // Display ON
}

void OLED_SetCursor(unsigned char pag, unsigned char col) {
    // PARCHE CRÍTICO PARA CONTROLADOR SH1106 (Pantallas de 1.3")
    col += 2; 
    
    OLED_Comando(0xB0 + pag);
    OLED_Comando(0x00 + (col & 0x0F));
    OLED_Comando(0x10 + ((col >> 4) & 0x0F));
}

void OLED_String(unsigned char pag, unsigned char col, char *str) {
    OLED_SetCursor(pag, col);
    while(*str) OLED_Dato(*str++);
}

void OLED_Clear(void) {
    for(unsigned char p = 0; p < 8; p++) {
        OLED_SetCursor(p, 0);
        // Ajuste a 132 columnas para borrar completamente la RAM del SH1106
        for(unsigned char i = 0; i < 132; i++) OLED_Dato(0x00); 
    }
}

void OLED_Int(unsigned char pag, unsigned char col, unsigned int numero) {
    char buffer[6];
    sprintf(buffer, "%u", numero);
    OLED_String(pag, col, buffer);
}