#ifndef ACTUADORES_LIBRERIA_H
#define ACTUADORES_LIBRERIA_H

#include <xc.h>

/*
 * =============================================================
 * Actuadores_Libreria.h - Control de LEDs, Ventilador y Buzzer
 *
 * LEDs Ambientales : RB2-RB6 (5 salidas digitales independientes)
 *   Escalonados inversamente a la luz (valor RAW ADC del LDR):
 *   RB6 (LED 1): enciende si luz_raw < 800  (~85% luz)
 *   RB5 (LED 2): enciende si luz_raw < 665  (~65%)
 *   RB4 (LED 3): enciende si luz_raw < 460  (~45%)
 *   RB3 (LED 4): enciende si luz_raw < 256  (~25%)
 *   RB2 (LED 5): enciende si luz_raw <  51  (~ 5%, casi oscuridad)
 *
 * Ventilador PWM   : RC1 via modulo CCP2 (~8 kHz, 5V)
 *   Prioridad 1 - Gas elevado (raw > 400)     : 100% inmediato
 *   Prioridad 2 - Temperatura 25-40 C         : 30% fijo
 *                 Temperatura > 40 C           : 100%
 *   Prioridad 3 - Humedad alta (> 47%)         : proporcional (suma)
 *
 * Buzzer           : RD1 (salida digital)
 *   Gas  > 400 raw sostenido 3s  -> alarma continua
 *   Temp > 40 C    sostenida 15s -> alarma intermitente
 *
 * Rele Calefactor  : RD0 (salida digital)
 *   ON  si temp < 26 C
 *   OFF si temp > 27 C  (histeresis 1 C)
 * =============================================================
 */

/* Umbrales LEDs (RAW ADC del LDR, 0-1023) */
/* Umbrales LEDs ? CAMBIADO a porcentaje (0-100%) 
 * para coincidir con luz_pct que se pasa desde main.c
 */
#define LED_U1   85   /* RB6: enciende si luz_pct < 85% */
#define LED_U2   65   /* RB5 */
#define LED_U3   50   /* RB4 */
#define LED_U4   35   /* RB3 */
#define LED_U5   20   /* RB2 */

/* Umbrales ventilador */
#define FAN_GAS_UMBRAL    400   /* raw ADC: por encima es peligroso        */
#define FAN_HUM_UMBRAL     47   /* % humedad: desde aqui escala el fan     */
#define FAN_TEMP_MEDIA     25   /* C: fan al 30% entre MEDIA y ALTA        */
#define FAN_TEMP_ALTA      40   /* C: fan al 100% por encima de este valor */

void Actuadores_Init(void);
void LEDs_Actualizar(unsigned int luz_raw);
void Ventilador_Actualizar(unsigned int hum_pct,
                           unsigned int gas_raw,
                           unsigned int temp_c);
void Buzzer_Set(unsigned char estado);

#endif /* ACTUADORES_LIBRERIA_H */