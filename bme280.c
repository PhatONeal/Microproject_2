#pragma config FOSC = INTOSCIO_EC   // Oscilador interno, RA6 como I/O
#pragma config PLLDIV = 1
#pragma config CPUDIV = OSC1_PLL2   // ?? valor válido
#pragma config USBDIV = 1

#pragma config FCMEN = OFF
#pragma config IESO = OFF
#pragma config PWRT = OFF
#pragma config BOR = OFF
#pragma config WDT = OFF
#pragma config MCLRE = ON
#pragma config LVP = OFF
#pragma config PBADEN = OFF

#define BME280_ADDR_WRITE 0xEC
#define BME280_ADDR_READ  0xED

#define _XTAL_FREQ 8000000
#include <xc.h>
#include <stdio.h>

#include "i2c.h"
#include "ssd1306_oled.h"

// ================= ADC =================

void BME280_Write(unsigned char reg, unsigned char data)
{
    I2C_Start();
    I2C_Write(BME280_ADDR_WRITE);
    I2C_Write(reg);
    I2C_Write(data);
    I2C_Stop();
}

unsigned char BME280_Read(unsigned char reg)
{
    unsigned char data;

    I2C_Start();
    I2C_Write(BME280_ADDR_WRITE);
    I2C_Write(reg);

    I2C_Restart();
    I2C_Write(BME280_ADDR_READ);
    data = I2C_Read();
    I2C_Nack();
    I2C_Stop();

    return data;
}

void BME280_Init(void)
{
    __delay_ms(100);

    // Humedad oversampling x1
    BME280_Write(0xF2, 0x01);

    // Temp y presión oversampling x1, modo normal
    BME280_Write(0xF4, 0x27);

    // Configuración (standby + filtro)
    BME280_Write(0xF5, 0xA0);
}

unsigned int BME280_ReadHumidity(void)
{
    unsigned int hum;

    unsigned char msb = BME280_Read(0xFD);
    unsigned char lsb = BME280_Read(0xFE);

    hum = (msb << 8) | lsb;

    // Escalar a 0?100% (aproximado)
    return (hum / 1024); 
}

void ADC_Init(void){
    ADCON1 = 0x0B;   // AN0, AN1 y AN2 analógicos
    ADCON2 = 0xAA;   // Correcto para 8MHz
    

    TRISAbits.TRISA0 = 1;
    TRISAbits.TRISA1 = 1;

    ADCON0bits.ADON = 1;
}

unsigned int ADC_Leer(unsigned char canal){
    ADCON0bits.CHS = canal;
    __delay_us(20);

    ADCON0bits.GO = 1;
    while(ADCON0bits.GO);

    return ((ADRESH << 8) | ADRESL);
}

unsigned int ADC_LeerTemperatura(void){
    unsigned long suma = 0;

    for(int i = 0; i < 10; i++){
        suma += ADC_Leer(0);
        __delay_ms(2);
    }

    unsigned int promedio = suma / 10;

    return (unsigned int)((promedio * 5000UL) / 1023UL);
}

unsigned int ADC_LeerLuz(void){
    unsigned long suma = 0;

    for(int i = 0; i < 10; i++){
        suma += ADC_Leer(1);
        __delay_ms(2);
    }

    unsigned int promedio = suma / 10;

    return (unsigned int)((promedio * 100UL) / 1023UL);
}

unsigned int ADC_LeerMQ135(void){
    unsigned long suma = 0;

    for(int i = 0; i < 10; i++){
        suma += ADC_Leer(2);   // AN2
        __delay_ms(2);
    }

    unsigned int promedio = suma / 10;

    // Convertir a porcentaje relativo (0?100%)
    return (unsigned int)((promedio * 100UL) / 1023UL);
}

// ================= MAIN =================
void main()
{
    OSCCON = 0x72;  // 8MHz interno

    char buffer1[20];
    char buffer2[20];
    unsigned int aire;
    char buffer3[20];

    unsigned int humedad;
    char buffer4[20];
    
    unsigned int temperatura;
    unsigned int luz;

    ADC_Init();
    I2C_Init_Master(I2C_100KHZ);
    OLED_Init();
    
    BME280_Init();

    while(1)
    {
        temperatura = ADC_LeerTemperatura();
        luz = ADC_LeerLuz();
        aire = ADC_LeerMQ135();
        
        humedad = BME280_ReadHumidity();
        
        sprintf(buffer4, "Hum: %d %%", humedad);

        sprintf(buffer3, "Aire: %d %%", aire);


        sprintf(buffer1, "Temp: %.1f C", temperatura / 10.0);
        sprintf(buffer2, "Luz: %d %%", luz);

        OLED_ClearDisplay();

        OLED_SetFont(FONT_1);
        OLED_Write_Text(10, 5, "SENSORES");

        OLED_SetFont(FONT_1);
        OLED_Write_Text(0, 15, buffer1);
        OLED_Write_Text(0, 25, buffer2);
        
        OLED_Write_Text(0, 35, buffer3);
        
        OLED_Write_Text(0, 45, buffer4);
        
        OLED_Update();

        __delay_ms(500);
    }
}
