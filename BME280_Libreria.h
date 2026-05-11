#ifndef BME280_LIBRERIA_H
#define BME280_LIBRERIA_H

#include <xc.h>
#include <stdint.h>

/*
 * =============================================================
 * BME280_Libreria.h
 *
 * Direccion I2C: 0x77 (SDO a VCC) ? confirmado con diagnostico
 *   Write : 0xEE  |  Read : 0xEF
 *
 * Logica: identica al proyecto Completo del amigo, adaptada a:
 *   - Direccion 0x77 en vez de 0x76
 *   - Funciones I2C de OLED_libreria (I2C_Start, I2C_Write,
 *     I2C_Stop, I2C_Restart, I2C_Read)
 *   - Formula de humedad sin float (enteros de 32 bits)
 *     para mayor velocidad y menor uso de memoria en XC8
 *
 * Salidas:
 *   BME280_GetHumedad() : porcentaje float  (ej: 65.3)
 *   BME280_GetTemp()    : centesimas entero (ej: 2550 = 25.50 C)
 * =============================================================
 */

#define BME280_ADDR_W  0xEC   /* 0x76 << 1       */
#define BME280_ADDR_R  0xED   /* (0x76 << 1) | 1 */

typedef struct {
    unsigned short dig_T1;
    short          dig_T2;
    short          dig_T3;
    unsigned char  dig_H1;
    short          dig_H2;
    unsigned char  dig_H3;
    short          dig_H4;
    short          dig_H5;
    signed char    dig_H6;
} BME_Calib;

/* Retorna 1 si el sensor respondio correctamente, 0 si hubo error */
unsigned char BME280_Init(void);

/* Retorna humedad en % como float ? igual que Completo */
float         BME280_GetHumedad(void);

/* Retorna temperatura en centesimas (2550 = 25.50 C) */
int32_t       BME280_GetTemp(void);

#endif /* BME280_LIBRERIA_H */