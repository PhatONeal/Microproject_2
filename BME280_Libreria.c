#include <xc.h>
#include "BME280_Libreria.h"
#include "OLED_libreria.h" 

#define BME280_ADDR_WRITE 0xEC // 0x76 << 1 (Write)
#define BME280_ADDR_READ  0xED // 0x76 << 1 (Read) | Cambia a 0xEE/0xEF si SDO está a VCC

// Variables de calibración interna del BME280
uint16_t dig_T1; int16_t dig_T2, dig_T3;
uint8_t  dig_H1, dig_H3;
int16_t  dig_H2, dig_H4, dig_H5; int8_t dig_H6;
int32_t  t_fine;

// Función privada para leer por I2C sin alterar tu librería OLED
unsigned char I2C_Read_Byte(unsigned char ack) {
    unsigned char data;
    I2C_Ready();
    SSPCON2bits.RCEN = 1;       // Habilita recepción
    while (!SSPSTATbits.BF);    // Espera el dato
    data = SSPBUF;              // Lee el buffer
    I2C_Ready();
    SSPCON2bits.ACKDT = ack ? 0 : 1; 
    SSPCON2bits.ACKEN = 1;      // Envia bit de ACK
    while (SSPCON2bits.ACKEN);
    return data;
}

uint8_t BME280_Read8(uint8_t reg) {
    uint8_t val;
    I2C_Start(BME280_ADDR_WRITE);
    I2C_Write(reg);
    I2C_Start(BME280_ADDR_READ);
    val = I2C_Read_Byte(0); // NACK para terminar
    I2C_Stop();
    return val;
}

uint16_t BME280_Read16_LE(uint8_t reg) {
    uint16_t val;
    I2C_Start(BME280_ADDR_WRITE);
    I2C_Write(reg);
    I2C_Start(BME280_ADDR_READ);
    val = I2C_Read_Byte(1);             // ACK
    val |= (I2C_Read_Byte(0) << 8);     // NACK al último byte
    I2C_Stop();
    return val;
}

void BME280_Write8(uint8_t reg, uint8_t data) {
    I2C_Start(BME280_ADDR_WRITE);
    I2C_Write(reg);
    I2C_Write(data);
    I2C_Stop();
}

void BME280_Init(void) {
    // 1. Leer registros de calibración de fábrica (OBLIGATORIO)
    dig_T1 = BME280_Read16_LE(0x88);
    dig_T2 = (int16_t)BME280_Read16_LE(0x8A);
    dig_T3 = (int16_t)BME280_Read16_LE(0x8C);
    
    dig_H1 = BME280_Read8(0xA1);
    dig_H2 = (int16_t)BME280_Read16_LE(0xE1);
    dig_H3 = BME280_Read8(0xE3);
    dig_H4 = (int16_t)((BME280_Read8(0xE4) << 4) | (BME280_Read8(0xE5) & 0x0F));
    dig_H5 = (int16_t)((BME280_Read8(0xE6) << 4) | (BME280_Read8(0xE5) >> 4));
    dig_H6 = (int8_t)BME280_Read8(0xE7);

    // 2. Configurar el sensor
    BME280_Write8(0xF2, 0x01); // ctrl_hum: Oversampling x1
    BME280_Write8(0xF4, 0x27); // ctrl_meas: Temp x1, Press x1, Modo Normal
}

void BME280_Leer(int32_t *temperatura, uint32_t *humedad) {
    int32_t adc_T, adc_H, var1, var2, T;
    
    // Leer registros crudos de Temp y Hum (Saltamos presión para optimizar)
    I2C_Start(BME280_ADDR_WRITE);
    I2C_Write(0xFA); // Inicio de registros de Temp
    I2C_Start(BME280_ADDR_READ);
    adc_T = ((int32_t)I2C_Read_Byte(1) << 12) | ((int32_t)I2C_Read_Byte(1) << 4) | (I2C_Read_Byte(1) >> 4);
    adc_H = ((int32_t)I2C_Read_Byte(1) << 8) | I2C_Read_Byte(0);
    I2C_Stop();

    // Compensación de Temperatura (Fórmula de Bosch)
    var1 = ((((adc_T>>3) - ((int32_t)dig_T1<<1))) * ((int32_t)dig_T2)) >> 11;
    var2 = (((((adc_T>>4) - ((int32_t)dig_T1)) * ((adc_T>>4) - ((int32_t)dig_T1))) >> 12) * ((int32_t)dig_T3)) >> 14;
    t_fine = var1 + var2;
    T = (t_fine * 5 + 128) >> 8;
    *temperatura = T; // Temperatura en formato XX.XX °C (2550 = 25.50C)

    // Compensación de Humedad (Fórmula de Bosch)
    int32_t v_x1_u32r;
    v_x1_u32r = (t_fine - ((int32_t)76800));
    v_x1_u32r = (((((adc_H << 14) - (((int32_t)dig_H4) << 20) - (((int32_t)dig_H5) * v_x1_u32r)) + ((int32_t)16384)) >> 15) * (((((((v_x1_u32r * ((int32_t)dig_H6)) >> 10) * (((v_x1_u32r * ((int32_t)dig_H3)) >> 11) + ((int32_t)32768))) >> 10) + ((int32_t)2097152)) * ((int32_t)dig_H2) + 8192) >> 14));
    v_x1_u32r = (v_x1_u32r - (((((v_x1_u32r >> 15) * (v_x1_u32r >> 15)) >> 7) * ((int32_t)dig_H1)) >> 4));
    v_x1_u32r = (v_x1_u32r < 0 ? 0 : v_x1_u32r);
    v_x1_u32r = (v_x1_u32r > 419430400 ? 419430400 : v_x1_u32r);
    *humedad = (uint32_t)(v_x1_u32r >> 12) / 1024; // Humedad en %RH (0-100%)
}
