#ifndef POCC_CONVERTER_H
#define POCC_CONVERTER_H

//#define OSL_GMP_IS_HERE

#include "scoplib/scop.h"
#include "osl/scop.h"

#define CONVERTER_error(msg)                                      \
  do {                                                            \
  fprintf(stderr,"[converter] Error: "msg" (%s).\n", __func__);   \
  exit(1);                                                        \
  }while(0)

#define CONVERTER_warning(msg)                                    \
  do {                                                            \
  fprintf(stderr,"[converter] Warning: "msg" (%s).\n", __func__); \
  }while(0)

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

osl_scop_p      convert_scop_scoplib2osl(scoplib_scop_p);
scoplib_scop_p  convert_scop_osl2scoplib(osl_scop_p);

#endif
