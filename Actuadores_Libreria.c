#include "Actuadores_Libreria.h"

void Actuadores_Init(void) {
    // 1. APAGAR MÓDULOS FANTASMA (Seguridad de manual)
    CMCON = 0x07;          // Apagar comparadores analógicos

    // 2. CONFIGURAR PUERTO B PARA LOS LEDS (RB2 a RB6)
    // Nota: RB0 y RB1 se dejan quietos porque los usa el I2C de la OLED
    TRISBbits.TRISB2 = 0; // LED 5
    TRISBbits.TRISB3 = 0; // LED 4
    TRISBbits.TRISB4 = 0; // LED 3
    TRISBbits.TRISB5 = 0; // LED 2
    TRISBbits.TRISB6 = 0; // LED 1

    // 3. CONFIGURAR BUZZER Y VENTILADOR
    TRISDbits.TRISD1 = 0; // Buzzer
    TRISCbits.TRISC1 = 0; // Ventilador PWM

    // 4. FORZAR ESTADO INICIAL APAGADO (0V)
    LATB &= 0x83;         // Apaga pines RB2 a RB6 sin tocar RB0, RB1 ni RB7
    LATDbits.LATD1 = 0;   // Apaga Buzzer
    LATCbits.LATC1 = 0;   // Apaga Ventilador

    // 5. CONFIGURACIÓN DEL VENTILADOR PWM (Módulo CCP2 en RC1)
    PR2 = 255;            
    CCP2CON = 0x0C;       
    CCPR2L = 0;           
    T2CON = 0x06;         
}

void LEDs_Cascada(unsigned int luz_porcentaje) {
    // PASO 1: Apagar todos los LEDs
    LATBbits.LATB2 = 0;
    LATBbits.LATB3 = 0;
    LATBbits.LATB4 = 0;
    LATBbits.LATB5 = 0;
    LATBbits.LATB6 = 0;

    // PASO 2: Encender en cascada segura
    if (luz_porcentaje < 85) { LATBbits.LATB6 = 1; }
    if (luz_porcentaje < 65) { LATBbits.LATB5 = 1; }
    if (luz_porcentaje < 45) { LATBbits.LATB4 = 1; }
    if (luz_porcentaje < 25) { LATBbits.LATB3 = 1; }
    if (luz_porcentaje < 5)  { LATBbits.LATB2 = 1; }
}

void Ventilador_SetPWM(unsigned int duty) {
    if(duty > 1023) duty = 1023;
    CCPR2L = (unsigned char)(duty >> 2);
    CCP2CON = (CCP2CON & 0xCF) | ((duty & 0x03) << 4);
}

void Buzzer_Set(unsigned char estado) {
    LATDbits.LATD1 = estado ? 1 : 0;
}