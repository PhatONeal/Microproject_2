#include <xc.h>
#include "Configuracion.h"
#include "ADC_Libreria.h" // Inclusión de librería ADC

#define _XTAL_FREQ  8000000UL

void main(void) {
    // Variables para datos crudos (0 a 1023)
    unsigned int adc_temp_raw;
    unsigned int adc_gas_raw;
    unsigned int adc_luz_raw;

    // Configuración de oscilador a 8MHz
    OSCCON = 0x72; 
    while (!OSCCONbits.IOFS);

    // Configuración de puertos analógicos
    ADCON1 = 0x0C; 
    TRISA = 0x07; 
    LATA = 0x00;

    // Inicializar módulo ADC
    ADC_Init(); 

    while (1) {
        // Lectura directa de los pines
        adc_temp_raw = ADC_Leer(0); // LM35 en AN0
        adc_gas_raw = ADC_Leer(1);  // MQ135 en AN1
        adc_luz_raw = ADC_Leer(2);  // LDR en AN2
        
        __delay_ms(100); // Pausa de estabilidad
    }
}