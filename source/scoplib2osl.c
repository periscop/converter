#include "converter/converter.h"
#include "converter/old_candl_dependence.h"
#include "osl/scop.h"
#include "osl/body.h"
#include "osl/generic.h"
#include "osl/extensions/arrays.h"
#include "osl/extensions/dependence.h"
#include "osl/interface.h"
#include <string.h>   // warnings for strup in macros.h: incomatible implicit
#include "osl/macros.h"

#include "scoplib/scop.h"
#include "scoplib/matrix.h"

#include "candl/statement.h" //candl_statement_usr_p
#include "candl/util.h" //candl_relation_get_line()

#include <stdio.h> 
#include <stdlib.h> 



/* 
* converts a normal matrix: without output/local/etc dims
*/
osl_relation_p convert_matrix_scoplib2osl( scoplib_matrix_p m){
                                           
  int i=0;
  int j=0;
  osl_relation_p out_rln = NULL;
  if(m==NULL)
    return NULL;

  //allocate according to matrix's precision ??? :^^
  out_rln = osl_relation_malloc(m->NbRows, m->NbColumns);
  out_rln->type = OSL_TYPE_CONTEXT; //TODO: set it outside??

    //calc supplementary dimensions
    out_rln->nb_parameters = m->NbColumns-2;
    out_rln->nb_output_dims = 0;
    out_rln->nb_input_dims = 0;
    out_rln->nb_local_dims = 0;
  
  //copy content
  for(i=0; i< m->NbRows; i++)
    for(j=0; j< m->NbColumns; j++){
      convert_int_assign_scoplib2osl(out_rln->precision, &out_rln->m[i][j],
                                      m->p[i][j] );
    }

  return out_rln;
}


/* 
* converts a domain matrix from scoplib to osl
*/
osl_relation_p convert_domain_scoplib2osl( scoplib_matrix_list_p m_list,
                                            scoplib_matrix_p ctx){
  osl_relation_p out_rln = NULL;
  osl_relation_p last_rl = NULL;
  if(m_list==NULL ||m_list->elt==NULL || ctx==NULL)
    return NULL;

  scoplib_matrix_list_p ml = m_list;
  for( ; ml; ml = ml->next){ 

    scoplib_matrix_p m = ml->elt;
    osl_relation_p rl = convert_matrix_scoplib2osl(ml->elt);
    rl->type = OSL_TYPE_DOMAIN; //TODO: set it outside??
    //rl->precision = //TODO: get it from scoplib

    //calc supplementary dimensions
    rl->nb_parameters = ctx->NbColumns-2;
    rl->nb_output_dims = m->NbColumns-rl->nb_parameters-2; // iteratiors
    rl->nb_input_dims = 0;  // none
    rl->nb_local_dims = 0;    //none
    

    if(out_rln==NULL)
      out_rln=last_rl=rl;
    else{
      last_rl->next = rl;
      last_rl = last_rl->next;
    }
  }

  return out_rln;
}




/*
*  converts a scattering matrix
*
* \param[in] m       scoplib domain
* \param[in] ctx     scoplib scop context)
* \return            osl_relation_p the converted osl domain
*/
osl_relation_p convert_scattering_scoplib2osl( scoplib_matrix_p m,
                                                scoplib_matrix_p ctx){

  int i=0;
  int j=0;
  int nb_parameters=0;
  int nb_output_dims=0;
  int nb_input_dims=0;
  int nb_local_dims=0;

  osl_relation_p out_rln = NULL;
  if(m==NULL || ctx==NULL)
    return NULL;

  //calc supplementary dimensions
  nb_parameters = ctx->NbColumns-2;

  nb_output_dims = m->NbRows; // equals number of matrix rows

  nb_input_dims =  m->NbColumns-nb_parameters-2;  // none
  nb_local_dims = 0;    //none
  
  //alloc - precision set in the malloc function
  int nb_cols = nb_parameters+nb_output_dims+nb_input_dims+nb_local_dims+2;
  int nb_rows = m->NbRows;
  out_rln = osl_relation_malloc(nb_rows, nb_cols);
  out_rln->type = OSL_TYPE_SCATTERING; //TODO: set it outside??
  out_rln->nb_parameters = nb_parameters;
  out_rln->nb_output_dims = nb_output_dims; // iteratiors
  out_rln->nb_input_dims = nb_input_dims;  // none
  out_rln->nb_local_dims = nb_local_dims;    //none
  //out_rln->precision = //TODO: get it from scoplib

  //copy content
  for(i=0; i< out_rln->nb_rows; i++)
    for(j=0; j< out_rln->nb_columns; j++){
      if(j==0)  // eq/neq (first) column
        convert_int_assign_scoplib2osl(out_rln->precision,&out_rln->m[i][j],
                                                m->p[i][j]); 
      else if(j>0 && j<=(nb_output_dims)) // for output dimentions
        if( j==(i+1) )   //identification diagonal
          {
          scoplib_int_t dummy;
          SCOPVAL_init_set_si(dummy, -1);
          convert_int_assign_scoplib2osl(out_rln->precision,&out_rln->m[i][j],
                                             dummy); 
          SCOPVAL_clear(dummy);
          }
        else  // non diagonal zeros
          {
          scoplib_int_t dummy;
          SCOPVAL_init_set_si(dummy, 0);
          convert_int_assign_scoplib2osl(out_rln->precision,&out_rln->m[i][j],
                                             dummy); 
          SCOPVAL_clear(dummy);
          }
      else  // input dimensions + parameters + constant
          convert_int_assign_scoplib2osl(out_rln->precision,&out_rln->m[i][j],
                                            m->p[i][j-nb_output_dims]); 
    }

  return out_rln;
}

