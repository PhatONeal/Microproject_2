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

#define _XTAL_FREQ 8000000
#include <xc.h>
#include <stdio.h>

#include "i2c.h"
#include "ssd1306_oled.h"

// ================= ADC =================
void ADC_Init(void){
    ADCON1 = 0x0D;   // AN0 y AN1 analógicos
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

// ================= MAIN =================
void main()
{
    OSCCON = 0x72;  // 8MHz interno

    char buffer1[20];
    char buffer2[20];

    unsigned int temperatura;
    unsigned int luz;

    ADC_Init();
    I2C_Init_Master(I2C_400KHZ);
    OLED_Init();

    while(1)
    {
        temperatura = ADC_LeerTemperatura();
        luz = ADC_LeerLuz();

        sprintf(buffer1, "Temp: %.1f C", temperatura / 10.0);
        sprintf(buffer2, "Luz: %d %%", luz);

        OLED_ClearDisplay();

        OLED_SetFont(FONT_2);
        OLED_Write_Text(10, 5, "SENSORES");

        OLED_SetFont(FONT_1);
        OLED_Write_Text(0, 25, buffer1);
        OLED_Write_Text(0, 45, buffer2);

        OLED_Update();

        __delay_ms(500);
    }
}
