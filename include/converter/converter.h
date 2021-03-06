#ifndef POCC_CONVERTER_H
#define POCC_CONVERTER_H

//#define OSL_GMP_IS_HERE
#include <string.h>  // for strdup

#ifdef OSL_GMP_IS_HERE
#include <gmp.h>
#endif

#include "scoplib/scop.h"
#include "osl/scop.h"
#include "converter/converter_int.h"

#define CONVERTER_error(msg)                                      \
  do {                                                            \
  fprintf(stderr,"[converter] Error: "msg" (%s).\n", __func__);   \
  exit(1);                                                        \
  }while(0)

#define CONVERTER_warning(msg)                                    \
  do {                                                            \
  fprintf(stderr,"[converter] Warning: "msg" (%s).\n", __func__); \
  }while(0)

# define CONVERTER_info(msg)                                               \
         do {                                                              \
           fprintf(stderr,"[converter] Info: "msg" (%s).\n", __func__);    \
         } while (0)

#define CONVERTER_malloc(ptr, type, size)                         \
  do {                                                            \
           if (((ptr) = (type)malloc(size)) == NULL)              \
            CONVERTER_error("memory overflow");                   \
  }while(0)

# define CONVERTER_strdup(destination, source)                      \
  do {                                                              \
    if (source != NULL) {                                           \
      if (((destination) = strdup(source)) == NULL)                 \
        CONVERTER_error("memory overflow");                         \
    }                                                               \
    else {                                                          \
      destination = NULL;                                           \
      CONVERTER_warning("strdup of a NULL string");                 \
    }                                                               \
  } while (0)

# define CONVERTER_max(x,y)    ((x) > (y)? (x) : (y))
# define CONVERTER_min(x,y)    ((x) < (y)? (x) : (y))

osl_scop_p      convert_scop_scoplib2osl(scoplib_scop_p);
scoplib_scop_p  convert_scop_osl2scoplib(osl_scop_p);

int convert_osl_scop_equal( osl_scop_p, osl_scop_p);
int convert_scoplib_scop_equal( scoplib_scop_p, scoplib_scop_p);
#endif
