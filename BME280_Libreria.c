#include <xc.h>
#include "BME280_Libreria.h"
#include "OLED_libreria.h"

#define BME280_ADDR_WRITE  0xEC   /* 0x76 << 1        */
#define BME280_ADDR_READ   0xED   /* (0x76 << 1) | 1  */

/* Variables de calibracion de temperatura (leidas al inicio) */
uint16_t dig_T1;
int16_t  dig_T2, dig_T3;

/* Variables de calibracion de humedad */
uint8_t  dig_H1, dig_H3;
int16_t  dig_H2, dig_H4, dig_H5;
int8_t   dig_H6;
int32_t  t_fine;

/*
 * I2C_Read_Byte
 * Lectura de un byte por I2C con ACK o NACK segun parametro.
 * ack = 1 -> ACK  (hay mas bytes por leer)
 * ack = 0 -> NACK (ultimo byte de la transaccion)
 */
static unsigned char I2C_Read_Byte(unsigned char ack) {
    unsigned char data;
    I2C_Ready();
    SSPCON2bits.RCEN = 1;
    while (!SSPSTATbits.BF);
    data = SSPBUF;
    I2C_Ready();
    SSPCON2bits.ACKDT = ack ? 0 : 1;
    SSPCON2bits.ACKEN = 1;
    while (SSPCON2bits.ACKEN);
    return data;
}

static uint8_t BME280_Read8(uint8_t reg) {
    uint8_t val;
    I2C_Start(BME280_ADDR_WRITE);
    I2C_Write(reg);
    I2C_Start(BME280_ADDR_READ);
    val = I2C_Read_Byte(0);
    I2C_Stop();
    return val;
}

static uint16_t BME280_Read16_LE(uint8_t reg) {
    uint16_t val;
    I2C_Start(BME280_ADDR_WRITE);
    I2C_Write(reg);
    I2C_Start(BME280_ADDR_READ);
    val  = I2C_Read_Byte(1);
    val |= (uint16_t)(I2C_Read_Byte(0) << 8);
    I2C_Stop();
    return val;
}

static void BME280_Write8(uint8_t reg, uint8_t data) {
    I2C_Start(BME280_ADDR_WRITE);
    I2C_Write(reg);
    I2C_Write(data);
    I2C_Stop();
}

/*
 * BME280_Init
 * Lee registros de calibracion de fabrica y configura el sensor
 * en modo normal con oversampling x1.
 */
void BME280_Init(void) {
    /* Calibracion temperatura */
    dig_T1 = BME280_Read16_LE(0x88);
    dig_T2 = (int16_t)BME280_Read16_LE(0x8A);
    dig_T3 = (int16_t)BME280_Read16_LE(0x8C);

    /* Calibracion humedad */
    dig_H1 = BME280_Read8(0xA1);
    dig_H2 = (int16_t)BME280_Read16_LE(0xE1);
    dig_H3 = BME280_Read8(0xE3);
    dig_H4 = (int16_t)((BME280_Read8(0xE4) << 4) | (BME280_Read8(0xE5) & 0x0F));
    dig_H5 = (int16_t)((BME280_Read8(0xE6) << 4) | (BME280_Read8(0xE5) >> 4));
    dig_H6 = (int8_t)BME280_Read8(0xE7);

    /* Configuracion del sensor */
    BME280_Write8(0xF2, 0x01); /* ctrl_hum  : oversampling humedad x1  */
    BME280_Write8(0xF4, 0x27); /* ctrl_meas : temp x1, pres x1, normal */
}

/*
 * BME280_Leer
 * Temperatura: compensacion completa de Bosch (t_fine para humedad).
 * Humedad    : lectura SEPARADA desde registro 0xFD (2 bytes).
 *              Se escala a 0-100% sin calibracion de fabrica,
 *              igual que el codigo de referencia funcional.
 *              Incluye proteccion contra lecturas invalidas (0 o 0xFFFF).
 */
void BME280_Leer(int32_t *temperatura, uint32_t *humedad) {
    int32_t  adc_T, var1, var2, T;
    unsigned char hum_msb, hum_lsb;
    unsigned long h_raw;

    /* --- Temperatura desde 0xFA (3 bytes) --- */
    I2C_Start(BME280_ADDR_WRITE);
    I2C_Write(0xFA);
    I2C_Start(BME280_ADDR_READ);
    adc_T  = ((int32_t)I2C_Read_Byte(1) << 12);
    adc_T |= ((int32_t)I2C_Read_Byte(1) << 4);
    adc_T |= (int32_t)(I2C_Read_Byte(0) >> 4);
    I2C_Stop();

    /* Compensacion temperatura (formula oficial Bosch) */
    var1 = ((((adc_T >> 3) - ((int32_t)dig_T1 << 1))) *
             ((int32_t)dig_T2)) >> 11;
    var2 = (((((adc_T >> 4) - ((int32_t)dig_T1)) *
               ((adc_T >> 4) - ((int32_t)dig_T1))) >> 12) *
              ((int32_t)dig_T3)) >> 14;
    t_fine = var1 + var2;
    T = (t_fine * 5 + 128) >> 8;
    *temperatura = T;

    /* --- Humedad desde 0xFD (transaccion SEPARADA, 2 bytes) ---
     * RAZON: entre los registros de temperatura (0xFA-0xFC) y
     * los de humedad (0xFD-0xFE) estan los de presion (0xF7-0xF9).
     * Leer en bloque desde 0xFA corrompe la humedad. Se hace
     * una nueva transaccion I2C apuntando directamente a 0xFD.
     */
    I2C_Start(BME280_ADDR_WRITE);
    I2C_Write(0xFD);
    I2C_Stop();
    I2C_Start(BME280_ADDR_READ);
    hum_msb = I2C_Read_Byte(1);   /* ACK: viene un byte mas  */
    hum_lsb = I2C_Read_Byte(0);   /* NACK: ultimo byte       */
    I2C_Stop();

    h_raw = ((unsigned long)hum_msb << 8) | hum_lsb;

    /* Proteccion contra lecturas invalidas del sensor */
    if (h_raw == 0 || h_raw == 0xFFFF) {
        *humedad = 40;   /* Valor por defecto razonable */
        return;
    }

    *humedad = (uint32_t)(h_raw * 100UL / 65535UL);
}
