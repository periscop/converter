#ifndef POCC_CONVERTER_INT_H
#define POCC_CONVERTER_INT_H

#include "osl/int.h"


void convert_int_assign_scoplib2osl(int, osl_int_p,
                                scoplib_int_t);

void convert_int_assign_osl2scoplib(scoplib_int_t *,
                                int, osl_int_t);

#endif
