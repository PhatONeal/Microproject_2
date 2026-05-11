#include "ADC_Libreria.h"

#ifndef _XTAL_FREQ
    #define _XTAL_FREQ 8000000UL
#endif

/*
 * ADC_Init
 * ADCON1 = 0x0C : AN0, AN1, AN2 como analogicos. Resto digital.
 * ADCON2 = 0xBD : Justificado derecha | 20 TAD adquisicion | Fosc/16
 *   0xBD = 1011 1101
 *   Bit7 (ADFM)  = 1  -> resultado justificado a la derecha (10 bits utiles)
 *   Bits5:3 (ACQT) = 111 -> 20 TAD de adquisicion (mas estable para LM35)
 *   Bits2:0 (ADCS) = 101 -> Fosc/16 (adecuado para 8 MHz)
 */
void ADC_Init(void) {
    ADCON1 = 0x0C;
    ADCON2 = 0xBD;
    ADCON0bits.ADON = 1;
}

/*
 * ADC_Leer
 * Selecciona el canal, espera estabilizacion y retorna resultado de 10 bits.
 * Solo acepta canales 0, 1 o 2 (proteccion incluida).
 */
unsigned int ADC_Leer(unsigned char canal) {
    if (canal > 2) return 0;

    ADCON0 &= 0xC3;
    ADCON0 |= (canal << 2);

    __delay_us(30);

    ADCON0bits.GO = 1;
    while (ADCON0bits.GO);

    return ((unsigned int)(ADRESH << 8) | ADRESL);
}
