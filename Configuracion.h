#ifndef CONFIGURACION_H
#define CONFIGURACION_H

/*
 * =============================================================
 * Configuracion.h ? Bits de configuracion para PIC18F4550
 * Compilador : MPLAB XC8
 * Oscilador  : Interno 8 MHz (INTOSC)
 * Proyecto   : PIC18F4550 + OLED SSD1306 I2C
 * =============================================================
 */

/* ---- CONFIG1L ---- */
#pragma config PLLDIV   = 1        // Sin divisor PLL (no se usa USB)
#pragma config CPUDIV   = OSC1_PLL2// Divisor del CPU: Fosc/1

/* ---- CONFIG1H ---- */
#pragma config FOSC     = INTOSC_XT// Oscilador interno, XT en OSC1/OSC2
                                   // Con OSCCON = 0x72 -> 8 MHz

/* ---- CONFIG2L ---- */
#pragma config PWRT     = ON       // Power-up Timer activado (estabiliza voltaje)
#pragma config BOR      = ON       // Brown-out Reset activado
#pragma config BORV     = 3        // Voltaje de BOR: 2.0V

/* ---- CONFIG2H ---- */
#pragma config WDT      = OFF      // Watchdog Timer APAGADO
                                   // CRITICO: si esta ON reinicia el PIC
                                   // durante los delays del DHT11 / I2C

/* ---- CONFIG3H ---- */
#pragma config PBADEN   = OFF      // CRITICO PARA LA OLED:
                                   // PORTB inicia como DIGITAL, no analogico
#pragma config CCP2MX   = ON       // CCP2 en RC1 (default)
#pragma config MCLRE    = ON       // Pin MCLR activo (conectar a VCC via 10k)

/* ---- CONFIG4L ---- */
#pragma config LVP      = OFF      // Low-Voltage Programming desactivado
                                   // Libera el pin RB5 para uso general
#pragma config XINST    = OFF      // Extended Instruction Set APAGADO
                                   // Obligatorio para que XC8 funcione bien
#pragma config DEBUG    = OFF      // Depuracion por hardware desactivada

/* ---- CONFIG5L/5H/6L/6H/7L/7H ---- */
#pragma config CP0      = OFF      // Sin proteccion de codigo
#pragma config CP1      = OFF
#pragma config CPB      = OFF
#pragma config WRT0     = OFF      // Sin proteccion de escritura
#pragma config WRT1     = OFF
#pragma config WRTB     = OFF
#pragma config WRTC     = OFF
#pragma config EBTR0    = OFF
#pragma config EBTR1    = OFF
#pragma config EBTRB    = OFF

#endif /* CONFIGURACION_H */