#ifndef CONVERTER_INT_H
#define CONVERTER_INT_H

#include "osl/macros.h"
#include "converter/converter.h"
#include <stdlib.h> 
#include <limits.h> 
#include <math.h> 


#ifdef OSL_GMP_IS_HERE
#include <gmp.h>
//#elif defined( defined(SCOPLIB_INT_T_IS_MP)
//#include <gmp.h>
#endif


#ifdef OSL_GMP_IS_HERE
/*
* Function to check if mpz_t fits inside a long long int
*
* \param[in] x     mpz_t to check
* \return          1 if yes, 0 other wise
* http://cpansearch.perl.org/src/BUBAFLUB/Math-Primality-0.04/spec/bpsw/trn.c
*/

int convert_mpz_fits_llong_p(mpz_t x){

  mpz_t llMax;
  char *szULL = NULL;
  CONVERTER_malloc(szULL, char *, 2 + ceil(log10(LLONG_MAX)));
  mpz_init2(llMax, floor(0.5 + log10(LLONG_MAX)/log10(2)));
  sprintf(szULL, "%lld", LLONG_MAX);
  mpz_set_str(llMax, szULL, 10);
	//(mpz_sgn(x) >= 0) &&
  if( (mpz_cmp(x, llMax) > 0)){
    free(szULL);
    return 0;
  }
  else { // what about LLONG_MIN??
    free(szULL);
    return 1;
  }
}


/*
* Function to convert an mpz into a long long int
*
* \param[in] x     mpz_t  to convert
* \return          converted long long int
*/

long long int convert_mpz_get_lld(mpz_t x){

  char *szLLD = NULL;
  CONVERTER_malloc(szLLD, char *, 2 + ceil(log10(LLONG_MAX)));
  if(szLLD){
    mpz_get_str(szLLD, 10, x);
    unsigned long long lld=strtoll(szLLD, NULL, 10);
    free(szLLD);
    return lld;
  }
  else{
    free(szLLD);
    return 0LL;
  }
}




//http://cpansearch.perl.org/src/BUBAFLUB/Math-Primality-0.04/spec/bpsw/trn.c
// see also: http://www.velocityreviews.com/forums/t317412-limits-h-and-int-len.html

//mpz type declares an array of size 1; therefoer the name of the variable
//is a pointer.
//declaring a simple pointer necessitates allocation of the object explicitly;
//while declaring an array of size one creates the objects statically and lets
//use the variable name as a pointer at the same time!

/*
* Function to assign a long long it to an mpz_t
*
* \param[in] x     mpz_t  to convert
* \return          converted long long int
*/

void convert_mpz_set_lld(mpz_t mpz, long long int x){

  char *szULL=NULL;
    //(char *)malloc(3 + (sizeof(unsigned long long)*CHAR_BIT*3)/10);
    //(char *)malloc(sizeof (long long int) * CHAR_BIT * 8 / 25 + 3);
  CONVERTER_malloc(szULL, char *, sizeof (long long int) * CHAR_BIT * 8 / 25 + 3);
  sprintf(szULL, "%lld", x);
  mpz_set_str(mpz, szULL, 10);
  free(szULL);
  return;
}
#endif //OSL_GMP_IS_HERE


/*
*  Function used in copying scoplib_matrix to a osl_relation.
*  Assigns a value in scoplib_matrix, to an offset in osl_relation 
*
*
* \param[in] dest_precision    precision of the destination osl relation
* \param[in] dest_value_base   base address in the relation (row start)
* \param[in] dest_value_offset offset in the relation (column position)
* \param[in] src               source scoplib_int to convert from
* \return                      void
*
*  Usage: convert_int_assign_scoplib2osl(osl_precision, osl_base, osl_offset,
                                      scoplib_matrix[row][col]);
*/
void convert_int_assign_scoplib2osl(int dest_precision, void* dest_value_base,
                                int dest_value_offset, 
                                scoplib_int_t src ){

  void * dest_value = osl_int_address(dest_precision, dest_value_base, dest_value_offset);

  switch(dest_precision){
    case OSL_PRECISION_SP:
#ifdef SCOPLIB_INT_T_IS_LONG
      *(long int *)dest_value = (long int)src;
#elif defined(  SCOPLIB_INT_T_IS_LONGLONG)
      if(src > LONG_MAX)
       CONVERTER_error("Conversion of longlong into long. Loss of information");
      else
        *(long int *)dest_value = (long int)src;  // loss of precision??
#elif defined(  SCOPLIB_INT_T_IS_MP)
      if( !mpz_fits_slong_p(src))
       CONVERTER_error("Conversion of MP-type into long. Loss of information");
      else
        *(long int *)dest_value = mpz_get_si(src);  // loss of precision??
#endif
    break;

    case OSL_PRECISION_DP:
#ifdef SCOPLIB_INT_T_IS_LONG
      *(long long int *)dest_value = (long int)src;
#elif defined(  SCOPLIB_INT_T_IS_LONGLONG)
      *(long long int *)dest_value = (long long int)src;
#elif defined(  SCOPLIB_INT_T_IS_MP)
      if(!convert_mpz_fits_llong_p(src))
       CONVERTER_error("Conversion of MP-type into longlong. Loss of information");
      else//loss of precision??
      *(long long int *)dest_value = convert_mpz_get_lld(src); 
#endif
    break;

#ifdef OSL_GMP_IS_HERE
    case OSL_PRECISION_MP:
#ifdef SCOPLIB_INT_T_IS_LONG
      mpz_set_si(*(mpz_t *)dest_value, src);
#elif defined(  SCOPLIB_INT_T_IS_LONGLONG)
      convert_mpz_set_lld(*(mpz_t *)dest_value, src);
#elif defined(  SCOPLIB_INT_T_IS_MP)
      mpz_set(*(mpz_t *)dest_value, src);
#endif
    break;
#endif  //OSL_GMP_IS_HERE

    default:
      CONVERTER_error("unknown precision for osl");

  }
}


