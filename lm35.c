#pragma config PLLDIV = 2, CPUDIV = OSC1_PLL2, USBDIV = 2
#pragma config FOSC = HSPLL_HS, FCMEN = OFF, IESO = OFF
#pragma config PWRT = OFF, BOR = OFF, BORV = 3, VREGEN = OFF
#pragma config WDT = OFF
#pragma config WDTPS = 32768
#pragma config CCP2MX = ON, PBADEN = OFF, LPT1OSC = OFF, MCLRE = ON
#pragma config STVREN = ON, LVP = OFF, ICPRT = OFF, XINST = OFF
#pragma config CP0 = OFF, CP1 = OFF, CP2 = OFF, CP3 = OFF
#pragma config CPB = OFF, CPD = OFF
#pragma config WRT0 = OFF, WRT1 = OFF, WRT2 = OFF, WRT3 = OFF
#pragma config WRTC = OFF, WRTB = OFF, WRTD = OFF
#pragma config EBTR0 = OFF, EBTR1 = OFF, EBTR2 = OFF, EBTR3 = OFF
#pragma config EBTRB = OFF

#define _XTAL_FREQ 48000000
#include <xc.h>
#include <stdio.h>

#include "i2c.h"
#include "ssd1306_oled.h"

// ================= ADC =================
void ADC_Init()
{
    ADCON1 = 0x0E;        // AN0 analógico, resto digital
    ADCON2 = 0xA9;        // Justificado derecha, Tacq y Fosc/8
    ADCON0 = 0x01;        // Canal AN0, ADC encendido
}

unsigned int ADC_Read()
{
    GO_nDONE = 1;
    while(GO_nDONE);
    return ((ADRESH << 8) + ADRESL);
}

// ================= MAIN =================
void main()
{
    char buffer[20];
    unsigned int adc_val;
    float temperatura;

    TRISAbits.TRISA0 = 1;     // RA0 como entrada (LM35)

    ADC_Init();               // Inicializar ADC

    I2C_Init_Master(I2C_400KHZ);
    OLED_Init();

    while(1)
    {
        // ===== Leer sensor =====
        adc_val = ADC_Read();

        // ===== Convertir a °C =====
        temperatura = (adc_val * 5.0 / 1023.0) * 100.0;

        // ===== Convertir a texto =====
        sprintf(buffer, "Temp: %.2f C", temperatura);

        // ===== Mostrar en OLED =====
        OLED_ClearDisplay();

        OLED_SetFont(FONT_2);
        OLED_Write_Text(10, 20, "TEMPERATURA");

        OLED_SetFont(FONT_1);
        OLED_Write_Text(20, 40, buffer);

        OLED_Update();

        __delay_ms(1000);
    }
}
