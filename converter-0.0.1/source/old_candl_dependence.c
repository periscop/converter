//
//  initially copied from Candl
//
   /**------ ( ----------------------------------------------------------**
    **       )\                      CAnDL                               **
    **----- /  ) --------------------------------------------------------**
    **     ( * (                  dependence.c                           **
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
 * terms of the GNU Lesser General Public License as published by the Free    *
 * Software Foundation; either version 3 of the License, or (at your option)  *
 * any later version.                                                         *
 *                                                                            *
 * This software is distributed in the hope that it will be useful, but       *
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY *
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License   *
 * for more details.                                                          *
 *                                                                            *
 * You should have received a copy of the GNU Lesser General Public License   *
 * along with software; if not, write to the Free Software Foundation, Inc.,  *
 * 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA                     *
 *                                                                            *
 * CAnDL, the Chunky Dependence Analyzer                                      *
 * Written by Cedric Bastoul, Cedric.Bastoul@inria.fr                         *
 *                                                                            *
 ******************************************************************************/
/**
 * \file dependence.c
 * \author Cedric Bastoul and Louis-Noel Pouchet
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
//#include <candl/candl.h>

#include "osl/scop.h" //
#include "osl/macros.h" // for access types
#include "osl/extensions/dependence.h" //
#include "converter/old_candl_dependence.h"
#include "converter/converter.h"

#include <assert.h>

#ifdef CANDL_SUPPORTS_ISL
# undef Q // Thank you polylib...
# include <isl/int.h>
# include <isl/constraint.h>
# include <isl/ctx.h>
# include <isl/set.h>
#endif



/******************************************************************************
 *                         Memory deallocation function                       *
 ******************************************************************************/


/* candl_dependence_free function:
 * This function frees the allocated memory for a CandlDependence structure.
 * - 18/09/2003: first version.
 */
void candl_dependence_free(candl_dependence_p dependence)
{
  candl_dependence_p next;

  while (dependence != NULL)
    {
      next = dependence->next;
      //TODO: candl_matrix==scoplib_matrix??
      scoplib_matrix_free(dependence->domain); 
      free(dependence);
      dependence = next;
    }
}


/******************************************************************************
 *                            Memory Allocation functions                     *
 ******************************************************************************/


/**
 * candl_dependence_malloc function:
 * This function allocates the memory space for a CandlDependence structure and
 * sets its fields with default values. Then it returns a pointer to the
 * allocated space.
 * - 07/12/2005: first version.
 */
candl_dependence_p candl_dependence_malloc()
{
  candl_dependence_p dependence;

  /* Memory allocation for the CandlDependence structure. */
  dependence = (candl_dependence_p) malloc(sizeof(CandlDependence));
  if (dependence == NULL)
    CONVERTER_error(" Error: memory overflow");

  /* We set the various fields with default values. */
  dependence->depth        = CANDL_UNSET;
  dependence->type         = CANDL_UNSET;
  dependence->array_id     = CANDL_UNSET;
  dependence->label_source = CANDL_UNSET;
  dependence->label_target = CANDL_UNSET;
  dependence->ref_source   = CANDL_UNSET;
  dependence->ref_target   = CANDL_UNSET;
  dependence->domain       = NULL;
  dependence->next         = NULL;
  dependence->usr	   = NULL;

  return dependence;
}


/******************************************************************************
 *                            Processing functions                            *
 ******************************************************************************/

/*
* copied from:
* candl_program_p candl_program_convert_scop(scoplib_scop_p scop, int** indices)
* TODO: replace mallocs by scoplib_mallocs
*/

int** convert_get_stmt_indices(scoplib_statement_p s, int nparams){

  int i, j, k, l;

  if(s==NULL)
   return NULL;

  scoplib_statement_p stmt = s;

  int nstmt = 0;
  for( ; stmt; stmt = stmt->next, nstmt++)
  ;

  int** index_2d_array = (int**) malloc(nstmt * sizeof(int*));
  
  int max = 0;
  int max_loop_depth = 128;
  int cur_index[max_loop_depth];
  int last[max_loop_depth];

  for (i = 0; i < max_loop_depth; ++i)
    {
      cur_index[i] = i;
      last[i] = 0;
    }

 int idx = 0;
 for( idx=0, stmt=s; stmt; stmt=stmt->next, ++idx){

   int stmt_depth = stmt->domain->elt->NbColumns - 2 - nparams;
   index_2d_array[idx] = (int*) malloc(stmt_depth * sizeof(int));
  
    /* Iterator indices must be computed from the scattering matrix. */
    scoplib_matrix_p m = stmt->schedule;
    if (m == NULL)
      CONVERTER_error("Error: No scheduling matrix and no loop "
  	       "indices specification");
  
    /* FIXME: Sort the statements in their execution order. */
    /* It must be a 2d+1 identity scheduling matrix, and
       statements must be sorted in their execution order. */
    /* Check that it is a identity matrix. */
    int error = 0;
    if (m->NbRows != 2 * stmt_depth + 1)
      error = 1;
    else
      for (l = 0; l < m->NbRows; ++l)
        {
  	for (k = 1; k < m->NbColumns - 1; ++k)
  	  switch (SCOPVAL_get_si(m->p[l][k]))
  	    {
  	    case 0:
  	      if (l % 2 && k == (l / 2) + 1) error = 1;
  	      break;
  	    case 1:
  	      if ((l % 2 && k != (l / 2) + 1) || (! l % 2)) error = 1;
  	      break;
  	    default:
  	      error = 1;
  	      break;
  	    }
  	if (l % 2 && SCOPVAL_get_si(m->p[l][k]))
  	  error = 1;
        }
    if (error)
      CONVERTER_error("Error: schedule is not identity 2d+1 shaped.\n"
  	       "Consider using the <indices> option tag to declare "
  	       " iterator indices");
  
    /* Compute the value of the iterator indices. */
    for (j = 0; j < stmt_depth; ++j)
      {
        int val = SCOPVAL_get_si(m->p[2 * j][m->NbColumns - 1]);
        if (last[j] < val)
  	{
  	  last[j] = val;
  	  for (k = j + 1; k < max_loop_depth; ++k)
  	    last[k] = 0;
  	  for (k = j; k < max_loop_depth; ++k)
  	    cur_index[k] = max + (k - j) + 1;
  	  break;
  	}
      }

    int* index = index_2d_array[idx];
    for (j = 0; j < stmt_depth; ++j)
      index[j] = cur_index[j];


    max = max < cur_index[j - 1] ? cur_index[j - 1] : max;

  } //end for each statement

  return index_2d_array;
}

