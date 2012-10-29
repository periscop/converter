#ifndef POCC_CONVERTER_INT_H
#define POCC_CONVERTER_INT_H

void convert_int_assign_scoplib2osl(int, void*,
                                int, 
                                scoplib_int_t);

void convert_int_assign_osl2scoplib(void*,
                                int, 
                                void*, int);

#endif
