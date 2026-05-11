/*
 * =============================================================
 * main.c - Monitor Ambiental Inteligente
 * PIC18F4550 | 8 MHz interno | MPLAB XC8
 *
 * Sensores  : LM35   -> AN0 (RA0) temperatura
 *             LDR    -> AN1 (RA1) luz ambiente
 *             MQ135  -> AN2 (RA2) calidad del aire
 *             BME280 -> I2C (RB0=SDA, RB1=SCL) humedad + temperatura
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
 *   LM35 offset   : -4 C calibracion empirica del hardware
 *   LM35 filtrado : promedio de 20 muestras cada 2ms (40ms total)
 *   Ventilador    : gas > 400 raw = 100% inmediato
 *                   hum > 50%      = proporcional (sin temperatura)
 *   Alarma gas    : gas_raw > 400 sostenido ~3s  (13 ticks x 240ms)
 *   Alarma temp   : temp > 40 C sostenida  ~15s  (63 ticks x 240ms)
 *   LEDs luz      : umbrales porcentaje 85/65/45/25/5
 *
 * Layout OLED:
 *   Pag 0:  Temp: XX.X C  (LM35)     CAL:ON/OFF
 *   Pag 1:  ----------------
 *   Pag 2:  Luz:  XX %               LED:X/5
 *   Pag 3:  Hum:  XX.X %  T:XX.X C   (BME280)
 *   Pag 4:  Aire: XX %               FAN:ON/OFF
 *   Pag 5:  ----------------
 *   Pag 6:  [mensaje alerta linea 1]
 *   Pag 7:  [mensaje alerta linea 2]
 * =============================================================
 */

#include <xc.h>
#include <stdio.h>
#include <stdint.h>
#include "Configuracion.h"
#include "OLED_libreria.h"
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

// Umbral de humedad para ventilador
// Se define en 50% para mayor reactividad del BME280.
#define HUM_UMBRAL_FAN    50

// Ticks de confirmacion (1 tick ~ 240ms)
// GAS_TICKS  = 13 x 240ms ~ 3 segundos
// TEMP_TICKS = 63 x 240ms ~ 15 segundos
#define GAS_TICKS         13
#define TEMP_TICKS        63