/**
 * Returns a string containing the dependence, formatted to fit the
 * .scop representation.
 *
 */
static
char*
convert_candl_program_deps_to_string(CandlDependence* dependence)
{
  CandlDependence* tmp = dependence;
  int refs = 0, reft = 0;
  int i, j, k;
  int nb_deps;
  int buffer_size = 2048;
  int szbuff;
  char* buffer = (char*) malloc(buffer_size * sizeof(char));
  char buff[1024];
  char* type;
  char* pbuffer;

  //printf("pointer buffer=%x\n",buffer);
  for (tmp = dependence, nb_deps = 0; tmp; tmp = tmp->next, ++nb_deps)
    ;
  sprintf(buffer, "# Number of dependences\n%d\n", nb_deps);
  if (nb_deps)
    {
      for (tmp = dependence, nb_deps = 1; tmp; tmp = tmp->next, ++nb_deps)
	{
	  /* Compute the type of the dependence, and the array id
	     accessed. */
	  switch (tmp->type)
	    {
	    case CANDL_UNSET:
	      type = "UNSET";
	      break;
	    case CANDL_RAW:
	      type = "RAW #(flow)";
	      refs = tmp->array_id; //already set outside
	      reft = tmp->array_id;
	      break;
	    case CANDL_WAR:
	      type = "WAR #(anti)";
	      refs = tmp->array_id;
	      reft = tmp->array_id;
	      break;
	    case CANDL_WAW:
	      type = "WAW #(output)";
	      refs = tmp->array_id;
	      reft = tmp->array_id;
	      break;
	    case CANDL_RAR:
	      type = "RAR #(input)";
	      refs = tmp->array_id;
	      reft = tmp->array_id;
	      break;
	    case CANDL_RAW_SCALPRIV:
	      type = "RAW_SCALPRIV #(scalar priv)";
	      refs = tmp->array_id;
	      reft = tmp->array_id;
	      break;
	    default:
	      exit(1);
	      break;
	    }
	  /* Quick consistency check. */
	  if (refs != reft)
	    CONVERTER_error("Internal error. refs != reft\n");

	  /* Output dependence information. */
	  sprintf(buff, "# Description of dependence %d\n"
		  "# type\n%s\n# From statement id\n%d\n"
		  "# To statement id\n%d\n# Depth \n%d\n# Array id\n%d\n"
                  "# Source ref index\n%d\n# Target ref index\n%d\n"
		  "# Dependence domain\n%d %d\n", nb_deps, type,
		  tmp->label_source, tmp->label_target, tmp->depth,
		  refs, tmp->ref_source, tmp->ref_target,
                  tmp->domain->NbRows, tmp->domain->NbColumns);
          
	  strcat(buffer, buff);
          
	  /* Output dependence domain. */
	  pbuffer = buffer + strlen(buffer);
	  for (i = 0; i < tmp->domain->NbRows; ++i)
	    {
	      for (j = 0; j < tmp->domain->NbColumns; ++j)
		{
		  sprintf(buff, "%ld ", SCOPVAL_get_si(tmp->domain->p[i][j]));
		  szbuff = strlen(buff);
		  if (szbuff == 2)
		    *(pbuffer++) = ' ';
		  for (k = 0; k < szbuff; ++k)
		    *(pbuffer++) = buff[k];
		}
	      *(pbuffer++) = '\n';
	    }
	  *(pbuffer++) = '\0';
	  /* Increase the buffer size if needed. Conservatively assume a
	     dependence is never larger than 2k. */
	  szbuff = strlen(buffer);
	  if (szbuff + 2048 > buffer_size)
	    {
	      buffer = (char*) realloc(buffer, (buffer_size *= 2) *
				       sizeof(char));
	      if (buffer == NULL)
		CONVERTER_error("Error: memory overflow");
	      buffer[szbuff] = '\0';
	    }
	}
    }

  
  return buffer;
}


/**
 * Update the scop option tag with the dependence list.
 *
* \param[in/out]     scop       scop to be updated
* \param[in]     dependence dependence list
* \return                   void
 */
