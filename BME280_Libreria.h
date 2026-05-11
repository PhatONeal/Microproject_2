#ifndef BME280_LIBRERIA_H
#define BME280_LIBRERIA_H

#include <stdint.h>

void BME280_Init(void);

// Lee los valores compensados. 
// La temperatura viene multiplicada por 100 (Ej: 2550 = 25.50 °C)
// La humedad viene en porcentaje (Ej: 55 = 55 %RH)
void BME280_Leer(int32_t *temperatura, uint32_t *humedad);

#endif