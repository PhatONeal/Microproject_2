#ifndef ADC_LIBRERIA_H
#define ADC_LIBRERIA_H

#include <xc.h>

void ADC_Init(void);
unsigned int ADC_Leer(unsigned char canal);

#endif /* ADC_LIBRERIA_H */
