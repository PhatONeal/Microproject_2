#include "Actuadores_Libreria.h"

/*
 * =============================================================
 * Actuadores_Libreria.c
 *
 * NOTAS DE HARDWARE:
 *   - PBADEN = OFF => PORTB arranca digital desde reset.
 *   - LVP    = OFF => RB5 disponible como salida general.
 *   - CCP2MX = ON  => CCP2 mapeado a RC1, RB3 libre como LED.
 *   - RB6 funciona sin restricciones sin depurador conectado.
 *   - T2CON  = 0x04 (prescaler 1:1) con PR2=249 => ~8 kHz PWM.
 *     Frecuencia alta reduce ruido audible del motor del ventilador.
 * =============================================================
 */

void Actuadores_Init(void) {
    /* --- LEDs: RB2 a RB6 como salidas digitales ---
     * Se configuran bit a bit para no tocar RB0/RB1 (I2C).
     */
    TRISBbits.TRISB2 = 0;   LATBbits.LATB2 = 0;
    TRISBbits.TRISB3 = 0;   LATBbits.LATB3 = 0;
    TRISBbits.TRISB4 = 0;   LATBbits.LATB4 = 0;
    TRISBbits.TRISB5 = 0;   LATBbits.LATB5 = 0;
    TRISBbits.TRISB6 = 0;   LATBbits.LATB6 = 0;

    /* --- Buzzer: RD1 --- */
    TRISDbits.TRISD1 = 0;   LATDbits.LATD1 = 0;

    /* --- Rele calefactor: RD0 --- */
    TRISDbits.TRISD0 = 0;   LATDbits.LATD0 = 0;

    /* --- Ventilador: RC1 via CCP2 PWM --- */
    TRISCbits.TRISC1 = 0;
    CCPR2L  = 0;        /* Duty inicial = 0 (apagado)       */
    CCP2CON = 0x0C;     /* Modo PWM en CCP2                 */
    PR2     = 249;      /* Periodo del Timer2               */
    T2CON   = 0x04;     /* Timer2 ON, prescaler 1:1, ~8kHz */
}

/*
 * LEDs_Actualizar
 * Enciende LEDs de forma escalonada segun la oscuridad.
 * Mas luz RAW => mas luz ambiente => menos LEDs encendidos.
 */
void LEDs_Actualizar(unsigned int luz_raw) {
    LATBbits.LATB6 = (luz_raw < LED_U1) ? 1 : 0;  /* LED 1: mas sensible  */
    LATBbits.LATB5 = (luz_raw < LED_U2) ? 1 : 0;
    LATBbits.LATB4 = (luz_raw < LED_U3) ? 1 : 0;
    LATBbits.LATB3 = (luz_raw < LED_U4) ? 1 : 0;
    LATBbits.LATB2 = (luz_raw < LED_U5) ? 1 : 0;  /* LED 5: menos sensible */
}

/*
 * Ventilador_Actualizar
 * Calcula duty cycle resultante de tres condiciones con prioridad:
 *
 *   P1 - Gas peligroso (gas_raw > 400): 100% inmediato, ignora el resto.
 *   P2 - Temperatura:
 *          > 40 C  -> 100% (duty = 1023)
 *          25-40 C -> 30%  (duty =  307)
 *          < 25 C  -> sin contribucion de temperatura
 *   P3 - Humedad alta (hum_pct > 47%):
 *          Proporcional, se suma a P2, maximo aporte = 512.
 *
 * Resultado final limitado a 1023.
 */
void Ventilador_Actualizar(unsigned int hum_pct,
                           unsigned int gas_raw,
                           unsigned int temp_c) {
    unsigned int duty = 0;

    if (gas_raw > FAN_GAS_UMBRAL) {
        duty = 1023;    /* Gas peligroso: maximo sin discusion */

    } else {
        /* Contribucion por temperatura */
        if (temp_c > FAN_TEMP_ALTA) {
            duty = 1023;
        } else if (temp_c >= FAN_TEMP_MEDIA) {
            duty = 307;             /* ~30% de 1023 */
        }

        /* Contribucion adicional por humedad (se suma sobre temperatura) */
        if (hum_pct > FAN_HUM_UMBRAL) {
            unsigned int contrib = (unsigned int)(
                (unsigned long)(hum_pct - FAN_HUM_UMBRAL) * 341UL);
            if (contrib > 512) contrib = 512;
            duty += contrib;
        }

        if (duty > 1023) duty = 1023;
    }

    /* Aplicar duty cycle de 10 bits al modulo CCP2 */
    CCPR2L  = (unsigned char)(duty >> 2);
    CCP2CON = (CCP2CON & 0xCF) | (unsigned char)((duty & 0x03) << 4);
}

/*
 * Buzzer_Set
 * Control directo del pin RD1.
 * Completamente independiente del modulo MSSP (I2C).
 */
void Buzzer_Set(unsigned char estado) {
    LATDbits.LATD1 = estado ? 1 : 0;
}
