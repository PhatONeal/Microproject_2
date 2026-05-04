#include <xc.h>
#include <stdio.h>
#include <stdint.h>
#include "Configuracion.h"
#include "OLED_Libreria.h"
#include "ADC_Libreria.h"
#include "BME280_Libreria.h"

#define _XTAL_FREQ  8000000UL

void main(void) {
    // Variables
    unsigned int adc_luz_raw;
    unsigned int temp_lm35, gas_porcentaje, luz_porcentaje;
    int32_t bme_temp_dummy; 
    uint32_t bme_hum_raw;

    // Inicialización
    OSCCON = 0x72; 
    ADCON1 = 0x0C; 
    TRISA  = 0x07; 
    TRISBbits.TRISB0 = 1; 
    TRISBbits.TRISB1 = 1; 

    ADC_Init();
    I2C_Init();
    OLED_Init();
    BME280_Init();

    // Pantalla de inicio
    OLED_Clear();
    OLED_String(1, 15, "   MONITOR    ");
    OLED_String(2, 15, "  AMBIENTAL   ");
    __delay_ms(1500);
    OLED_Clear();

    // ETIQUETAS ESTÁTICAS
    OLED_String(0, 10, "TEMP (LM35):");
    OLED_String(2, 10, "HUMEDAD BME:");
    OLED_String(5, 0, "AIRE:");
    OLED_String(5, 64, "LUZ:");

    while (1) {
        // --- 1. LM35 con Promedio (Filtro de ruido) ---
        unsigned long suma = 0;
        for(char i = 0; i < 20; i++){
            suma += ADC_Leer(0); // Leemos el canal AN0 (LM35)
            __delay_ms(2);
        }
        // Calculamos la temperatura con el promedio de las 20 muestras
        temp_lm35 = (unsigned int)(((suma / 20) * 506UL) / 1023);

        // --- 2. BME280 (Solo Humedad) ---
        BME280_Leer(&bme_temp_dummy, &bme_hum_raw);
        unsigned int hum_final = (unsigned int)bme_hum_raw;

        // --- 3. MQ135 INVERTIDO ---
        unsigned int gas_inv = 1023 - ADC_Leer(1);
        gas_porcentaje = (unsigned int)((gas_inv * 100UL) / 1023);

        // --- 4. LDR ---
        adc_luz_raw = ADC_Leer(2);
        luz_porcentaje = (unsigned int)((adc_luz_raw * 100UL) / 1023);

        // --- MOSTRAR DATOS ---
        
        // Temperatura Analógica
        OLED_String(0, 85, "    "); 
        OLED_Int(0, 85, temp_lm35);
        OLED_String(0, 110, "C");

        // Humedad Digital (BME)
        OLED_String(2, 85, "    ");
        OLED_Int(2, 85, hum_final);
        OLED_String(2, 110, "%");

        // Fila inferior (Gas y Luz)
        OLED_String(6, 0, "    ");
        OLED_Int(6, 0, gas_porcentaje);
        OLED_String(6, 30, "%");

        OLED_String(6, 90, "    ");
        OLED_Int(6, 90, luz_porcentaje);
        OLED_String(6, 115, "%");

        __delay_ms(500);
    }
}