void convert_candl_dependence_update_scop_with_deps(scoplib_scop_p scop,
					    CandlDependence* dependence)
{
  char* start;
  char* stop;
  char* content;
  char* olddeps = NULL;
  char* newdeps;
  char* newopttags;
  char* curr;
  char* tmp;
  int size = 0;
  int size_newdeps;
  int size_olddeps = 0;
  int size_optiontags;

  start = stop = scop->optiontags;
  /* Get the candl tag, if any. */
  content = scoplib_scop_tag_content(scop, "<candl>", "</candl>");
  if (content)
    {
      /* Get the dependence tag, if any. */
      olddeps = scoplib_scop_tag_content_from_string
	(content, "<dependence-polyhedra>", "</dependence-polyhedra>");
      /* Seek for the correct start/stop characters to insert
	 dependences. */
      if (olddeps)
	{
	  size = size_olddeps = strlen(olddeps);
	  while (start && *start && strncmp(start, olddeps, size))
	    ++start;
	  stop = start + size;
	}
      else
	{
	  size = strlen(content);
	  while (start && *start && strncmp(start, content, size))
	    ++start;
	  stop = start;
	}
    }
      

  /* Convert the program dependences to dotscop representation. */
  newdeps = convert_candl_program_deps_to_string(dependence);

  /* Compute the new size of the full options tags, and allocate a new
     string. */
  size_newdeps = newdeps ? strlen(newdeps) : 0;

  size_optiontags = scop->optiontags ? strlen(scop->optiontags) : 0;

  if (content == NULL)
    size = strlen("<candl>\n") + strlen("</candl>\n") +
      strlen("<dependence-polyhedra>\n")
      + strlen("</dependence-polyhedra>\n");
  else if (olddeps == NULL)
    size = strlen("<dependence-polyhedra>\n") + 
                                    strlen("</dependence-polyhedra>\n");
  else
    size = 0;
  newopttags = (char*) malloc((size_newdeps + size_optiontags
			       - size_olddeps + size + 1)
			      * sizeof(char));


  if (newopttags == NULL)
    CONVERTER_error("Error: memory overflow");
  curr = newopttags;

  /* Copy the beginning of the options. */
  for (tmp = scop->optiontags; tmp != start; ++tmp)
    *(curr++) = *tmp;
  *curr = '\0';
  //printf("pointer curr, after copy start: %x\n", curr);
  /* Copy the candl tags, if needed. */
  if (content == NULL)
    {
      strcat(newopttags, "<candl>\n");
      curr += strlen("<candl>\n");
    }
  if (olddeps == NULL)
    {
      strcat(newopttags, "<dependence-polyhedra>\n");
      curr += strlen("<dependence-polyhedra>\n");
    }


  /* Copy the program dependences. */
  for (tmp = newdeps; tmp && *tmp; ++tmp)
      *(curr++) = *tmp;
  *curr = '\0';



  /* Copy the candl tags, if needed. */
  if (olddeps == NULL)
    {
      strcat(curr, "</dependence-polyhedra>\n");
      curr += strlen("</dependence-polyhedra>\n");
    }
  if (content == NULL)
    {
      strcat(curr, "</candl>\n");
      curr += strlen("</candl>\n");
    }


  /* Copy the remainder of the options. */
  for (tmp = stop; tmp && *tmp; ++tmp)
      *(curr++) = *tmp;
  *curr = '\0';



  if (scop->optiontags)
    free(scop->optiontags);
  scop->optiontags = newopttags;

  /* Be clean. */
  if (content)
    free(content);
  if (olddeps)
    free(olddeps);

  if (newdeps){
    free(newdeps);
  }
  
}




