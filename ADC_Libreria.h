#ifndef ADC_LIBRERIA_H
#define ADC_LIBRERIA_H

#include <xc.h>

/*
 * =============================================================
 * ADC_Libreria.h - Conversor Analogico a Digital (10 bits)
 * Canales usados:
 *   AN0 (RA0) -> LM35  (temperatura)
 *   AN1 (RA1) -> LDR   (luz ambiente)
 *   AN2 (RA2) -> MQ135 (calidad del aire)
 * =============================================================
 */

void ADC_Init(void);
unsigned int ADC_Leer(unsigned char canal);

#endif /* ADC_LIBRERIA_H */