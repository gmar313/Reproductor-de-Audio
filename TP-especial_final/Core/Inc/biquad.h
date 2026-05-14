#ifndef BIQUAD_H
#define BIQUAD_H

#include <stdlib.h>


/* whatever sample type you want */
typedef float smp_type;




/* this holds the data required to update samples thru a filter */
typedef struct {
	smp_type a0, a1, a2, a3, a4;
	smp_type x1, x2, y1, y2;
}
biquad;


// coeficientes de los filtros
extern biquad filtro_LP;
extern biquad filtro_HP;

extern smp_type BiQuad(const smp_type sample, biquad* const b);


#endif
