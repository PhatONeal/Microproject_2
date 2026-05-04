#include "ADC_Libreria.h"

#ifndef _XTAL_FREQ
    #define _XTAL_FREQ 8000000UL
#endif

void ADC_Init(void) {
    /* * ADCON1: 
     * Configura voltajes de referencia (VDD y VSS internos).
     * El valor 0x0C (0000 1100) configura AN0, AN1 y AN2 como ANALOGICOS.
     * El resto de pines del Puerto A y B se mantienen digitales.
     */
    ADCON1 = 0x0C;

    /* * ADCON2:
     * Bit 7 (ADFM) = 1 (Justificado a la derecha, para usar los 10 bits).
     * Bits 5:3 (ACQT) = 101 (12 TAD, tiempo de adquisicion).
     * Bits 2:0 (ADCS) = 010 (Fosc/32, reloj de conversion para 8 MHz).
     * Total = 10100010 = 0xA2
     */
    ADCON2 = 0xA2;

    /* Enciende el modulo ADC */
    ADCON0bits.ADON = 1;
}

unsigned int ADC_Leer(unsigned char canal) {
    if(canal > 2) return 0; // Proteccion: Solo usamos 0, 1 o 2

    // Limpia el canal actual (bits 5:2 de ADCON0) y asigna el nuevo
    ADCON0 &= 0xC3; 
    ADCON0 |= (canal << 2);

    // Pequeña pausa para estabilizar el capacitor de retencion interno
    __delay_us(20);

    // Inicia la conversion analógica a digital
    ADCON0bits.GO = 1;

    // Espera a que termine (el hardware pone el bit en 0 al finalizar)
    while(ADCON0bits.GO);

    // Retorna el resultado de 10 bits combinando ADRESH y ADRESL
    return ((ADRESH << 8) + ADRESL);
}