/*
*  Determine heuristically, whether a given reference is an array or a variable
*   In scoplib matrix, [   12    0    0    0    0    0 ]
*   it is not clear whether A=...., or A[0]=....
*   We try to look at all the references to A (id=12 here) in the scop,
*   and we try to see if it is accessed as A[x] (x!=0) in any statement.
*   If yes, we convert the above matrix into A[0].
*   Otherwise, we translate it as A=..., though there is still a slight chance
*   that it is A[0] and used as A[0] in all its occrences!
*  Converter issues a warning in such a conversion.
*
* \param[in] id      the identifier for the array reference
* \param[in] in_stmt pointer to a list of statements to search in
* \return            1 if the reference is an array, 0 otherwise
*/
int is_array(int id, scoplib_statement_p in_stmt){

  int is_array = 0;

  if(in_stmt==NULL)
    CONVERTER_error("NULL statemnet pointer passed when searching for array\n");

  while(in_stmt){

    // check in read accesses
    if(in_stmt->read != NULL){
      int i=0;
      for(i=0; i< in_stmt->read->NbRows; i++){ //for all rows
        if( SCOPVAL_get_si(in_stmt->read->p[i][0]) == id){  //if found our id
          //check for multidimensionality
          if(i< in_stmt->read->NbRows-1 && 
                                  SCOPVAL_zero_p(in_stmt->read->p[i+1][0]) ){
              is_array=1;
          } //end if not last row

          //one dimensional array; single row entry in matrix
          int j=0;
          for(j=1; j< in_stmt->read->NbColumns; j++){//search after the 1st col
            if( !SCOPVAL_zero_p(in_stmt->read->p[i][j]) ) //1st index != 0
              is_array=1;
          }

        } //end if our type of id
      } //end for all rows
    } // end if read!=NULL

    // check in write accesses
    if(in_stmt->write != NULL){
      int i=0;
      for(i=0; i< in_stmt->write->NbRows; i++){ //for all rows
        if(SCOPVAL_get_si(in_stmt->write->p[i][0]) == id){  //if found our id
          //check for multidimensionality
          if(i< in_stmt->write->NbRows-1 && 
                             SCOPVAL_zero_p(in_stmt->write->p[i+1][0]) ){
              is_array=1;
          } //end if not last row

          //one dimensional array; single row entry in matrix
          int j=0;
          for(j=1; j< in_stmt->write->NbColumns; j++){//after the 1st col
            if( !SCOPVAL_zero_p(in_stmt->write->p[i][j]) ) //1st index != 0
              is_array=1;
          }

        } //end if our type of id
      } //end for all rows
    } // end if write!=NULL


    in_stmt = in_stmt->next; // go to next statemnet
  }

  // a return value of zero is not 100% sure (see description above)
  // issue a warning when using that value
  return is_array;
}

/*
* given a scoplib access matrix containing access details, this fucntion
* extracts info(array_id, nb_dims, is_scalar)  about each array accessed
*
* \param[in] m       scoplib access matrix
* \param[in] in_stmt statements to search if the nature of access is doubtful
* \param[out] num_arrays holds the number of accesses in matrix
* \param[out] array_identities array_ids of the accesses in the matrix
* \param[out] array_dimensions number of dimensions for each array
* \param[out] is_array_bool    1/0 whether the access is a vector/scalar
* \return            number of arrays detected in the input matrix
*/

