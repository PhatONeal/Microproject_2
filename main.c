/*
 * =============================================================
 * main.c - Sistema de Monitoreo Ambiental (Versión Centrada)
 * PIC18F4550 | OLED SSD1306 | LM35 | LDR | MQ135
 * =============================================================
 */

#include <xc.h>
#include <stdio.h>
#include <stdint.h>
#include "Configuracion.h"
#include "OLED_Libreria.h"
#include "ADC_Libreria.h"
#include "BME280_Libreria.h"
#include "Actuadores_Libreria.h"

#define _XTAL_FREQ 8000000UL

void main(void) {
    // Variables unificadas
    unsigned int adc_temp, adc_luz, adc_gas;
    unsigned int temp_lm35_10;
    unsigned int luz_porcentaje;
    unsigned int gas_porcentaje;

    // Variables BME280 (en espera)
    int32_t bme_temp_dummy;
    uint32_t bme_hum_raw;

    char buffer_oled[16];

    // Inicialización general
    OSCCON = 0x72;   // Reloj a 8 MHz

    ADC_Init();
    I2C_Init();
    OLED_Init();
    BME280_Init();
    Actuadores_Init();

    // Pantalla de bienvenida
    OLED_Clear();
    OLED_String(2, 10, "SISTEMA INICIADO");
    __delay_ms(1500);
    OLED_Clear();

    // Etiquetas fijas para evitar parpadeos
    OLED_String(0, 0, "T:");
    OLED_String(2, 0, "LUZ:");
    OLED_String(4, 0, "AIRE:");
    OLED_String(6, 0, "FAN:");

    while (1) {
        // --- 1. LM35 con promedio de 10 muestras (AN0) ---
        unsigned long suma_temp = 0;
        for (char i = 0; i < 10; i++) { suma_temp += ADC_Leer(0); __delay_ms(1); }
        adc_temp = suma_temp / 10;

        // 2. LDR (AN2) y MQ135 (AN1) - lectura directa
        adc_luz = ADC_Leer(2);
        adc_gas = ADC_Leer(1);

        // --- Conversiones matemáticas ---

        // LM35: (ADC * 5000mV) / 1024 -> resultado x10, ej: 265 = 26.5 C
        temp_lm35_10 = (unsigned int)((adc_temp * 5000UL) / 1024);

        luz_porcentaje = (unsigned int)((adc_luz * 100UL) / 1023);

        // MQ135 invertido: menos resistencia = más gas
        gas_porcentaje = (unsigned int)(((1023UL - adc_gas) * 100UL) / 1023);

        // Lectura silenciosa del BME280
        BME280_Leer(&bme_temp_dummy, &bme_hum_raw);

        // --- Control de actuadores ---

        // LEDs en cascada según nivel de luz
        LEDs_Cascada(luz_porcentaje);

        // Ventilador térmico: umbral 26.0 °C (260 en x10)
        if (temp_lm35_10 >= 260) {
            Ventilador_SetPWM(1023);      // 100%
            OLED_String(6, 30, "ON ");
        } else {
            Ventilador_SetPWM(0);         // Apagado
            OLED_String(6, 30, "OFF");
        }

        // Buzzer de alarma: menos del 40% = aire sucio
        if (gas_porcentaje < 40) {
            Buzzer_Set(1);
        } else {
            Buzzer_Set(0);
        }

        // --- Actualización de pantalla ---
        sprintf(buffer_oled, "%u.%u C  ", temp_lm35_10 / 10, temp_lm35_10 % 10);
        OLED_String(0, 30, buffer_oled);

        sprintf(buffer_oled, "%u %%  ", luz_porcentaje);
        OLED_String(2, 30, buffer_oled);

        sprintf(buffer_oled, "%u %%  ", gas_porcentaje);
        OLED_String(4, 30, buffer_oled);

        // Estado de calidad del aire
        if (gas_porcentaje < 40) {
            OLED_String(4, 70, "!PELIGRO");
        } else {
            OLED_String(4, 70, " NORMAL ");
        }

        __delay_ms(300);
    }
}
