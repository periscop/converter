#ifndef CONVERTER_INT_H
#define CONVERTER_INT_H

#include <stdlib.h> 
#include <limits.h> 
#include <math.h> 


#ifdef OSL_GMP_IS_HERE
#include <gmp.h>
//#elif defined( defined(SCOPLIB_INT_T_IS_MP)
//#include <gmp.h>
#endif

#include "osl/macros.h"
#include "converter/converter.h"

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
* \param[in] dest              address of the destination osl_int_t
* \param[in] src               source scoplib_int_t to convert from
* \return                      void
*
*/
void convert_int_assign_scoplib2osl(int dest_precision, osl_int_p dest,
                                scoplib_int_t src ){

  switch(dest_precision){
    case OSL_PRECISION_SP:
#ifdef SCOPLIB_INT_T_IS_LONG
      dest->sp = (long int)src;
#elif defined(  SCOPLIB_INT_T_IS_LONGLONG)
      if(src > LONG_MAX)
       CONVERTER_error("Conversion of longlong into long. Loss of information");
      else
        dest->sp = (long int)src;  // loss of precision??
#elif defined(  SCOPLIB_INT_T_IS_MP)
      if( !mpz_fits_slong_p(src))
       CONVERTER_error("Conversion of MP-type into long. Loss of information");
      else
        dest->sp = mpz_get_si(src);  // loss of precision??
#endif
    break;

    case OSL_PRECISION_DP:
#ifdef SCOPLIB_INT_T_IS_LONG
      dest->dp = (long int)src;
#elif defined(  SCOPLIB_INT_T_IS_LONGLONG)
      dest->dp = (long long int)src;
#elif defined(  SCOPLIB_INT_T_IS_MP)
      if(!convert_mpz_fits_llong_p(src))
       CONVERTER_error("Conversion of MP-type into longlong. Loss of information");
      else//loss of precision??
      dest->dp = convert_mpz_get_lld(src); 
#endif
    break;

#ifdef OSL_GMP_IS_HERE
    case OSL_PRECISION_MP:
#ifdef SCOPLIB_INT_T_IS_LONG
      mpz_set_si(*(mpz_t *)dest->mp, src);
#elif defined(  SCOPLIB_INT_T_IS_LONGLONG)
      convert_mpz_set_lld(*(mpz_t *)dest->mp, src);
#elif defined(  SCOPLIB_INT_T_IS_MP)
      mpz_set(*(mpz_t*)dest->mp, src);
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
* \param[in] dest               address of scoplib_int_t to assign to 
* \param[in] src_precision      precision of the source osl relation
* \param[in] src                source osl_int_t to assign from 
* \return                       void
*
*/
void convert_int_assign_osl2scoplib(scoplib_int_t *dest,
                                int src_precision, 
                                osl_int_t src){

//TODO: replace all typcasts below with (scoplib_int_t*) ??

#ifdef SCOPLIB_INT_T_IS_LONG
  switch(src_precision){
    case OSL_PRECISION_SP:
      *(long int *)dest = src.sp;
    break;

    case OSL_PRECISION_DP:
      if(src.dp > LONG_MAX)
       CONVERTER_error("Conversion of longlong into long. Loss of information");
      else
        *(long int *)dest = src.dp; //loss of precision?
    break;

#ifdef OSL_GMP_IS_HERE
    case OSL_PRECISION_MP:
      if( !mpz_fits_slong_p(*(mpz_t*)src.mp) )
       CONVERTER_error("Conversion of MP-type into long. Loss of information");
      else //loss of precision?
      *(long int *)dest = mpz_get_si(*(mpz_t*)src.mp);
    break;
#endif  //OSL_GMP_IS_HERE

    default:
      CONVERTER_error("unknown precision for osl");
    }



#elif defined( SCOPLIB_INT_T_IS_LONGLONG)
  switch(src_precision){
    case OSL_PRECISION_SP:
      *(long long int *)dest = src.sp;
    break;

    case OSL_PRECISION_DP:
      *(long long int *)dest = src.dp;
    break;

#ifdef OSL_GMP_IS_HERE
    case OSL_PRECISION_MP:
      if( !convert_mpz_fits_llong_p(*(mpz_t*)src.mp) ){
       CONVERTER_error("Conversion of MP-type into longlong. Loss of information");
	}
      else{ //loss of precision?
      *(long long int *)dest = convert_mpz_get_lld(*(mpz_t*)src.mp); //loss of precision?
	}
    break;
#endif  //OSL_GMP_IS_HERE

    default:
      CONVERTER_error("unknown precision for osl");
    }



#elif defined( SCOPLIB_INT_T_IS_MP)
  switch(src_precision){
    case OSL_PRECISION_SP:
      mpz_set_si(*(mpz_t *)dest, src.sp);
    break;

    case OSL_PRECISION_DP:
      convert_mpz_set_lld(*(mpz_t *)dest, src.dp);
    break;

#ifdef OSL_GMP_IS_HERE
    case OSL_PRECISION_MP:
      mpz_set(*(mpz_t *)dest, *(mpz_t *)src.mp);
    break;
#endif  //OSL_GMP_IS_HERE

    default:
      CONVERTER_error("unknown precision for osl");
    }

#endif

}

#endif //CONVERTER_INT_H
