#ifndef BME280_LIBRERIA_H
#define BME280_LIBRERIA_H

#include <stdint.h>

/*
 * =============================================================
 * BME280_Libreria.h - Driver simplificado Temperatura y Humedad
 * Direccion I2C: 0x76 (SDO a GND) => Write=0xEC, Read=0xED
 * =============================================================
 */

void BME280_Init(void);

/*
 * BME280_Leer
 * temperatura : compensada en formato XX.XX (2550 = 25.50 C)
 * humedad     : porcentaje directo 0-100 %RH
 *               (lectura separada desde 0xFD, sin calibracion de fabrica)
 */
void BME280_Leer(int32_t *temperatura, uint32_t *humedad);

#endif /* BME280_LIBRERIA_H */
