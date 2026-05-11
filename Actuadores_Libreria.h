#ifndef ACTUADORES_LIBRERIA_H
#define ACTUADORES_LIBRERIA_H

#include <xc.h>

void Actuadores_Init(void);
void LEDs_Cascada(unsigned int luz_porcentaje); 
void Ventilador_SetPWM(unsigned int duty);
void Buzzer_Set(unsigned char estado);

#endif