void main(void) {
    // Variables ADC
    unsigned long  suma;
    unsigned int   t_raw, l_raw, gas_raw;
    unsigned int   temp_c, luz_pct, gas_pct;
    unsigned char  i;

    // Variables BME280
    float          hum_bme;       // Humedad float con decimal
    int32_t        temp_bme;      // Temperatura BME en centesimas
    unsigned int   hum_pct;       // Humedad entera para logica

    // Partes de temp BME para pantalla (temp_bme en centesimas: 2537 = 25.37 C)
    unsigned int   bme_t_entero;  // Parte entera
    unsigned int   bme_t_dec;     // Primer decimal

    // Variables de control
    unsigned int   gas_timer   = 0;
    unsigned int   temp_timer  = 0;
    unsigned int   tick_count  = 0;
    unsigned char  alarma_gas  = 0;
    unsigned char  alarma_temp = 0;
    unsigned char  n_leds      = 0;
    unsigned char  fan_activo  = 0;

    char           buffer[22];

    // Inicialización del PIC
    OSCCON = 0x72;
    TRISA  = 0x07;
    TRISD  = 0x00;
    LATD   = 0x00;

    // Inicialización de módulos
    I2C_Init();
    ADC_Init();
    OLED_Init();
    Actuadores_Init();

    // Pantalla de bienvenida
    OLED_Clear();
    OLED_String(1, 10, "  MONITOR     ");
    OLED_String(2, 10, "  AMBIENTAL   ");
    OLED_String(4, 5,  " UniCauca 2025");
    __delay_ms(1500);

    // Inicialización BME280 con verificación
    if (!BME280_Init()) {
        OLED_Clear();
        OLED_String(3, 0, " ERROR: BME280 ");
        OLED_String(4, 0, "  REVISAR I2C  ");
        while (1);
    }

    OLED_Clear();

    // Loop principal - tick ~ 240ms
    while (1) {

        // --- 1. LM35 con promedio de 20 muestras, resultado x10 (1 decimal) ---
        suma = 0;
        for (i = 0; i < 20; i++) {
            suma += ADC_Leer(0);
            __delay_ms(2);
        }
        t_raw  = (unsigned int)(suma / 20);
        temp_c = (unsigned int)((unsigned long)t_raw * 5000UL / 1023UL);
        if (temp_c >= 40) temp_c -= 40;   // Offset -4.0 C x10
        else              temp_c  = 0;

        // 2. LDR (AN1) - luz en porcentaje
        l_raw   = ADC_Leer(1);
        luz_pct = (unsigned int)((unsigned long)l_raw * 100UL / 1023UL);

        // 3. MQ135 (AN2) - gas en porcentaje
        gas_raw = ADC_Leer(2);
        gas_pct = (unsigned int)((unsigned long)gas_raw * 100UL / 1023UL);

        // --- 4. BME280 - humedad y temperatura ---
        // GetHumedad() siempre primero: calcula t_fine internamente.
        // GetTemp()    siempre despues: consume t_fine calculado.
        hum_bme  = BME280_GetHumedad();
        temp_bme = BME280_GetTemp();
        hum_pct  = (unsigned int)hum_bme;

        bme_t_entero = (unsigned int)(temp_bme / 100);
        bme_t_dec    = (unsigned int)((temp_bme % 100) / 10);

        // --- 5. Control de calefaccion (basado en LM35) ---
        // ON < 26.0 C (260 x10) | OFF > 27.0 C (270 x10)
        if (temp_c < (SP_TEMP_CAL_ON  * 10)) RELE_CALOR = 1;
        if (temp_c > (SP_TEMP_CAL_OFF * 10)) RELE_CALOR = 0;

        // 6. Control de iluminacion
        LEDs_Actualizar(luz_pct);

        // --- 7. Control del ventilador - prioridad: gas > humedad ---
        // Sin temperatura: temp=0 nunca alcanza FAN_TEMP_MEDIA (25).
        // Humedad proporcional desde HUM_UMBRAL_FAN (50%) hasta 100%.
        {
            unsigned int duty = 0;

            if (gas_raw > FAN_GAS_UMBRAL) {
                // Gas peligroso: maximo inmediato
                duty = 1023;
            } else if (hum_pct > HUM_UMBRAL_FAN) {
                // Humedad alta: proporcional. Rango 50-100% => duty 0-1023
                // Factor: 1023/50 ~ 20.46, usamos 20
                duty = (unsigned int)((unsigned long)
                       (hum_pct - HUM_UMBRAL_FAN) * 20UL);
                if (duty > 1023) duty = 1023;
            }

            // Aplicar duty al CCP2
            CCPR2L  = (unsigned char)(duty >> 2);
            CCP2CON = (CCP2CON & 0xCF) |
                      (unsigned char)((duty & 0x03) << 4);

            fan_activo = (duty > 0) ? 1 : 0;
        }

        // --- 8. Alarma de gas (~3 segundos) ---
        if (gas_raw > SP_GAS_RAW) {
            if (gas_timer < GAS_TICKS) gas_timer++;
            else                       alarma_gas = 1;
        } else {
            gas_timer  = 0;
            alarma_gas = 0;
        }

        // --- 9. Alarma de temperatura (~15 segundos) ---
        // Basada en LM35; temp_c / 10 = grados enteros.
        if ((temp_c / 10) > SP_TEMP_ALARMA) {
            if (temp_timer < TEMP_TICKS) temp_timer++;
            else                         alarma_temp = 1;
        } else {
            temp_timer  = 0;
            alarma_temp = 0;
        }

        // --- 10. Control del buzzer ---
        if (alarma_gas) {
            Buzzer_Set(1);                      // Continuo: peligro gas
        } else if (alarma_temp) {
            Buzzer_Set(tick_count % 4 < 2);     // Intermitente: temp alta
        } else {
            Buzzer_Set(0);
        }

        // --- 11. Visualización OLED ---
        // Pag 0: Temperatura LM35 + calefactor
        sprintf(buffer, " Temp: %u.%u C  ", temp_c / 10, temp_c % 10);
        OLED_String(0, 0, buffer);
        OLED_String(0, 90, RELE_CALOR ? "CAL:ON " : "CAL:OFF");

        // Pag 1: Separador
        OLED_String(1, 0, "----------------");

        // Pag 2: Luz + LEDs activos
        sprintf(buffer, " Luz:  %u %%    ", luz_pct);
        OLED_String(2, 0, buffer);
        n_leds = 0;
        if (luz_pct < LED_U1) n_leds++;
        if (luz_pct < LED_U2) n_leds++;
        if (luz_pct < LED_U3) n_leds++;
        if (luz_pct < LED_U4) n_leds++;
        if (luz_pct < LED_U5) n_leds++;
        sprintf(buffer, "LED:%u/5", n_leds);
        OLED_String(2, 90, buffer);

        // Pag 3: Humedad BME280 con decimal
        sprintf(buffer, " Hum:%.1f%% ", hum_bme);
        OLED_String(3, 0, buffer);

        // Pag 4: Gas + ventilador
        sprintf(buffer, " Aire: %u %%    ", gas_pct);
        OLED_String(4, 0, buffer);
        OLED_String(4, 90, fan_activo ? "FAN:ON " : "FAN:OFF");

        // Pag 5: Separador
        OLED_String(5, 0, "----------------");

        // Pags 6-7: Mensajes contextuales con prioridad
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
        } else if ((temp_c / 10) > SP_TEMP_ALARMA) {
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
