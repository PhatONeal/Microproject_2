#include <xc.h>
#include <stdio.h>
#include "Configuracion.h"
#include "ADC_Libreria.h"
#include "OLED_Libreria.h"

#define _XTAL_FREQ 8000000UL

void main(void) {
    unsigned int adc_temp, adc_gas, adc_luz;

    OSCCON = 0x72; 
    while (!OSCCONbits.IOFS);

    ADCON1 = 0x0C; 
    TRISA = 0x07; 
    LATA = 0x00;

    ADC_Init();
    I2C_Init();
    OLED_Init(); 

    OLED_Clear();
    OLED_String(1, 10, "PROBANDO OLED");

    while (1) {
        adc_temp = ADC_Leer(0);
        adc_gas = ADC_Leer(1);
        adc_luz = ADC_Leer(2);
        __delay_ms(100);
    }
}