/*
 * =============================================================
 * main.c — Sistema de Monitoreo (Versión Centrada)
 * RB0 = SDA, RB1 = SCL (PIC18F4550)
 * =============================================================
 */

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
    unsigned int gas_porcentaje, luz_porcentaje;
    unsigned int temp_lm35_10, t_entero, t_decimal; 
    int32_t bme_temp_dummy; 
    uint32_t bme_hum_raw;
    char buf_temp[12];

    // Inicialización
    OSCCON = 0x72; 
    ADCON1 = 0x0C; 
    TRISA  = 0x07; 
    // TRISB ya no se configura aquí, está dentro de I2C_Init()

    ADC_Init();
    I2C_Init();
    OLED_Init();
    BME280_Init();

    OLED_Clear();
    OLED_String(1, 15, "   MONITOR    ");
    OLED_String(2, 15, "  AMBIENTAL   ");
    __delay_ms(1500);
    OLED_Clear();

    // ETIQUETAS ESTÁTICAS (Nueva distribución)
    OLED_String(0, 10, "TEMP (LM35):");
    OLED_String(2, 10, "HUMEDAD BME:");
    OLED_String(5, 0, "AIRE:");
    OLED_String(5, 64, "LUZ:");

    while (1) {
        // --- 1. LM35 con Promedio y Alta Precisión (1 decimal) ---
        unsigned long suma = 0;
        for(char i = 0; i < 20; i++){
            suma += ADC_Leer(0); 
            __delay_ms(2);
        }
        
        // Multiplicamos por 5060 para conservar 1 decimal
        unsigned int temp_cruda = suma / 20;
        temp_lm35_10 = (unsigned int)((temp_cruda * 5060UL) / 1023);
        
        // Ajuste por software: restamos 40 (equivale a 4.0 grados)
        if (temp_lm35_10 >= 40) {
            temp_lm35_10 = temp_lm35_10 - 40; 
        } else {
            temp_lm35_10 = 0; 
        }

        t_entero = temp_lm35_10 / 10;
        t_decimal = temp_lm35_10 % 10;

        // 2. BME280 (Solo Humedad)
        BME280_Leer(&bme_temp_dummy, &bme_hum_raw);
        unsigned int hum_final = (unsigned int)bme_hum_raw;

        // 3. MQ135 Invertido
        unsigned int gas_inv = 1023 - ADC_Leer(1);
        gas_porcentaje = (unsigned int)((gas_inv * 100UL) / 1023);

        // 4. LDR
        luz_porcentaje = (unsigned int)((ADC_Leer(2) * 100UL) / 1023);

        // --- MOSTRAR DATOS ---
        
        // Temperatura Analógica (Con decimal)
        sprintf(buf_temp, "%u.%u C ", t_entero, t_decimal);
        OLED_String(0, 85, buf_temp);

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