int convert_get_scoplib_access_dimensions( scoplib_matrix_p m,
                                     scoplib_statement_p in_stmt, //needed for array search
                                     int *num_arrays,
                                     int **array_identities,
                                     int **array_dimensions,
                                     int **is_array_booleans){

  int nb_arrays=0;

  if(m==NULL)
    return nb_arrays;
  
  int i=0;
  for(i=0; i < m->NbRows; i++) //count number of access arrays 
    if( !SCOPVAL_zero_p(m->p[i][0]) ){
      nb_arrays++;
    }

  *num_arrays = nb_arrays;


  int *array_id = malloc(nb_arrays*sizeof(int));
  int *array_has_dims = malloc(nb_arrays*sizeof(int));
  int *array_dims = malloc(nb_arrays*sizeof(int));

  *array_identities = array_id;
  *array_dimensions = array_dims;
  *is_array_booleans= array_has_dims;


  for(i=0; i< nb_arrays; i++) 
    array_has_dims[i]=0;

  //get array infos
  int array_start = 0;
  nb_arrays=0;
  array_id[nb_arrays] = SCOPVAL_get_si(m->p[0][0]);  // get array_id

  //check for zero/one/multi-dimonsional arrays
  if(m->NbRows>1 && SCOPVAL_zero_p(m->p[1][0]) ){ //multi-dimensions
    array_has_dims[nb_arrays] = 1;
  }
  else{ //last row, or single_row entry in matrix; at-most one dimension
    int tmp=0;
    int j=0;
    for(j=1; j< m->NbColumns; j++)
      if(SCOPVAL_get_si(m->p[0][j]) != 0 ) //1st dimension != 0
        tmp=1;

    if(tmp==0) // differentiate b/w:  A=...  &   A[0]=....
      tmp = is_array(SCOPVAL_get_si(m->p[0][0]), in_stmt);

    if(tmp==0) //possible zero-dimension
      CONVERTER_warning("Guessing access is zero-dimensional array\n");

    array_has_dims[nb_arrays] = tmp;
  }

      

  for(i=1; i < m->NbRows; i++){ //findout the dimensionn of each array
    if( !SCOPVAL_zero_p(m->p[i][0]) ){
      array_dims[nb_arrays++] = i - array_start; //curr. array dims
      array_id[nb_arrays] = SCOPVAL_get_si(m->p[i][0]); //next array_id

      if(i<m->NbRows-1 && m->p[i+1][0] == 0){ //multi-dimensional case
        array_has_dims[nb_arrays] = 1;
      }
      else{ //last row, or single_row entry in matrix; atmost one dimensin
        int tmp=0;
        int j=0;
        for(j=1; j< m->NbColumns; j++)
          if( !SCOPVAL_zero_p(m->p[i][j]) ) //1st dimension != [0]
            tmp=1;

        if(tmp==0) // differentiate b/w:  A=...  &   A[0]=....
          tmp = is_array(SCOPVAL_get_si(m->p[i][0]), in_stmt);

        if(tmp==0) //possibly zero-dimensional array
          CONVERTER_warning("Guessing access is zero-dimensional array\n");

        array_has_dims[nb_arrays] = tmp;
      }
      


      array_start = i;
    }
  }
  array_dims[nb_arrays++] = i - array_start; //last array dims
  

  return nb_arrays;
}


/*
* converts access matrix
*
* \param[in] in_matrix  scoplib access matrix to convert
* \param[in] ctx        scoplib context needed to calculate nb_parameters
* \param[in] in_stmt    statements to verify vector/scalar nature of acccess
* \param[in] type       scoplib access matrix type: read/write
* \return               new osl access relations list
*/
osl_relation_list_p convert_access_scoplib2osl( scoplib_matrix_p in_matrix,
                                                scoplib_matrix_p ctx,
                                                scoplib_statement_p in_stmt,
                                                int type){

  int i=0;
  int j=0;
  int k=0;
  int nb_parameters=0;
  int nb_output_dims=0;
  int nb_input_dims=0;
  int nb_local_dims=0;

  osl_relation_list_p out_rln_list = NULL;
  osl_relation_list_p last_rl_list = NULL;
  scoplib_matrix_p m = NULL;
  if(in_matrix==NULL || ctx==NULL)
    return NULL;

  //Empty matrix
  if(in_matrix->NbRows==0)
    return NULL;

  //calc supplementary dimensions
  nb_parameters = ctx->NbColumns-2;

  m = in_matrix;
  nb_output_dims = 0; // 
  


  int num_arrays=0;
  int * array_id = NULL;
  int * array_has_dims = NULL;
  int * array_dims = NULL;
  convert_get_scoplib_access_dimensions(in_matrix, in_stmt, &num_arrays,
                               &array_id, &array_dims,
                               &array_has_dims);




  //copy each array into a separate access relation
  int m_row = 0;  // scoplib matrix's row_count
  osl_relation_p rl = NULL;
  for(i=0; i< num_arrays; i++){


    //set parameters
    nb_parameters = ctx->NbColumns -2;
    nb_output_dims = array_dims[i]+array_has_dims[i]; //one for the array_id
    nb_local_dims = 0;
    nb_input_dims = m->NbColumns-nb_parameters-2;

    int nb_cols = nb_parameters+nb_output_dims+nb_local_dims+nb_input_dims+2;
    int nb_rows = array_dims[i]+array_has_dims[i];
    //out_rln->precision = //TODO: get it from scoplib
    rl = osl_relation_malloc(nb_rows, nb_cols);
    rl->type = type; //TODO: set it outside??
    rl->nb_parameters = nb_parameters;
    rl->nb_output_dims = nb_output_dims; // 
    rl->nb_input_dims = nb_input_dims;  // 
    rl->nb_local_dims = nb_local_dims;    //none





    // copy the array_id line
    // takes care of access relation for zero-dimensional array as well

    for(k=0; k< nb_cols; k++){
      if(k==0)  // eq/neq (first) column
        {
        scoplib_int_t dummy;
        SCOPVAL_init_set_si(dummy, 0);
        convert_int_assign_scoplib2osl(rl->precision, &rl->m[0][k],
                                           dummy);
        SCOPVAL_clear(dummy);
        }
      else if( k==(0+1) )   // Arr
        {
        scoplib_int_t dummy;
        SCOPVAL_init_set_si(dummy, -1);
        convert_int_assign_scoplib2osl(rl->precision, &rl->m[0][k],
                                           dummy);
        SCOPVAL_clear(dummy);
        }
      else if(k==(nb_cols-1))  //Arr_id
        {
        scoplib_int_t dummy;
        SCOPVAL_init_set_si(dummy, array_id[i]);
        convert_int_assign_scoplib2osl(rl->precision, &rl->m[0][k], 
                                            dummy);
        SCOPVAL_clear(dummy);
        }
      else // non diagonal zeros
        {
        scoplib_int_t dummy;
        SCOPVAL_init_set_si(dummy, 0);
        convert_int_assign_scoplib2osl(rl->precision, &rl->m[0][k],
                                           dummy);
        SCOPVAL_clear(dummy);
        }
    }
     
    if(array_has_dims[i]){ //one/multi-dimensional case
      //for each row
      for(j=0; j< array_dims[i]; j++,m_row++){
          //for each column
        for(k=0; k< nb_cols; k++){
          if(k==0)  // eq/neq (first) column
            {
            scoplib_int_t dummy;
            SCOPVAL_init_set_si(dummy, 0);
            convert_int_assign_scoplib2osl(rl->precision, &rl->m[j+1][k],
                                               dummy);
            SCOPVAL_clear(dummy);
            }
          else if(k>0 && k<=(nb_output_dims)){ // for output dimentions
            if( k==(j+2) )   //identification diagonal
              {
              scoplib_int_t dummy;
              SCOPVAL_init_set_si(dummy, -1);
              convert_int_assign_scoplib2osl(rl->precision, &rl->m[j+1][k],
                                                dummy);
              SCOPVAL_clear(dummy);
              }
            else  // non diagonal zeros
              {
              scoplib_int_t dummy;
              SCOPVAL_init_set_si(dummy, 0);
              convert_int_assign_scoplib2osl(rl->precision, &rl->m[j+1][k],
                                                dummy);
              SCOPVAL_clear(dummy);
              }
          }
          else  // input dimensions + parameters + constant
            convert_int_assign_scoplib2osl(rl->precision, &rl->m[j+1][k],
                                              m->p[m_row][k-nb_output_dims]);
        }

      } // end for each row

    }  // end if(array_has_dims)
    else{ // zero-dimensional array handling
        m_row++;
    }// if(array_has_dims)



    osl_relation_list_p tmp_rl_list = osl_relation_list_malloc();
    tmp_rl_list->elt = rl;
    if(out_rln_list==NULL) {
      out_rln_list=last_rl_list=tmp_rl_list;
    }
    else{
      last_rl_list->next = tmp_rl_list;
      last_rl_list = last_rl_list->next;
    }
  
  } //for num_arrays

  // free the above allocated arrays
  if(array_id != NULL)
    free(array_id);
  if(array_has_dims != NULL)
    free(array_has_dims);
  if(array_dims != NULL)
    free(array_dims);

  return out_rln_list;
}

