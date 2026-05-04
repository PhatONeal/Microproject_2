#include <xc.h>
#include <stdio.h>
#include "Configuracion.h"
#include "ADC_Libreria.h"
#include "OLED_Libreria.h"

#define _XTAL_FREQ 8000000UL

void main(void) {
    unsigned int adc_temp, adc_gas, adc_luz;
    unsigned int temp_c, gas_p, luz_p;
    char buffer[16];

    OSCCON = 0x72; while (!OSCCONbits.IOFS);
    ADCON1 = 0x0C; TRISA = 0x07; LATA = 0x00;
    TRISBbits.TRISB0 = 1; TRISBbits.TRISB1 = 1; 

    ADC_Init(); I2C_Init(); OLED_Init(); OLED_Clear();

    while (1) {
        adc_temp = ADC_Leer(0);
        adc_gas = ADC_Leer(1);
        adc_luz = ADC_Leer(2);

        // Conversiones matemáticas básicas
        temp_c = (unsigned int)((adc_temp * 506UL) / 1023);
        gas_p = (unsigned int)((adc_gas * 100UL) / 1023);
        luz_p = (unsigned int)((adc_luz * 100UL) / 1023);

        sprintf(buffer, "Temp: %u C  ", temp_c);
        OLED_String(0, 0, buffer);
        sprintf(buffer, "Gas: %u %%  ", gas_p);
        OLED_String(2, 0, buffer);
        sprintf(buffer, "Luz: %u %%  ", luz_p);
        OLED_String(4, 0, buffer);
        
        __delay_ms(200);
    }
}