/*
*  Convert osl_scop_dependence into scoplib_scop_dependence
*
*  look for dependency extensions in osl_scop
*  if found, convert them into candl-scoplib_dependencis
*  insert them into the tagoptions part of the scoplib_scop
*
* \param[in] inscop       input osl scop
* \param[in/out] outscop  to be upadated scoplib_scop
* \return                 void
*/
void convert_dep_osl2scoplib(osl_scop_p inscop, scoplib_scop_p outscop){
   int i=0;
   scoplib_statement_p s = NULL;
   scoplib_statement_p t = NULL;

   if(inscop==NULL || outscop==NULL)
      return; //warning?

   //look for candl-osl dependencies inscop
   osl_dependence_p gen_dep = osl_generic_lookup(inscop->extension,
                                               OSL_URI_DEPENDENCE);
   if(gen_dep==NULL)
     return; //no depnedence_structure found


   //convert dependencies
   CandlDependence *cdep = NULL;
   CandlDependence *last = NULL;
   for( ; gen_dep; gen_dep = gen_dep->next){
     

     CandlDependence * tmp = candl_dependence_malloc();

     // get source statement 
     tmp->label_source = gen_dep->label_source;
     for(i=0, s=outscop->statement; i<gen_dep->label_source; i++, s=s->next)
     ;

     // get target statement
     tmp->label_target = gen_dep->label_target;
     for(i=0, t=outscop->statement; i<gen_dep->label_target; i++, t=t->next)
     ;

     // accordint to dep_type, figure out array_s and array_t
     // get type
     scoplib_matrix_p array_s = NULL; //TODO: scoplib_matrix == pipmatrix??
     scoplib_matrix_p array_t = NULL; 
     int s_acc_type = -1;
     int t_acc_type = -1;
     switch(gen_dep->type){
      case OSL_DEPENDENCE_RAW:
        tmp->type = CANDL_RAW;
        array_s = s->write;
        array_t = t->read;
        s_acc_type = OSL_TYPE_WRITE;
        t_acc_type = OSL_TYPE_READ;
      break;

      case OSL_DEPENDENCE_RAR:
        tmp->type = CANDL_RAR;
        array_s = s->read;
        array_t = t->read;
        s_acc_type = OSL_TYPE_READ;
        t_acc_type = OSL_TYPE_READ;
      break;

      case OSL_DEPENDENCE_WAR:
        tmp->type = CANDL_WAR;
        array_s = s->read;
        array_t = t->write;
        s_acc_type = OSL_TYPE_READ;
        t_acc_type = OSL_TYPE_WRITE;
      break;

      case OSL_DEPENDENCE_WAW:
        tmp->type = CANDL_WAW;
        array_s = s->write;
        array_t = t->write;
        s_acc_type = OSL_TYPE_WRITE;
        t_acc_type = OSL_TYPE_WRITE;
      break;

      case OSL_DEPENDENCE_RAW_SCALPRIV:
        tmp->type = CANDL_RAW_SCALPRIV;
        array_s = s->write;
        array_t = t->read;
        s_acc_type = OSL_TYPE_WRITE;
        t_acc_type = OSL_TYPE_READ;
      break;

      default:
        CONVERTER_error("Unknown type for dependency in scoplib_scop\n");
      break;
     }

     //get depth
     tmp->depth = gen_dep->depth;




     int s_ref_index= 0;
     //get array_id
     osl_relation_list_p in_acc = gen_dep->stmt_source_ptr->access;
     for(i=0; in_acc && i< gen_dep->ref_source; i++){

       if(s_acc_type==in_acc->elt->type){
         s_ref_index += in_acc->elt->nb_output_dims==1?
                        in_acc->elt->nb_output_dims :
                        in_acc->elt->nb_output_dims-1;
       } //special case
       else if(s_acc_type==OSL_TYPE_WRITE && 
                                       in_acc->elt->type==OSL_TYPE_MAY_WRITE)
         s_ref_index += in_acc->elt->nb_output_dims==1?
                        in_acc->elt->nb_output_dims :
                        in_acc->elt->nb_output_dims-1;

       in_acc=in_acc->next;
     }
     int src_arr_id = osl_int_get_si(in_acc->elt->precision,
                                     in_acc->elt->m[0], 
                                     in_acc->elt->nb_columns-1);

     int t_ref_index= 0;
     in_acc = gen_dep->stmt_target_ptr->access;
     for(i=0; in_acc && i< gen_dep->ref_target; i++){

       if(t_acc_type==in_acc->elt->type){
         t_ref_index += in_acc->elt->nb_output_dims==1?
                        in_acc->elt->nb_output_dims :
                        in_acc->elt->nb_output_dims-1;
       } //special case
       else if(t_acc_type==OSL_TYPE_WRITE && 
                                       in_acc->elt->type==OSL_TYPE_MAY_WRITE)
         t_ref_index += in_acc->elt->nb_output_dims==1?
                        in_acc->elt->nb_output_dims :
                        in_acc->elt->nb_output_dims-1;

       in_acc=in_acc->next;
     }
     int tgt_arr_id = osl_int_get_si(in_acc->elt->precision,
                                     in_acc->elt->m[0], 
                                     in_acc->elt->nb_columns-1);



     if(src_arr_id != tgt_arr_id)
       CONVERTER_error("osl_dep different src and target references\n");
     else
       tmp->array_id = src_arr_id;




     tmp->ref_source = s_ref_index;
     tmp->ref_target = t_ref_index;  //


     
     tmp->domain = convert_dep_domain_osl2scoplib(gen_dep);
     

     //update the dependence list     
     if(cdep==NULL)
       cdep = last = tmp;
     else{
       last->next = tmp;
       last = last->next;
     }


   }


   //update outscop with dependencies
   convert_candl_dependence_update_scop_with_deps(outscop, cdep);

}


