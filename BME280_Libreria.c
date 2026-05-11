#include "BME280_Libreria.h"
#include "OLED_libreria.h"

/*
 * =============================================================
 * BME280_Libreria.c
 *
 * Logica de calibracion y lectura: identica al proyecto Completo
 * del amigo (lectura en bloque con Repeated Start).
 *
 * Adaptaciones respecto a Completo:
 *   1. Direccion 0x77 (0xEE/0xEF) en vez de 0x76 (0xEC/0xED)
 *   2. Se agrega BME280_GetTemp() con compensacion Bosch entera
 *   3. Se agrega modo sleep antes de configurar ctrl_hum (0xF2)
 *      para garantizar que el oversampling de humedad tenga efecto
 *   4. Se agrega verificacion de Chip ID (0x60) con doble intento
 * =============================================================
 */

static BME_Calib c;
static long      t_fine;

/* -----------------------------------------------
 * BME_ReadReg
 * Lee un registro de 8 bits usando Repeated Start.
 * Identico al del amigo, solo cambia la direccion.
 * ----------------------------------------------- */
unsigned char BME_ReadReg(unsigned char reg) {
    unsigned char res;
    I2C_Start(BME280_ADDR_W);
    I2C_Write(reg);
    I2C_Restart();
    I2C_Start(BME280_ADDR_R);
    res = I2C_Read(0);    /* NACK: unico byte */
    I2C_Stop();
    return res;
}

/* -----------------------------------------------
 * BME280_Init
 * Identico al del amigo con tres mejoras:
 *   1. Verificacion Chip ID con doble intento
 *   2. Modo sleep antes de escribir ctrl_hum
 *   3. Delay de espera tras configurar modo normal
 * ----------------------------------------------- */
unsigned char BME280_Init(void) {
    unsigned char cal_h[7];
    unsigned char i;

    /* Verificar Chip ID ? el BME280 siempre responde 0x60 */
    if (BME_ReadReg(0xD0) != 0x60) {
        __delay_ms(100);
        if (BME_ReadReg(0xD0) != 0x60) return 0;
    }

    /* --- Calibracion temperatura en bloque desde 0x88 ---
     * Identico al amigo: una sola transaccion I2C para los 6 bytes.
     */
    I2C_Start(BME280_ADDR_W);
    I2C_Write(0x88);
    I2C_Restart();
    I2C_Start(BME280_ADDR_R);
    c.dig_T1 = I2C_Read(1) | ((unsigned short)I2C_Read(1) << 8);
    c.dig_T2 = (short)(I2C_Read(1) | (I2C_Read(1) << 8));
    c.dig_T3 = (short)(I2C_Read(1) | (I2C_Read(0) << 8));
    I2C_Stop();

    /* --- Calibracion humedad ---
     * H1 separado en 0xA1.
     * H2-H6 en bloque desde 0xE1 (7 bytes), identico al amigo.
     */
    c.dig_H1 = BME_ReadReg(0xA1);

    I2C_Start(BME280_ADDR_W);
    I2C_Write(0xE1);
    I2C_Restart();
    I2C_Start(BME280_ADDR_R);
    for (i = 0; i < 6; i++) cal_h[i] = I2C_Read(1);
    cal_h[6] = I2C_Read(0);
    I2C_Stop();

    /* Reconstruccion H2-H6 ? identica al amigo */
    c.dig_H2 = (short)(cal_h[0] | (cal_h[1] << 8));
    c.dig_H3 = cal_h[2];
    c.dig_H4 = (short)((cal_h[3] << 4) | (cal_h[4] & 0x0F));
    c.dig_H5 = (short)((cal_h[5] << 4) | (cal_h[4] >> 4));
    c.dig_H6 = (signed char)cal_h[6];

    /* --- Configuracion en orden critico ---
     * El BME280 solo acepta cambios en ctrl_hum (0xF2) en modo sleep.
     * Sin poner sleep primero, la humedad puede quedar desactivada
     * y devolver siempre 0x8000 (causa del valor fijo de 50).
     */
    I2C_Start(BME280_ADDR_W); I2C_Write(0xF4); I2C_Write(0x24); I2C_Stop(); /* Sleep  */
    __delay_ms(10);
    I2C_Start(BME280_ADDR_W); I2C_Write(0xF2); I2C_Write(0x01); I2C_Stop(); /* Hum x1 */
    __delay_ms(10);
    I2C_Start(BME280_ADDR_W); I2C_Write(0xF4); I2C_Write(0x27); I2C_Stop(); /* Normal */
    __delay_ms(100);

    return 1;
}

/* -----------------------------------------------
 * BME280_GetHumedad
 * Lectura en bloque desde 0xFA ? identica al amigo.
 * Lee 5 bytes: 3 de temp + 2 de hum en una transaccion.
 * Formula de humedad con float ? igual que el amigo.
 * ----------------------------------------------- */
float BME280_GetHumedad(void) {
    unsigned char d[5];
    unsigned char i;
    long adc_T, adc_H;
    long v1, v2;
    float h;

    I2C_Start(BME280_ADDR_W);
    I2C_Write(0xFA);
    I2C_Restart();
    I2C_Start(BME280_ADDR_R);
    for (i = 0; i < 4; i++) d[i] = I2C_Read(1);
    d[4] = I2C_Read(0);
    I2C_Stop();

    adc_T = ((long)d[0] << 12) | ((long)d[1] << 4) | (d[2] >> 4);
    adc_H = ((long)d[3] << 8)  |  d[4];

    /* Compensacion temperatura para obtener t_fine */
    v1 = ((((adc_T >> 3) - ((long)c.dig_T1 << 1))) *
           ((long)c.dig_T2)) >> 11;
    v2 = (((((adc_T >> 4) - ((long)c.dig_T1)) *
             ((adc_T >> 4) - ((long)c.dig_T1))) >> 12) *
           ((long)c.dig_T3)) >> 14;
    t_fine = v1 + v2;

    /* Formula de humedad ? identica al amigo */
    h = (float)t_fine - 76800.0f;
    h = (adc_H - (((float)c.dig_H4) * 64.0f +
                  ((float)c.dig_H5) / 16384.0f * h)) *
        (((float)c.dig_H2) / 65536.0f *
         (1.0f + ((float)c.dig_H6) / 67108864.0f * h *
          (1.0f + ((float)c.dig_H3) / 67108864.0f * h)));
    h = h * (1.0f - ((float)c.dig_H1) * h / 524288.0f);

    if (h > 100.0f) h = 100.0f;
    if (h <   0.0f) h =   0.0f;

    return h;
}

/* -----------------------------------------------
 * BME280_GetTemp
 * Usa t_fine calculado en la ultima llamada a
 * BME280_GetHumedad para obtener la temperatura
 * compensada en centesimas (2550 = 25.50 C).
 * Llamar SIEMPRE despues de BME280_GetHumedad.
 * ----------------------------------------------- */
int32_t BME280_GetTemp(void) {
    return (int32_t)((t_fine * 5 + 128) >> 8);
}