/*
 * =============================================================
 * main.c - Monitor Ambiental Inteligente
 * PIC18F4550 | 8 MHz interno | MPLAB XC8
 *
 * Sensores  : LM35   -> AN0 (RA0) temperatura
 *             LDR    -> AN1 (RA1) luz ambiente
 *             MQ135  -> AN2 (RA2) calidad del aire
 *             BME280 -> I2C (RB0=SDA, RB1=SCL) humedad
 *
 * Actuadores: 5 LEDs escalonados -> RB2-RB6
 *             Ventilador 5V PWM  -> RC1 (CCP2)
 *             Buzzer             -> RD1
 *             Rele calefactor    -> RD0
 *
 * Display   : OLED SSD1306 128x64 por I2C
 *
 * Setpoints del sistema:
 *   Calefaccion   : ON < 26 C | OFF > 27 C (histeresis 1 C)
 *   LM35 offset   : -4 C por calibracion empirica del hardware
 *   LM35 filtrado : promedio de 20 muestras cada 2ms (40ms total)
 *   Ventilador    : gas > 400 raw = 100% | temp 25-40 C = 30%
 *                   temp > 40 C = 100%   | hum > 47% = proporcional
 *   Alarma gas    : gas_raw > 400 sostenido 3 segundos  (15 ticks)
 *   Alarma temp   : temp > 40 C  sostenida 15 segundos (75 ticks)
 *   LEDs luz      : umbrales raw 800 / 665 / 460 / 256 / 51
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

#define _XTAL_FREQ  8000000UL

#define RELE_CALOR      LATDbits.LATD0

// Setpoints de control
#define SP_TEMP_CAL_ON    26    // C: enciende calefactor
#define SP_TEMP_CAL_OFF   27    // C: apaga calefactor (histeresis)
#define SP_GAS_RAW       400    // ADC raw: umbral peligro MQ135
#define SP_TEMP_ALARMA    40    // C: temperatura critica

// Ticks de confirmacion (1 tick ~ 240ms: 40ms promedio LM35 + 200ms delay)
// GAS_TICKS  = 13 x 240ms ~ 3 segundos
// TEMP_TICKS = 63 x 240ms ~ 15 segundos
#define GAS_TICKS         13
#define TEMP_TICKS        63

void main(void) {
    // Variables de sensores
    unsigned long  suma;
    unsigned int   t_raw, l_raw, gas_raw;
    unsigned int   hum_pct, temp_c;
    int32_t        bme_temp_dummy;
    uint32_t       bme_hum_raw;
    unsigned char  i;

    // Variables de control
    unsigned int   gas_timer   = 0;
    unsigned int   temp_timer  = 0;
    unsigned int   tick_count  = 0;
    unsigned char  alarma_gas  = 0;
    unsigned char  alarma_temp = 0;
    unsigned char  n_leds      = 0;
    unsigned char  fan_activo  = 0;

    // Inicialización del PIC
    // TRISA = 0x07: RA0, RA1, RA2 como entradas analogicas
    // TRISD = 0x00: Puerto D completo como salida
    // TRISB: I2C_Init() maneja RB0/RB1; Actuadores_Init() maneja RB2-RB6
    OSCCON = 0x72;
    TRISA  = 0x07;
    TRISD  = 0x00;
    LATD   = 0x00;

    // Inicialización de módulos (orden importante)
    // I2C_Init primero: configura RB0/RB1 antes que Actuadores_Init
    I2C_Init();
    ADC_Init();
    OLED_Init();
    BME280_Init();
    Actuadores_Init();

    // Pantalla de bienvenida
    OLED_Clear();
    OLED_String(1, 10, "  MONITOR     ");
    OLED_String(2, 10, "  AMBIENTAL   ");
    OLED_String(4, 5,  " UniCauca 2025");
    __delay_ms(2000);
    OLED_Clear();

    // Loop principal - periodo por tick: ~240ms
    while (1) {

        // --- 1. LM35 con promedio de 20 muestras (1 tick = 40ms) ---
        // Promedio reduce ruido del PWM del ventilador e interferencias I2C.
        // Factor 500: Vref=5V, LM35 da 10mV/C => 1C = 1023/500 = 2.046 cuentas ADC.
        // Offset -4C: correccion empirica medida en el hardware.
        suma = 0;
        for (i = 0; i < 20; i++) {
            suma += ADC_Leer(0);
            __delay_ms(2);
        }
        t_raw  = (unsigned int)(suma / 20);
        temp_c = (unsigned int)((unsigned long)t_raw * 500UL / 1023UL);
        if (temp_c >= 4) temp_c -= 4;
        else             temp_c  = 0;

        // 2. LDR (AN1) y MQ135 (AN2) - lectura directa
        // Una sola lectura es suficiente; estos sensores son mas estables.
        l_raw   = ADC_Leer(1);
        gas_raw = ADC_Leer(2);

        // 3. BME280 - humedad por I2C
        BME280_Leer(&bme_temp_dummy, &bme_hum_raw);
        hum_pct = (unsigned int)bme_hum_raw;

        // --- 4. Control de calefaccion ---
        // Histeresis de 1C evita ciclos rapidos del rele.
        if (temp_c < SP_TEMP_CAL_ON)  RELE_CALOR = 1;
        if (temp_c > SP_TEMP_CAL_OFF) RELE_CALOR = 0;

        // 5. Control de iluminacion - mas oscuridad => mas LEDs encendidos
        LEDs_Actualizar(l_raw);

        // 6. Control del ventilador - prioridad: gas > temperatura > humedad
        Ventilador_Actualizar(hum_pct, gas_raw, temp_c);

        // --- 7. Alarma de gas (~3 segundos) ---
        // gas_raw > 400 sostenido GAS_TICKS => alarma. Reset inmediato si baja.
        if (gas_raw > SP_GAS_RAW) {
            if (gas_timer < GAS_TICKS) gas_timer++;
            else                       alarma_gas = 1;
        } else {
            gas_timer  = 0;
            alarma_gas = 0;
        }

        // --- 8. Alarma de temperatura (~15 segundos) ---
        // Indica fallo del calefactor o fuente externa de calor.
        if (temp_c > SP_TEMP_ALARMA) {
            if (temp_timer < TEMP_TICKS) temp_timer++;
            else                         alarma_temp = 1;
        } else {
            temp_timer  = 0;
            alarma_temp = 0;
        }

        // --- 9. Control del buzzer ---
        // Gas (continuo) tiene prioridad sobre temperatura (intermitente).
        // Patrones distintos identifican el tipo de alarma sin ver la pantalla.
        if (alarma_gas) {
            Buzzer_Set(1);                      // Continuo: peligro gas
        } else if (alarma_temp) {
            Buzzer_Set(tick_count % 4 < 2);     // Intermitente: temp alta
        } else {
            Buzzer_Set(0);
        }

        // --- 10. Visualización OLED ---
        // Layout 8 páginas (128x64px, 8px por página):
        //   Pag 0: Temperatura C        | Estado calefactor
        //   Pag 1: Separador visual
        //   Pag 2: Luz RAW              | LEDs activos X/5
        //   Pag 3: Humedad %            | Setpoint >47%
        //   Pag 4: Estado gas           | Estado ventilador
        //   Pag 5: Separador visual
        //   Pag 6: Mensaje alerta linea 1
        //   Pag 7: Mensaje alerta linea 2
        //
        // Prioridad de mensajes (mayor a menor):
        //   1. Alarma gas confirmada    (parpadeo rapido)
        //   2. Gas elevado sin confirmar
        //   3. Alarma temperatura       (parpadeo lento)
        //   4. Temperatura critica sin confirmar
        //   5. Humedad muy alta > 70%
        //   6. Humedad muy baja < 30%
        //   7. Calefactor activo
        //   8. Sistema normal

        // Pag 0: Temperatura + calefactor
        OLED_String(0, 0, "T:");
        OLED_BorrarTexto(0, 12, 3);
        OLED_Int(0, 12, temp_c);
        OLED_String(0, 36, "C");
        OLED_String(0, 55, RELE_CALOR ? "CAL:ON " : "CAL:OFF");

        // Pag 1: Separador
        OLED_String(1, 0, "----------------");

        // Pag 2: Luz RAW + LEDs activos
        OLED_String(2, 0, "L:");
        OLED_BorrarTexto(2, 12, 4);
        OLED_Int(2, 12, l_raw);

        n_leds = 0;
        if (l_raw < LED_U1) n_leds++;
        if (l_raw < LED_U2) n_leds++;
        if (l_raw < LED_U3) n_leds++;
        if (l_raw < LED_U4) n_leds++;
        if (l_raw < LED_U5) n_leds++;
        OLED_String(2, 60, "LED:");
        OLED_BorrarTexto(2, 90, 2);
        OLED_Int(2, 90, n_leds);
        OLED_String(2, 102, "/5");

        // Pag 3: Humedad + setpoint
        OLED_String(3, 0, "H:");
        OLED_BorrarTexto(3, 12, 3);
        OLED_Int(3, 12, hum_pct);
        OLED_String(3, 36, "%");
        OLED_String(3, 55, "SP:>47%");

        // Pag 4: Gas + ventilador
        OLED_String(4, 0, "G:");
        OLED_String(4, 12, gas_raw > SP_GAS_RAW ? "MAL" : "OK ");

        fan_activo = (gas_raw > FAN_GAS_UMBRAL)  ||
                     (hum_pct > FAN_HUM_UMBRAL)   ||
                     (temp_c  >= FAN_TEMP_MEDIA);
        OLED_String(4, 55, "FAN:");
        OLED_String(4, 85, fan_activo ? "ON " : "OFF");

        // Pag 5: Separador
        OLED_String(5, 0, "----------------");

        // Pags 6-7: Mensajes de estado con prioridad
        if (alarma_gas) {
            if (tick_count % 2 == 0) {
                OLED_String(6, 0, "!! ALARMA GAS !!");
                OLED_String(7, 0, "  VENTILAR YA  ");
            } else {
                OLED_BorrarTexto(6, 0, 17);
                OLED_BorrarTexto(7, 0, 17);
            }
        } else if (gas_raw > SP_GAS_RAW) {
            OLED_String(6, 0, " GAS ELEVADO.. ");
            OLED_String(7, 0, " ESPERA: 3s... ");
        } else if (alarma_temp) {
            if (tick_count % 4 < 2) {
                OLED_String(6, 0, "!! TEMP ALTA !! ");
                OLED_String(7, 0, "  REVISAR CAL  ");
            } else {
                OLED_BorrarTexto(6, 0, 17);
                OLED_BorrarTexto(7, 0, 17);
            }
        } else if (temp_c > SP_TEMP_ALARMA) {
            OLED_String(6, 0, " TEMP CRITICA  ");
            OLED_String(7, 0, " ESPERA: 15s.. ");
        } else if (hum_pct > 70) {
            OLED_String(6, 0, " HUM MUY ALTA  ");
            OLED_String(7, 0, " VENTILANDO... ");
        } else if (hum_pct < 30) {
            OLED_String(6, 0, " HUM MUY BAJA  ");
            OLED_String(7, 0, " REDUCIR VENT. ");
        } else if (RELE_CALOR) {
            OLED_String(6, 0, " TEMP BAJA     ");
            OLED_String(7, 0, " CALENTANDO... ");
        } else {
            OLED_String(6, 0, " SISTEMA OK    ");
            OLED_String(7, 0, "  TODO NORMAL  ");
        }

        tick_count++;
        __delay_ms(200);
    }
}