/*
* this functions converts an osl dependence domain to scoplib dep domain
*
* \param[in] in_dep     osl  dependence
* \return               scoplib_matrix containing scoplib dependence domain
*/
scoplib_matrix_p convert_dep_domain_osl2scoplib(osl_dependence_p in_dep){

 // allocate out_domain
 int s_dom_output_dims = in_dep->source_nb_output_dims_domain;
 int s_acc_output_dims = in_dep->source_nb_output_dims_access;
 int t_dom_output_dims = in_dep->target_nb_output_dims_domain;
 int t_acc_output_dims = in_dep->target_nb_output_dims_access;

 /* Compute osl domain indexes */
 int nb_par         = in_dep->stmt_source_ptr->domain->nb_parameters;
 int nb_local_dims  = in_dep->source_nb_local_dims_domain +
                      in_dep->source_nb_local_dims_access +
                      in_dep->target_nb_local_dims_domain +
                      in_dep->target_nb_local_dims_access;
 int nb_output_dims = in_dep->source_nb_output_dims_domain +
                      in_dep->source_nb_output_dims_access;
 int nb_input_dims  = in_dep->target_nb_output_dims_domain +
                      in_dep->target_nb_output_dims_access;

 int osl_ind_source_local_domain = 1 + nb_output_dims + nb_input_dims;
 int osl_ind_source_local_access = osl_ind_source_local_domain +
                                   in_dep->source_nb_local_dims_domain;
 int osl_ind_target_local_domain = osl_ind_source_local_access +
                                   in_dep->source_nb_local_dims_access;
 int osl_ind_target_local_access = osl_ind_target_local_domain +
                                   in_dep->target_nb_local_dims_domain;
 int osl_ind_params              = osl_ind_target_local_access +
                                   in_dep->target_nb_local_dims_access;

 /* Compute scoplib domain indexes */
 // num_output_dims same for osl==scoplib??
 int slib_ind_target_domain      = 1 + in_dep->source_nb_output_dims_domain;
 int slib_ind_params             = slib_ind_target_domain +
                                   in_dep->target_nb_output_dims_domain;

 int rows, cols =0;
 
 int nb_pars = in_dep->stmt_source_ptr->domain->nb_parameters;
 int s_dom_rows =in_dep->stmt_source_ptr->domain->nb_rows;
 int t_dom_rows =in_dep->stmt_target_ptr->domain->nb_rows;
 int s_acc_rows = in_dep->ref_source_access_ptr->nb_rows - 1;
                 
                 
 int depth = in_dep->depth;

 //
 rows = s_dom_rows+t_dom_rows+
      (s_acc_rows==0? 1: s_acc_rows)  //special case for 0-dimention array(scalar)
        +depth; 
 //cols: 2 => eq + const
 cols = s_dom_output_dims+t_dom_output_dims+nb_pars+2;

 scoplib_matrix_p m = scoplib_matrix_malloc(rows, cols);

 int i=0;
 int j=0;
 int osl_constraint = 0;
 int scoplib_constraint = 0;
 int osl_index=0;
 int scoplib_index=0;


 // copy source domain
 osl_relation_p s_domain = in_dep->stmt_source_ptr->domain;
 for(i=0; i< s_domain->nb_rows; i++){

   //copy first column + start of matrix
   for (j=0;j<=s_dom_output_dims; j++)
     convert_int_assign_osl2scoplib(&(m->p[scoplib_constraint][j]),
                                     in_dep->domain->precision,
                                     in_dep->domain->m[osl_constraint],
                                     j );
     

   // copy localdims - not supprted by converter
   if(s_domain->nb_local_dims)
     CONVERTER_error("local dimensions in domain not supproted\n");


   // copy params + constant
   osl_index = osl_ind_params;
   scoplib_index = slib_ind_params;
   for (j=0; j<nb_pars+1; j++)
     convert_int_assign_osl2scoplib(
                                  &(m->p[scoplib_constraint][scoplib_index+j]),
                                  in_dep->domain->precision,
                                  in_dep->domain->m[osl_constraint],
                                  osl_index+j);

   osl_constraint++;
   scoplib_constraint++;
 }

 // copy target domain
 osl_relation_p t_domain = in_dep->stmt_target_ptr->domain;
 for(i=0; i< t_domain->nb_rows; i++){

   //copy first column
   convert_int_assign_osl2scoplib(&(m->p[scoplib_constraint][0]),
                                    in_dep->domain->precision,
                                    in_dep->domain->m[osl_constraint], 0);
   //start of matrix
   osl_index = 1 + nb_output_dims;
   scoplib_index = slib_ind_target_domain;
   for (j=0;j<t_dom_output_dims; j++)
     convert_int_assign_osl2scoplib(
                                  &(m->p[scoplib_constraint][scoplib_index+j]),
                                  in_dep->domain->precision,
                                  in_dep->domain->m[osl_constraint],
                                  osl_index+j);
     
   // copy local dims - not supported in converter
   if(t_domain->nb_local_dims)
     CONVERTER_error("local dimensions in domain not supproted\n");

   // copy params + constant
   osl_index = osl_ind_params;
   scoplib_index = slib_ind_params;
   for (j=0; j<nb_pars+1; j++)
     convert_int_assign_osl2scoplib(
                                  &(m->p[scoplib_constraint][scoplib_index+j]),
                                  in_dep->domain->precision,
                                  in_dep->domain->m[osl_constraint],
                                  osl_index+j);

   scoplib_constraint++;
   osl_constraint++;
 }

 // copy source as well as target access
 int osl_s_index     = 0;
 int osl_t_index     = 0;
 int scoplib_s_index = 0;
 int scoplib_t_index = 0;

 osl_relation_p s_access = in_dep->ref_source_access_ptr;
 osl_relation_p t_access = in_dep->ref_target_access_ptr;

 osl_constraint++; //skip the array_id line
 // s_acc_rows calculated skipping array_id line
 //TODO: see the array_id line skipping again!!
 for(i=0; i < s_acc_rows; i++){

   osl_s_index     = 1;
   osl_t_index     = 1 + nb_output_dims;
   scoplib_s_index = 1;
   scoplib_t_index = slib_ind_target_domain;

   for (j=0; j<s_access->nb_input_dims; j++){
     convert_int_assign_osl2scoplib(
                                 &(m->p[scoplib_constraint][scoplib_s_index+j]),
                                 in_dep->domain->precision,
                                 in_dep->domain->m[osl_constraint],
                                 osl_s_index+j);
   }

   for (j=0; j<t_access->nb_input_dims; j++){ //t_acc_dims==s_acc_dims
     convert_int_assign_osl2scoplib(
                                 &(m->p[scoplib_constraint][scoplib_t_index+j]),
                                 in_dep->domain->precision,
                            in_dep->domain->m[osl_constraint+s_access->nb_rows],
                                 osl_t_index+j);
   }

   //copy local dimensions - not supported by converter
   if(s_access->nb_local_dims || t_access->nb_local_dims)
     CONVERTER_error("local dimensions in Access not supproted\n");

   // copy params + constant
   osl_index = osl_ind_params;
   scoplib_index = slib_ind_params;
   for (j=0; j<nb_pars+1; j++){
     //get src params
     int isrc_param = osl_int_get_si(in_dep->domain->precision,
                              in_dep->domain->m[osl_constraint],
                              osl_index+j);
     //get tgt params
     int itgt_param = osl_int_get_si(in_dep->domain->precision,
                            in_dep->domain->m[osl_constraint+s_access->nb_rows],
                                    osl_index+j);
     //convert ints to scoplib_int_t
     scoplib_int_t tgt_param;
     scoplib_int_t src_param;
     SCOPVAL_init_set_si(tgt_param, itgt_param);
     SCOPVAL_init_set_si(src_param, isrc_param);

     SCOPVAL_oppose(tgt_param, tgt_param);
     //TODO:ask Cédric: why subtraction??
     SCOPVAL_subtract((m->p[scoplib_constraint][scoplib_index+j]),
                      src_param,
                      tgt_param);
   }

   scoplib_constraint++;
   osl_constraint++;
 }

 // copy access equalities -> not in CANDL-scoplib??
 // skip min_depth
 int min_depth = CONVERTER_min(s_access->nb_output_dims, 
                               t_access->nb_output_dims);

 osl_constraint += s_access->nb_rows + min_depth; 
                   
 if(s_acc_rows==0) scoplib_constraint++; //TODO: explain this

 
 // copy depth
 osl_s_index = 1;
 osl_t_index = 1 + nb_output_dims;
 scoplib_s_index = 1;
 scoplib_t_index = slib_ind_target_domain;
 for(i=0; i< depth; i++){
   // copy first column

   convert_int_assign_osl2scoplib(&(m->p[scoplib_constraint][0]),
                                    in_dep->domain->precision,
                                    in_dep->domain->m[osl_constraint], 0);

   // copy subscript equalities
   convert_int_assign_osl2scoplib(&(m->p[scoplib_constraint][scoplib_s_index+i]),
                                    in_dep->domain->precision,
                                    in_dep->domain->m[osl_constraint],
                                    osl_s_index+i);
   convert_int_assign_osl2scoplib(&(m->p[scoplib_constraint][scoplib_t_index+i]),
                                    in_dep->domain->precision,
                                    in_dep->domain->m[osl_constraint],
                                    osl_t_index+i);

   // copy params -> not applicable here

   // copy const == last column
   convert_int_assign_osl2scoplib(&(m->p[scoplib_constraint][m->NbColumns-1]),
                                    in_dep->domain->precision,
                                    in_dep->domain->m[osl_constraint],
                                    in_dep->domain->nb_columns-1);

   osl_constraint++;
   scoplib_constraint++;
 }

 // return new domain
 return m;
}