/*
* converts scoplib statement body to osl generic
*
* \param[in] nb_iterators number of iterators for the statement
* \param[in] iter         an array of iterator-names strings
* \param[in] body         string containing statement body
* \return                 osl generic encapsulating the statment body
*/
osl_generic_p convert_body_scoplib2osl(int nb_iterators, char **iter,
                                      char* body){
  int i=0;
  if(iter==NULL && body==NULL)
    return NULL;

  osl_generic_p out_gen = osl_generic_malloc();
  osl_body_p out_body = osl_body_malloc();
  
  //iterators
  char** iters_cpy = NULL;
  OSL_malloc(iters_cpy, char **, sizeof(char *) * (nb_iterators+ 1)); 
  iters_cpy[nb_iterators] = NULL;
  if(iter!=NULL && *iter!=NULL){
    for(i=0; i< nb_iterators; i++)
      CONVERTER_strdup(iters_cpy[i], (const char*)iter[i]);
  }
  out_body->iterators = osl_strings_malloc();
  out_body->iterators->string = iters_cpy;
  
  //expression
  if(body!=NULL){
    char* expr_cpy = NULL;
    OSL_strdup(expr_cpy, body);
    out_body->expression = osl_strings_encapsulate(expr_cpy);
  }

  out_gen->data = out_body;
  out_gen->interface = osl_body_interface();
  
  //osl_body_print(stdout, out_body);
  return out_gen;
}



/*
* this functions converts a scoplib statement to the osl equivalent
*
* \param[in] in_stmt    scoplib statement to be converted
* \param[in] ctx        scoplib context needed to calculate nb_parameters
* \return               new osl statement
*/
osl_statement_p convert_statement_scoplib2osl( scoplib_statement_p in_stmt,
                                               scoplib_matrix_p in_ctx){
  if(in_stmt==NULL)
    return NULL;

  //allocate
  osl_statement_p out_stmt = NULL;
  osl_statement_p last_stmt = NULL;
  osl_statement_p tmp_stmt = NULL;

  scoplib_statement_p s = in_stmt;
  for(; s; s= s->next){

    tmp_stmt = osl_statement_malloc();

    //domain
    tmp_stmt->domain = convert_domain_scoplib2osl(s->domain, in_ctx);

    //scattering
    tmp_stmt->scattering = convert_scattering_scoplib2osl(s->schedule, in_ctx);

    //access  //TODO: make generic append(read) append(write) ?
    tmp_stmt->access = convert_access_scoplib2osl(s->read, in_ctx, in_stmt,
                                                  OSL_TYPE_READ);

    osl_relation_list_p tmp_rl = tmp_stmt->access;
    for(; tmp_rl && tmp_rl->next; ) 
      tmp_rl = tmp_rl->next;
    if(!tmp_rl){
      tmp_stmt->access = convert_access_scoplib2osl(s->write, in_ctx, in_stmt, 
                                                          OSL_TYPE_WRITE);
}
    else{
      tmp_rl->next = convert_access_scoplib2osl(s->write, in_ctx, in_stmt, 
                                                         OSL_TYPE_WRITE);
    }

    //body
    tmp_stmt->body = convert_body_scoplib2osl(s->nb_iterators, s->iterators,
                                              s->body);

    //usr
    tmp_stmt->usr = NULL;

    //next
    if(out_stmt==NULL){
      out_stmt=last_stmt=tmp_stmt;
    }
    else{
      last_stmt->next = tmp_stmt;
      last_stmt = last_stmt->next;
    }

  }

  return out_stmt;
}


