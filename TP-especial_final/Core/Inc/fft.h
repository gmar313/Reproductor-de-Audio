/*
 * fft.h
 *
 *
 *
 */

#ifndef INC_FFT_H_
#define INC_FFT_H_

#include "stdint.h"

typedef struct
{
float real;
float imag;
}cmpx;

extern uint8_t log_table[];

/* Función para realizar la FFT de N puntos
 * La salida se carga en el arreglo Y pasado por referencia */
void FFT(cmpx *Y, int N);

#endif /* INC_FFT_H_ */