/*
* for a scoplib reference access
* returns the number of output dimensions (including the array_id)
*
* \param[in] access     scoplib access matrix
* \param[in] ref_index  index for the target array in access matrix
* \return               number of dimensions for target array
*/
int get_scoplib_access_output_dims(scoplib_matrix_p access, int ref_index){

 if( access == NULL )
   return 0;

 /* We calculate the number of dimensions of the considered array. */
 int nb_dimensions = 1; //TODO: check this out!!!!
 while ( ((ref_index + nb_dimensions + 1) <= access->NbRows) &&
         SCOPVAL_zero_p(access->p[ref_index + nb_dimensions][0]) )
 nb_dimensions ++ ;

 //check for zero-dimension array (scalar)
 int i=0;
 int has_dimension = 0;
 if(nb_dimensions == 1)
   for(i=1; i< access->NbColumns; i++)
     if( !SCOPVAL_zero_p(access->p[ref_index][i]) ) //it's non-scalar
       has_dimension = 1; ; //

 if(nb_dimensions > 1) //normal case
   nb_dimensions ++ ;//add one for the array_id dimension
 else if(nb_dimensions==1 && has_dimension) //single dimension array
   nb_dimensions ++ ;//add one for the array_id dimension
 //else //TODO: scalar
  // nb_dimensions ++ ;//add one for the array_id dimension

 return nb_dimensions;
}



/*
* given a reference sequence number n, this function returns the index in the
* scoplib access matrix for the nth reference
* ref_number starts from 1
*
* \param[in] m          scoplib_access_matrix
* \param[in] ref_number sequence number for ref in the matrix
* \return               nth reference's index
*/
int get_scoplib_ref_index( scoplib_matrix_p m, int ref_number){
 int i=0; 
 for(i=0; i<m->NbRows ; i++){
   if(m->p[i][0] != 0)
     ref_number--;
   
   if(ref_number==0)
     break;
 }

 return i;
}


/*
*  given an index, returns the statement at that index
*  index starting from zero
*
* \param[in] in_scop     scop containing the statmeent list
* \param[in] index       statement index
* \return                statemetn at 'index"
*/
scoplib_statement_p get_statement( scoplib_scop_p in_scop, int index){

 if( in_scop == NULL )
   return NULL;

 scoplib_statement_p stmt = in_scop->statement;
 int i=0;
 for( i=0; i< index; i++)
   stmt = stmt->next;

 return stmt;
}



/*
* given a dependence type, returns the correct (read/write)
* access_lists from source/target statements.
*
* \param[in] type       dependence type
* \param[in] s          source statement 
* \param[in] t          target statement 
* \param[out] access_s  list of relevant accesses for source statement 
* \param[out] access_t  list of relevant accesses for target statement 
* \return               void
*/
void get_dependence_accesses (int type,
                              scoplib_statement_p s, scoplib_statement_p t,
                              scoplib_matrix_t **access_s,
                              scoplib_matrix_t ** access_t){

  if(s==NULL || t==NULL)
    CONVERTER_error("Null statement pointer passed\n");

  switch (type)
    {
    case CANDL_RAW:
    case CANDL_RAW_SCALPRIV:
      *access_s = s->write;
      *access_t = t->read;
      break;
    case CANDL_WAR:
      *access_s = s->read;
      *access_t = t->write;
      break;
    case CANDL_WAW:
      *access_s = s->write;
      *access_t = t->write;
      break;
    case CANDL_RAR:
      *access_s = s->read;
      *access_t = t->read;
      break;
    default:
      CONVERTER_error("Unknown dependence type encountered.\n");
      break;
    }

 return;
}