/*
*
*  Function used in copying osl_relation to a scoplib_matrix.
*  Assigns a value at an offset in osl_relation, to a value in scoplib_matrix 
*
*
* \param[in] dest_base_value    scoplib matrix address to assign to 
* \param[in] src_precision      precision of the source osl relation
* \param[in] src_value_base     base address in the source osl relation (row address)
* \param[in] src_value_offset   offset after the base address (column position)
* \return                       void
*
*  Usage: convert_int_assign_osl2scoplib(&scoplib_matrix[row][col],
                                    osl_precision, osl_base, osl_offset);
*/
void convert_int_assign_osl2scoplib(void* dest_value_base,
                                int src_precision, 
                                void* src_value_base, int src_value_offset){

  scoplib_int_t * dest_value = (scoplib_int_t*)dest_value_base;
  void * src_value = osl_int_address(src_precision, 
                      src_value_base, src_value_offset);

//TODO: replace all typcasts below with (scoplib_int_t*) ??

#ifdef SCOPLIB_INT_T_IS_LONG
  switch(src_precision){
    case OSL_PRECISION_SP:
      *(long int *)dest_value = *(long int*)src_value;
    break;

    case OSL_PRECISION_DP:
      if(*(long long int*)src_value > LONG_MAX)
       CONVERTER_error("Conversion of longlong into long. Loss of information");
      else
        *(long int *)dest_value = *(long int*)src_value; //loss of precision?
    break;

#ifdef OSL_GMP_IS_HERE
    case OSL_PRECISION_MP:
      if( !mpz_fits_slong_p(*(mpz_t*)src_value) )
       CONVERTER_error("Conversion of MP-type into long. Loss of information");
      else //loss of precision?
      *(long int *)dest_value = mpz_get_si(*(mpz_t*)src_value);
    break;
#endif  //OSL_GMP_IS_HERE

    default:
      CONVERTER_error("unknown precision for osl");
    }



#elif defined( SCOPLIB_INT_T_IS_LONGLONG)
  switch(src_precision){
    case OSL_PRECISION_SP:
      *(long long int *)dest_value = *(long int*)src_value;
    break;

    case OSL_PRECISION_DP:
      *(long long int *)dest_value = *(long long int*)src_value;
    break;

#ifdef OSL_GMP_IS_HERE
    case OSL_PRECISION_MP:
      if( !convert_mpz_fits_llong_p(*(mpz_t*)src_value) ){
       CONVERTER_error("Conversion of MP-type into longlong. Loss of information");
	}
      else{ //loss of precision?
      *(long long int *)dest_value = convert_mpz_get_lld(*(mpz_t*)src_value); //loss of precision?
	}
    break;
#endif  //OSL_GMP_IS_HERE

    default:
      CONVERTER_error("unknown precision for osl");
    }



#elif defined( SCOPLIB_INT_T_IS_MP)
  switch(src_precision){
    case OSL_PRECISION_SP:
      mpz_set_si(*(mpz_t *)dest_value, *(long int*)src_value);
    break;

    case OSL_PRECISION_DP:
      convert_mpz_set_lld(*(mpz_t *)dest_value, *(long long int *)src_value);
    break;

#ifdef OSL_GMP_IS_HERE
    case OSL_PRECISION_MP:
      //printf("convertint mp -> mp\n");
      mpz_set(*(mpz_t *)dest_value, *(mpz_t *)src_value);
    break;
#endif  //OSL_GMP_IS_HERE

    default:
      CONVERTER_error("unknown precision for osl");
    }

#endif

}

#endif //CONVERTER_INT_H