/*
* this function tests the equivalence of two osl relation_lists
*
* \param[in] l1      osl relation_list
* \param[in] l2      osl relation_list
* \return            1 if the two arguments are deemed equal, 0 otherwise
*/
int convert_osl_relation_list_equal(osl_relation_list_p l1,
                                        osl_relation_list_p l2) {

  int l1_size = osl_relation_list_count(l1);
  int l2_size = osl_relation_list_count(l2);
  
  if(l1_size != l2_size){
    CONVERTER_error("relation_list sizes are not the same\n");
    //return 0;
  }

  for(; l1; l1 = l1->next){
    osl_relation_list_p l_tmp = l2;
    while (l_tmp != NULL) {
      
      if (osl_relation_equal(l1->elt, l_tmp->elt))
        break;

      l_tmp = l_tmp->next;
    }
    if(l_tmp==NULL)  // relation not found
      return 0; 
  }

    return 1;
}


/*
* this functions checks the equality of two osl statements
*
* \param[in] s1      list of osl statements
* \param[in] s2      list of osl statements
* \return            1 if the two arguments are deemed equal, 0 otherwise
*/
int convert_osl_statement_equal(osl_statement_p s1, osl_statement_p s2) {
  
  if (s1 == s2)
    return 1;
  
  if (((s1->next != NULL) && (s2->next == NULL)) ||
      ((s1->next == NULL) && (s2->next != NULL))) {
    CONVERTER_info("number of statements is not the same"); 
    return 0;
  }

  if ((s1->next != NULL) && (s2->next != NULL)) {
    if (!convert_osl_statement_equal(s1->next, s2->next)) {
      CONVERTER_info("statements are not the same"); 
      return 0;
    }
  }
    
  if (!osl_relation_equal(s1->domain, s2->domain)) {
    CONVERTER_info("statement domains are not the same"); 
    osl_relation_print(stdout, s1->domain);
    osl_relation_print(stdout, s2->domain);
    return 0;
  }

  if (!osl_relation_equal(s1->scattering, s2->scattering)) {
    CONVERTER_info("statement scatterings are not the same"); 
    osl_relation_print(stdout, s1->scattering);
    osl_relation_print(stdout, s2->scattering);
    return 0;
  }

  if (!convert_osl_relation_list_equal(s1->access, s2->access)) {
    CONVERTER_info("statement accesses are not the same"); 
    osl_relation_list_print(stdout, s1->access);
    osl_relation_list_print(stdout, s2->access);
    return 0;
  }

  if (!osl_generic_equal(s1->body, s2->body)) {
    CONVERTER_info("statement bodies are not the same"); 
    osl_generic_print(stdout, s1->body);
    osl_generic_print(stdout, s2->body);
    return 0;
  }

  return 1;
}


/*
* this functions checks the equality of two osl scops
*
* \param[in] s1      osl scop
* \param[in] s2      osl scop
* \return            1 if the two arguments are deemed equal, 0 otherwise
*/
int convert_osl_scop_equal(osl_scop_p s1, osl_scop_p s2) {

  while ((s1 != NULL) && (s2 != NULL)) {
    if (s1 == s2)
      return 1;

    if (s1->version != s2->version) {
      CONVERTER_info("versions are not the same"); 
      return 0;
    }
    //else
    //  printf("osl_scop versions equal\n");

    if (strcmp(s1->language, s2->language) != 0) {
      CONVERTER_info("languages are not the same"); 
      return 0;
    }
    //else
    //  printf("osl_scop language equal\n");

    if (!osl_relation_equal(s1->context, s2->context)) {
      CONVERTER_info("contexts are not the same"); 
      return 0;
    }
    //else
    //  printf("osl_scop context equal\n");

    if (!osl_generic_equal(s1->parameters, s2->parameters)) {
      CONVERTER_info("parameters are not the same"); 
      return 0;
    }
    //else
    //  printf("osl_scop parameters equal\n");

    if (!convert_osl_statement_equal(s1->statement, s2->statement)) {
      CONVERTER_info("statements are not the same"); 
      return 0;
    }
    //else
    //  printf("osl_scop statements equal\n");

    //TODO: below
    //if (!osl_interface_equal(s1->registry, s2->registry)) {
     // CONVERTER_info("registries are not the same"); 
      //return 0;
    //}
    //else
     // printf("osl_scop registry equal\n");

    //if (!osl_generic_equal(s1->extension, s2->extension)) {
     // CONVERTER_info("extensions are not the same"); 
      //return 0;
    //}
    //else
     // printf("osl_scop extensions equal\n");

    s1 = s1->next;
    s2 = s2->next;
  }
  
  if (((s1 == NULL) && (s2 != NULL)) || ((s1 != NULL) && (s2 == NULL)))
    return 0;

  return 1;
}


