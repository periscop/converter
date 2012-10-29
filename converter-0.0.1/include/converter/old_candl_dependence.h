
   /**------ ( ----------------------------------------------------------**
    **       )\                      CAnDL                               **
    **----- /  ) --------------------------------------------------------**
    **     ( * (                  dependence.h                           **
    **----  \#/  --------------------------------------------------------**
    **    .-"#'-.        First version: september 18th 2003              **
    **--- |"-.-"| -------------------------------------------------------**
          |     |
          |     |
 ******** |     | *************************************************************
 * CAnDL  '-._,-' the Chunky Analyzer for Dependences in Loops (experimental) *
 ******************************************************************************
 *                                                                            *
 * Copyright (C) 2003-2008 Cedric Bastoul                                     *
 *                                                                            *
 * This is free software; you can redistribute it and/or modify it under the  *
 * terms of the GNU General Public License as published by the Free Software  *
 * Foundation; either version 2 of the License, or (at your option) any later *
 * version.                                                                   *
 *                                                                            *
 * This software is distributed in the hope that it will be useful, but       *
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY *
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License   *
 * for more details.                                                          *
 *                                                                            *
 * You should have received a copy of the GNU General Public License along    *
 * with software; if not, write to the Free Software Foundation, Inc.,        *
 * 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA                     *
 *                                                                            *
 * CAnDL, the Chunky Dependence Analyzer                                      *
 * Written by Cedric Bastoul, Cedric.Bastoul@inria.fr                         *
 *                                                                            *
 ******************************************************************************/


#ifndef OLD_CANDL_DEPENDENCE_H
# define OLD_CANDL_DEPENDENCE_H


# include <stdio.h>
//# include <candl/matrix.h>

#include "scoplib/scop.h"  //to define SCOPLIB_SCOP_INT needed for matrix.h
#include "scoplib/matrix.h"



# define CANDL_ARRAY_BUFF_SIZE		2048
# define CANDL_VAR_UNDEF		1
# define CANDL_VAR_IS_DEF		2
# define CANDL_VAR_IS_USED		3
# define CANDL_VAR_IS_DEF_USED		4

//copied from old_candl/candl.h
# define CANDL_UNSET -1 /* Must be negative (we do use that property).
                         * All other constants have to be different.
                         */

# define CANDL_RAW    1
# define CANDL_WAR    2
# define CANDL_WAW    3
# define CANDL_RAR    4
# define CANDL_RAW_SCALPRIV     5




# if defined(__cplusplus)
extern "C"
  {
# endif

typedef scoplib_matrix_t CandlMatrix;

/**
 * CandlDependence structure:
 * this structure contains all the informations about a data dependence, it is
 * also a node of the linked list of all dependences of the dependence graph.
 */
struct candldependence
{
  int depth;                     /**< Dependence level. */
  int type;                      /**< Dependence type: a dependence from source
                                   *   to target can be:
				   *   - CANDL_UNSET if the dependence type is
				   *     still not set,
				   *   - CANDL_RAW if source writes M and
				   *     target read M (flow-dependence),
				   *   - CANDL_WAR if source reads M and
				   *     target writes M (anti-dependence),
				   *   - CANDL_WAW if source writes M and
				   *     target writes M too (output-dependence)
				   *   - CANDL_RAR if source reads M and
				   *     target reads M too (input-dependence).
				   */
  int label_source;                /**< Position of source statemnt. */
  int label_target;                /**< Position of target statement. */
  int ref_source;                /**< Position of source reference. */
  int ref_target;                /**< Position of target reference. */
  int array_id;
  CandlMatrix * domain;          /**< Dependence polyhedron. */

  void* usr;			 /**< User field, for library users
				    convenience. */
  struct candldependence * next; /**< Pointer to next dependence */
};
typedef struct candldependence CandlDependence;
typedef struct candldependence candl_dependence_t;
typedef struct candldependence * candl_dependence_p;


/******************************************************************************
 *                         Memory alloc/dealloc function                      *
 ******************************************************************************/
candl_dependence_p      candl_dependence_malloc();
void			candl_dependence_free(candl_dependence_p);
    
scoplib_matrix_p convert_dep_domain_osl2scoplib(osl_dependence_p);

# if defined(__cplusplus)
  }
# endif
#endif /* define OLD_CANDL_DEPENDENCE_H */

