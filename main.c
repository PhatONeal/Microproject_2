/*
 * =============================================================
 * main.c — Sistema de Monitoreo de Confort (PIC18F4550)
 * =============================================================
 * Fase 1: Configuración inicial de oscilador y puertos analógicos.
 */

#include <xc.h>
#include "Configuracion.h" // Aquí están los #pragma config

#define _XTAL_FREQ  8000000UL

void main(void) {
    // --- 1. Configuración de Oscilador a 8 MHz ---
    OSCCON = 0x72; 
    while (!OSCCONbits.IOFS); // Espera a que el oscilador se estabilice

    // --- 2. Configuración de Puertos (ADC) ---
    // ADCON1 = 0x0C -> Configura AN0, AN1 y AN2 como Analógicos
    // VREF+ = VDD (5V), VREF- = VSS (GND)
    ADCON1 = 0x0C; 
    
    // Configuramos los pines físicos como entradas (1 = Entrada)
    // RA0 (LM35), RA1 (MQ135), RA2 (LDR)
    TRISA = 0x07; 
    
    // Limpiamos los pestillos por seguridad
    LATA = 0x00;

    while (1) {
        // Bucle principal vacío.
        // Aquí agregaremos las lecturas crudas en la siguiente fase.
    }
}