/*
* this function converts a scoplib dependence domain to its osl equivalent
*
* \param[in] in_dep     scoplib dependence
* \param[in] in_dep     scoplib scop
* \return               equivalent coverted osl dependence domain
* return osl_dependence domain
*/
osl_relation_p convert_dep_domain_scoplib2osl(candl_dependence_p in_dep,
                                              scoplib_scop_p  in_scop){

 if(in_dep == NULL)
   return NULL;

 // arrays_output_dims
 scoplib_statement_p stmt_s = get_statement(in_scop, in_dep->label_source);
 scoplib_statement_p stmt_t = get_statement(in_scop, in_dep->label_target);

 // get the corresponding access
 scoplib_matrix_p s_array = NULL;
 scoplib_matrix_p t_array = NULL;
 get_dependence_accesses (in_dep->type, stmt_s, stmt_t, &s_array, &t_array);


 int s_arr_output_dims = get_scoplib_access_output_dims(s_array,
                                            in_dep->ref_source);
 int t_arr_output_dims = get_scoplib_access_output_dims(t_array,
                                            in_dep->ref_target);

 // calculate min_depth
 int min_depth = CONVERTER_min(s_arr_output_dims, t_arr_output_dims);

 // calculate the number of row and cols for osl_domain
 int nb_par = in_scop->context->NbColumns-2;
 int s_dom_output_dims = stmt_s->domain->elt->NbColumns - nb_par -2; 
 int t_dom_output_dims = stmt_t->domain->elt->NbColumns - nb_par -2; 
 int osl_output_dims = s_dom_output_dims + s_arr_output_dims; 
 int osl_input_dims = t_dom_output_dims + t_arr_output_dims;
 int osl_local_dims  = 0;  //local dims not supported by converter


 int nrows = stmt_s->domain->elt->NbRows + stmt_t->domain->elt->NbRows +
             s_arr_output_dims + t_arr_output_dims +
             min_depth + in_dep->depth;

 int ncols =  osl_output_dims +
              osl_input_dims +
              osl_local_dims +
              nb_par + 2;

 
 osl_relation_p m = osl_relation_malloc(nrows, ncols);
 scoplib_matrix_p dom = in_dep->domain;
 

 //calculate certain indexes
 //scoplib 
 int scoplib_t_dom_index  = 1 + s_dom_output_dims;
 int scoplib_param_index = scoplib_t_dom_index + t_dom_output_dims;

 //osl
 int osl_s_acc_index     = 1 + s_dom_output_dims;
 int osl_t_dom_index     = osl_s_acc_index + s_arr_output_dims;
 int osl_t_acc_index     = osl_t_dom_index + t_dom_output_dims;
 int osl_s_loc_dom_index = osl_t_acc_index + t_arr_output_dims;
 int osl_param_index    = osl_s_loc_dom_index + osl_local_dims;


 //fill in the values related to source domain
 int scoplib_constraint = 0; //row index scoplib_domain
 int osl_constraint = 0;     //row index for osl_domain

 int i=0;
 int j=0;

 // fill in source domain
 for(i=0; i< stmt_s->domain->elt->NbRows; i++){
   // copy first column
   // copy matrix start
   int osl_index = 0;
   int scoplib_index = 0;
   for(j=0; j<=s_dom_output_dims; j++){
     convert_int_assign_scoplib2osl(m->precision,
                                    m->m[osl_constraint], osl_index+j,
                           dom->p[scoplib_constraint][scoplib_index+j]);
   }

   // copy local dims - not supported

   // copy params + const
   osl_index = osl_param_index;
   scoplib_index = scoplib_param_index;
   for(j=0; j< nb_par+1; j++){
     convert_int_assign_scoplib2osl(m->precision,
                                    m->m[osl_constraint], osl_index+j,
                           dom->p[scoplib_constraint][scoplib_index+j]);
   }

   osl_constraint++;
   scoplib_constraint++;
 }
	 
 



 //fill in the values related to target domain
 for(i=0; i< stmt_t->domain->elt->NbRows; i++){
   // copy first column
   convert_int_assign_scoplib2osl(m->precision,
                                  m->m[osl_constraint], 0,
                         dom->p[scoplib_constraint][0]);

   // copy matrix start
   int osl_index = osl_t_dom_index;
   int scoplib_index = scoplib_t_dom_index;
   for(j=0; j<t_dom_output_dims; j++){
     convert_int_assign_scoplib2osl(m->precision,
                                    m->m[osl_constraint], osl_index+j,
                           dom->p[scoplib_constraint][scoplib_index+j]);
   }

   // copy local dims - not supported

   // copy params + const
   osl_index = osl_param_index;
   scoplib_index = scoplib_param_index;
   for(j=0; j< nb_par+1; j++){
     convert_int_assign_scoplib2osl(m->precision,
                                    m->m[osl_constraint], osl_index+j,
                           dom->p[scoplib_constraint][scoplib_index+j]);
   }

   osl_constraint++;
   scoplib_constraint++;
 }



 
 
 int scoplib_constraint_backup = scoplib_constraint;
 // fill in values related to source access
 for(i=0; i< s_arr_output_dims; i++){
   if(i==0){   // creating the Array_ID line first
     // copy first column
     osl_int_set_si(m->precision,
                    m->m[osl_constraint], 0,
                    0);
  
     //set the output dims
     int osl_index = osl_s_acc_index;
     osl_int_set_si(m->precision,
                    m->m[osl_constraint], osl_index+i, //using "i"
                    -1);
  
     //copy const
     long int value = -1;
     value = SCOPVAL_get_si(s_array->p[in_dep->ref_source][0]);
     osl_int_set_si(m->precision,
                    m->m[osl_constraint], m->nb_columns-1,
                    value);
  
     osl_constraint++;
   }
   else{  // the rest of the dimensions
     // copy first column
     convert_int_assign_scoplib2osl(m->precision,
                                    m->m[osl_constraint], 0,
                           dom->p[scoplib_constraint][0]);
  
     //set the output dims
     int osl_index = osl_s_acc_index;
     int scoplib_index = -1;
     osl_int_set_si(m->precision,
                    m->m[osl_constraint], osl_index+i, //using "i"
                    -1);
  
     //copy the input dims
     osl_index = 1;
     scoplib_index = 1;
     for(j=0; j< s_dom_output_dims; j++){
       convert_int_assign_scoplib2osl(m->precision,
                                      m->m[osl_constraint], osl_index+j,
                             dom->p[scoplib_constraint][scoplib_index+j]);
     }
  
     //copy the params
     // params and cosnt copied from access relation!!
     osl_index = osl_param_index;
     scoplib_index = scoplib_param_index;
     int scoplib_index_acc_param = s_array->NbColumns - nb_par - 1;//1 -> const
     for(j=0; j< nb_par; j++){
       convert_int_assign_scoplib2osl(m->precision,
                                      m->m[osl_constraint], osl_index+j,
              s_array->p[in_dep->ref_source+i-1][scoplib_index_acc_param+j]);
     }
     
     //copy const
     long int value = -1;
     value = SCOPVAL_get_si(s_array->p[in_dep->ref_source+i-1][s_array->NbColumns-1]);
     osl_int_set_si(m->precision,
                    m->m[osl_constraint], m->nb_columns-1,
                    value);
  
     osl_constraint++;
     scoplib_constraint++;
   }
 }
 



 scoplib_constraint = scoplib_constraint_backup;
 // fill in target access
 for(i=0; i< t_arr_output_dims; i++){
   if(i==0){ // create the Array_ID line first
     // copy first column
     osl_int_set_si(m->precision,
                    m->m[osl_constraint], 0,
                    0);
  
     //set the output dims
     int osl_index = osl_t_acc_index;
     osl_int_set_si(m->precision,
                    m->m[osl_constraint], osl_index+i, //using "i"
                    1);  //1: as target is -ve of source
  
     //copy const
     scoplib_int_t value;
     SCOPVAL_init_set_si(value, -1);
     SCOPVAL_oppose(value, t_array->p[in_dep->ref_target][0]); //target is -ve
     osl_int_set_si(m->precision,
                    m->m[osl_constraint], m->nb_columns-1,
                    SCOPVAL_get_si(value) );
  
     osl_constraint++;
   }
   else{  // rest of the values
     // copy first column
     convert_int_assign_scoplib2osl(m->precision,
                                    m->m[osl_constraint], 0,
                           dom->p[scoplib_constraint][0]);
  
     //set the output dims
     int osl_index = osl_t_acc_index;
     int scoplib_index = -1;
     osl_int_set_si(m->precision,
                    m->m[osl_constraint], osl_index+i, //using "i"
                    1);  //1: as target is -ve of source
  
     //copy the input dims
     osl_index = osl_t_dom_index;
     scoplib_index = scoplib_t_dom_index;
     for(j=0; j< t_dom_output_dims; j++){
       convert_int_assign_scoplib2osl(m->precision,
                                      m->m[osl_constraint], osl_index+j,
                             dom->p[scoplib_constraint][scoplib_index+j]);
     }
  
     //copy the params
     osl_index = osl_param_index;
     scoplib_index = scoplib_param_index;
     int scoplib_index_acc_param = t_array->NbColumns - nb_par - 1;//1 -> const
     for(j=0; j< nb_par; j++){
       convert_int_assign_scoplib2osl(m->precision,
                                      m->m[osl_constraint], osl_index+j,
           t_array->p[in_dep->ref_target+i-1][scoplib_index_acc_param+j]);

       osl_int_oppose(m->precision,
                      m->m[osl_constraint], osl_index+j,
                      m->m[osl_constraint], osl_index+j);
     }
     
     //copy const
     int value = -255;
     if(i==0)
       value = SCOPVAL_get_si(t_array->p[in_dep->ref_target][0]);
     else
       value = SCOPVAL_get_si(t_array->p[in_dep->ref_target+i-1][t_array->NbColumns-1]);
  
     osl_int_set_si(m->precision,
                    m->m[osl_constraint], m->nb_columns-1,
                    value);
     osl_int_oppose(m->precision,
                    m->m[osl_constraint], m->nb_columns-1,
                    m->m[osl_constraint], m->nb_columns-1);
  
     osl_constraint++;
     scoplib_constraint++;
   }
 }




 //fill in the access identities
 int osl_index = osl_s_acc_index;
 int osl_index_2 = osl_t_acc_index;
 for(i=0; i< min_depth; i++){
   osl_int_set_si(m->precision, m->m[osl_constraint], osl_index+i, -1); 
   osl_int_set_si(m->precision, m->m[osl_constraint], osl_index_2+i, 1); 
   osl_constraint++;
 }

 if(s_arr_output_dims == 1)  //TODO: add eq counterpart in reverse direction
   scoplib_constraint++;

 // fill in precedence constraints
 osl_index = 1;
 osl_index_2 = osl_t_dom_index;
 int scoplib_index = 1;
 int scoplib_index_2 = scoplib_t_dom_index;
 for(i=0; i< in_dep->depth; i++){
   // copy first column
   convert_int_assign_scoplib2osl(m->precision,
                                  m->m[osl_constraint], 0,
                         dom->p[scoplib_constraint][0]);


   convert_int_assign_scoplib2osl(m->precision,
                                  m->m[osl_constraint], osl_index+i,
                         dom->p[scoplib_constraint][scoplib_index+i]);
   convert_int_assign_scoplib2osl(m->precision,
                                  m->m[osl_constraint], osl_index_2+i,
                         dom->p[scoplib_constraint][scoplib_index_2+i]);

   //last column
   convert_int_assign_scoplib2osl(m->precision,
                                  m->m[osl_constraint], m->nb_columns-1,
                         dom->p[scoplib_constraint][dom->NbColumns-1]);

   osl_constraint++;
   scoplib_constraint++;
 }
 


 m->nb_parameters = nb_par;
 m->nb_input_dims = osl_input_dims;
 m->nb_output_dims = osl_output_dims;
 m->nb_local_dims =  osl_local_dims;

 return m;
}