/*
* returns the number of refrences in read/write access until row
*
* \param[in] m          scoplib access matrix
* \param[in] row        number of row to search until (starting from 0)
* \return               number of references found until 'row'th row
*/
static
int get_num_refs(scoplib_matrix_p m, int row){
  if(m==NULL)
    return -1;

  int num_ref = 0;
  int i = 0;
  for(i=0; i<=row; i++){
    if( !SCOPVAL_zero_p(m->p[i][0]) )
      num_ref++;
  }

  return num_ref;
}



/*
*  Read a scoplib dependence from a char_stream 
*  Convert it into osl_dependence
*
*  This functions assumes that the statements have already been converted!!
*
* \param[in] str      char stream containing the dependence
* \param[in] next     store the updated char stream after reading the dependence
* \param[in] in_scop  scop containing dependences to be converted
* \param[in] out_scop scop in which converted dependences will be stored
*                     both scops are used to access access related info
* \return             converted osl dependence
*
* 
*/
static
osl_dependence_p convert_candl_dependence_read_one_dep_2(char* str, char** next,
					       scoplib_scop_p in_scop,
                                               osl_scop_p out_scop)
{
  candl_dependence_p old_dep = candl_dependence_malloc();
  osl_dependence_p dep = osl_dependence_malloc();
  char buffer[1024];

  int i, j, k;
  int id;
  int id2;
  /* # Description of dependence xxx */
  for (; *str != '\n'; ++str);
  ++str;

  /* # type */
  for (; *str != '\n'; ++str);
  ++str;

  /* {RAW,RAR,WAR,WAW} #(type) */
  for (i = 0; *str != ' '; ++str, ++i)
    buffer[i] = *str;
  buffer[i] = '\0';
  
  int s_acc_type = -1;
  int t_acc_type = -1;
  if (! strcmp(buffer, "RAW")){
    dep->type = OSL_DEPENDENCE_RAW;
    old_dep->type = OSL_DEPENDENCE_RAW;
    s_acc_type = OSL_TYPE_WRITE;
    t_acc_type = OSL_TYPE_READ;
  }
  else if (! strcmp(buffer, "RAR")){
    dep->type = OSL_DEPENDENCE_RAR;
    old_dep->type = OSL_DEPENDENCE_RAR;
    s_acc_type = OSL_TYPE_READ;
    t_acc_type = OSL_TYPE_READ;
  }
  else if (! strcmp(buffer, "WAR")){
    dep->type = OSL_DEPENDENCE_WAR;
    old_dep->type = OSL_DEPENDENCE_WAR;
    s_acc_type = OSL_TYPE_READ;
    t_acc_type = OSL_TYPE_WRITE;
  }
  else if (! strcmp(buffer, "WAW")){
    dep->type = OSL_DEPENDENCE_WAW;
    old_dep->type = OSL_DEPENDENCE_WAW;
    s_acc_type = OSL_TYPE_WRITE;
    t_acc_type = OSL_TYPE_WRITE;
  }
  else if (! strcmp(buffer, "RAW_SCALPRIV")){
    dep->type = OSL_DEPENDENCE_RAW_SCALPRIV;
    old_dep->type = OSL_DEPENDENCE_RAW_SCALPRIV;
    s_acc_type = OSL_TYPE_WRITE;
    t_acc_type = OSL_TYPE_READ;
  }

  for (; *str != '\n'; ++str);
  ++str;


  /* # From statement xxx */
  for (; *str != '\n'; ++str);
  ++str;
  /* stmt-id */
  for (i = 0; *str != '\n'; ++str, ++i)
    buffer[i] = *str;
  ++str;
  buffer[i] = '\0';
  id = atoi(buffer);
  
  //ASSUMING that stmt sequence remains the same!!!!
  osl_statement_p s = out_scop->statement;
  scoplib_statement_p scoplib_s_stmt = in_scop->statement;
  for (i = 0; i < id; ++i){
    s=s->next;
    scoplib_s_stmt = scoplib_s_stmt->next;
  }
  if(s==NULL)
    printf("Coundnt find Statement\n");
  else{
    dep->stmt_source_ptr = s;
    dep->label_source = id;
    old_dep->label_source = id;
  }
  

  /* # To statement xxx */
  for (; *str != '\n'; ++str);
  ++str;
  /* stmt-id */
  for (i = 0; *str != '\n'; ++str, ++i)
    buffer[i] = *str;
  ++str;
  buffer[i] = '\0';
  id = atoi(buffer);
  
  s = out_scop->statement;
  scoplib_statement_p scoplib_t_stmt = in_scop->statement;
  for (i = 0; i < id; ++i){
    s=s->next;
    scoplib_t_stmt = scoplib_t_stmt->next;
  }
  if(s==NULL)
    printf("Coundnt find Statement\n");
  else{
    dep->stmt_target_ptr = s;
    dep->label_target = id;
    old_dep->label_target = id;
  }
  


  /* # Depth */
  for (; *str != '\n'; ++str);
  ++str;
  /* depth */
  for (i = 0; *str != '\n'; ++str, ++i)
    buffer[i] = *str;
  ++str;
  buffer[i] = '\0';
  dep->depth = atoi(buffer);
  old_dep->depth = atoi(buffer);

  /* # Array id */
  for (; *str != '\n'; ++str);
  ++str;
  /* array-id */
  for (i = 0; *str != '\n'; ++str, ++i)
    buffer[i] = *str;
  ++str;
  buffer[i] = '\0';
  id = atoi(buffer);
  old_dep->array_id = id;

  /* # src ref */
  for (; *str != '\n'; ++str);
  ++str;
  /* src ref */
  for (i = 0; *str != '\n'; ++str, ++i)
    buffer[i] = *str;
  ++str;
  buffer[i] = '\0';
  int src_ref = atoi(buffer);
  old_dep->ref_source = src_ref;

  /* # tgt ref */
  for (; *str != '\n'; ++str);
  ++str;
  /* tgt ref */
  for (i = 0; *str != '\n'; ++str, ++i)
    buffer[i] = *str;
  ++str;
  buffer[i] = '\0';
  int tgt_ref = atoi(buffer);
  old_dep->ref_target = tgt_ref;


  int precision = out_scop->context->precision;
  // source reference
  // normally the access list order in converter is:
  // read->write->maywrite
  int osl_s_ref_index = -1;
  int osl_t_ref_index = -1;
  if(s_acc_type==OSL_TYPE_READ)
    osl_s_ref_index = get_num_refs(scoplib_s_stmt->read,
                                   old_dep->ref_source);
  else
    osl_s_ref_index = get_num_refs(scoplib_s_stmt->read,
                          scoplib_s_stmt->read->NbRows-1) +
                      get_num_refs(scoplib_s_stmt->write,
                          old_dep->ref_source);

  dep->ref_source = osl_s_ref_index-1;
  osl_relation_list_p tmp_access = dep->stmt_source_ptr->access;
  for(i=0; i< dep->ref_source; i++)
    tmp_access = tmp_access->next;
  dep->ref_source_access_ptr = tmp_access->elt;

  //target reference
  // normally the access list order in converter is:
  // read->write->maywrite
  if(t_acc_type==OSL_TYPE_READ)
    osl_t_ref_index = get_num_refs(scoplib_t_stmt->read,
                                   old_dep->ref_target);
  else
    osl_t_ref_index = get_num_refs(scoplib_t_stmt->read,
                          scoplib_t_stmt->read->NbRows-1) +
                      get_num_refs(scoplib_t_stmt->write,
                          old_dep->ref_target);

  dep->ref_target = osl_t_ref_index-1;
  tmp_access = dep->stmt_target_ptr->access;
  for(i=0; i< dep->ref_target; i++)
    tmp_access = tmp_access->next;
  dep->ref_target_access_ptr = tmp_access->elt;
  

  /* # Dependence domain */
  for (; *str != '\n'; ++str);
  ++str;

  /* nb-row nb-col */
  for (i = 0; *str != ' '; ++str, ++i)
    buffer[i] = *str;
  ++str;
  buffer[i] = '\0';
  id = atoi(buffer);
  for (i = 0; *str != '\n'; ++str, ++i)
    buffer[i] = *str;
  ++str;
  buffer[i] = '\0';
  id2 = atoi(buffer);

  // allocate memory for domain
  old_dep->domain = scoplib_matrix_malloc(id, id2);
  /* Read matrix elements. */
  for (j = 0; j < id; ++j)
    {
    for (k = 0; k < id2; ++k)
      {
	while (*str && *str == ' ')
	  str++;
	for (i = 0; *str != '\n' && *str != ' '; ++str, ++i)
	  buffer[i] = *str;
	buffer[i] = '\0';
	++str;
	SCOPVAL_set_si(old_dep->domain->p[j][k], atoi(buffer));
      }
    if (*(str - 1) != '\n')
      {
	for (; *str != '\n'; ++str);
	++str;
      }
    }
  /* Store the next character in the string to be analyzed. */
  *next = str;



  /*  convert the domain */
  dep->domain = convert_dep_domain_scoplib2osl( old_dep, in_scop);

  

  /* Compute the system size */
  dep->source_nb_output_dims_domain = 
                                dep->stmt_source_ptr->domain->nb_output_dims;
  dep->source_nb_output_dims_access = 
                                dep->ref_source_access_ptr->nb_output_dims;
  
  dep->target_nb_output_dims_domain = 
                                dep->stmt_target_ptr->domain->nb_output_dims;
  dep->target_nb_output_dims_access = 
                                dep->ref_target_access_ptr->nb_output_dims;
  
  dep->source_nb_local_dims_domain  = 
                                dep->stmt_source_ptr->domain->nb_local_dims;
  dep->source_nb_local_dims_access  = 
                                dep->ref_source_access_ptr->nb_local_dims;
  dep->target_nb_local_dims_domain  = 
                                dep->stmt_target_ptr->domain->nb_local_dims;
  dep->target_nb_local_dims_access  = 
                                dep->ref_target_access_ptr->nb_local_dims;
    

  candl_dependence_free(old_dep);

  return dep;
}

/*
*  Extract dependence from scoplib_scop tagoptions
*  convert them one by one in osl dependences
*  encapsulate them into an osl_generic
*
*  ASSUMPTION: 1. Statements have already been converted been scops
*              2. The order of the statements is the same
*
* \param[in] in_scop    scoplib scop containing dependences
* \param[in] out_scop   osl scop : needed by dep_read_2()
* \return               generic containing list of converted osl dependences
*/
osl_generic_p convert_dep_scoplib2osl(scoplib_scop_p in_scop, osl_scop_p out_scop){

  osl_dependence_p first = NULL;
  osl_dependence_p currdep = NULL;

  if(in_scop == NULL)
    return NULL;

  //initialize the usr structure with depth, indices, etc.
  //the candl_statement_usr struct in statement->usr
  candl_statement_usr_init_all(out_scop);

  //search for candl tags in in_scop

  //below code copied from candl_dependence_read_from_scop()
  //candl_scoplib: dependence.c

  //search for dependence-polyhedra tags
  char* deps = scoplib_scop_tag_content(in_scop,
                                        "<dependence-polyhedra>",
                                        "</dependence-polyhedra>");

  //if not found dep tags
  if(deps == NULL)
    return NULL;
  
  /* Keep the starting address of the array. */               
  char* base = deps;                                          
                                                            
  int i;                                                      
  int depcount;                                               
  /* Get the number of dependences. */                        
  char buffer_nb[32];                                         
  /* # Number of dependences */                               
  for (; *deps != '\n'; ++deps);                              

  ++deps;                                                     
  for (i = 0; *deps != '\n'; ++i, ++deps)                     
    buffer_nb[i] = *deps;                                     
  buffer_nb[i] = '\0';                                        
  int nbdeps = atoi (buffer_nb);                              
  
  char* next;                                                 
                                                              
  ++deps;                                                     
  /* For each of them, read 1 and shift of the read size. */  
  for (depcount = 0; depcount < nbdeps; ++depcount)           
    {                                                         
      osl_dependence_p adep =                                 
        convert_candl_dependence_read_one_dep_2(deps, &next, in_scop, out_scop);  
      if (first == NULL)                                      
        currdep = first = adep;                               
      else                                                    
        {                                                     
          currdep->next = adep;                               
          currdep = currdep->next;                            
        }                                                     
      deps = next;                                            
    }                                                         
                                                              
  /* Be clean. */                                             
  free(base);                                                 
                                                              
  osl_generic_p ext = NULL;
  if(first!=NULL){
     ext = osl_generic_shell(first, osl_dependence_interface());
  }
  return ext;                                               
}


/*
* Translates  scoplib_scop to osl_scop
*  : pointer to osl_scop if successful
*
* \param[in] in_scop    scoplib scop to convert
* \return               new osl scop, NULL if unsuccessful
*
*TODO: later extend it to scop_linked_list
*/
osl_scop_p   convert_scop_scoplib2osl( scoplib_scop_p inscop){

  int i=0;
  if(inscop==NULL){
    CONVERTER_warning("Received NULL scop\n");
    return NULL;    //
  }

  //scop integrety check
  //if(){}

  //allocate output_scop
  osl_scop_p output_scop = NULL;
  osl_scop_p tmp_scop = osl_scop_malloc();


  //context
  if(inscop->context){
    tmp_scop->context = convert_matrix_scoplib2osl(inscop->context);
  }
  else{
    return output_scop; //NULL
  }

  //parameters
  if(inscop->nb_parameters > 0 && inscop->parameters!=NULL ){

    char** params_cpy = NULL;
    OSL_malloc(params_cpy, char **, 
    sizeof(char *) * (inscop->nb_parameters+ 1)); 
    params_cpy[inscop->nb_parameters] = NULL;
    for(i=0; i< inscop->nb_parameters; i++){
      OSL_strdup(params_cpy[i], inscop->parameters[i]);
    }
    osl_strings_p params = osl_strings_malloc();
    params->string = params_cpy;
    
    tmp_scop->parameters = osl_generic_malloc();
    tmp_scop->parameters->data = params;
    tmp_scop->parameters->interface = osl_strings_interface();
  }
  else
    CONVERTER_warning("No parameters detected\n");
  
  //statement
  if(inscop->statement){
    tmp_scop->statement = convert_statement_scoplib2osl(inscop->statement,
                                                        inscop->context);
  }
  else{
    CONVERTER_error("NULL statement in inscop\n");
    return output_scop; //NULL
  }


  //optiontags
  //arrays
  char* content = scoplib_scop_tag_content (inscop, "<arrays>", "</arrays>");
  if(content==NULL)
    tmp_scop->extension = NULL;
  else{ // create an extension
    char* content_cpy = content;
    osl_arrays_p arrays = osl_arrays_sread( &content_cpy );
    tmp_scop->extension = osl_generic_malloc();
    tmp_scop->extension->data = arrays;
    tmp_scop->extension->interface = osl_arrays_interface();
  }

  if(content!=NULL);
    free(content);


  //get dependence dependence
  osl_generic_p ext = convert_dep_scoplib2osl(inscop, tmp_scop);
  ext->next = tmp_scop->extension;
  tmp_scop->extension = ext;
  
  output_scop = tmp_scop;
  return output_scop;
}
