#include "converter/converter.h"
#include "osl/scop.h"
#include "osl/body.h"
#include "osl/generic.h"
#include "osl/extensions/arrays.h"
#include <string.h>   // warnings for strup in macros.h: incomatible implicit
#include "osl/macros.h"

#include "scoplib/scop.h"
#include "scoplib/matrix.h"

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
    //out_rln->precision = //TODO: get it from scoplib
  //printf("osl precision = %d\n", out_rln->precision);

    //calc supplementary dimensions
    out_rln->nb_parameters = m->NbColumns-2;
    out_rln->nb_output_dims = 0;
    out_rln->nb_input_dims = 0;
    out_rln->nb_local_dims = 0;
  
  //copy content
  for(i=0; i< m->NbRows; i++)
    for(j=0; j< m->NbColumns; j++){
      //osl_int_set_si(out_rln->precision, out_rln->m[i], j,
       //                              SCOPVAL_get_si(m->p[i][j]) );
      convert_int_assign_scoplib2osl(out_rln->precision, out_rln->m[i], j,
                                      m->p[i][j] );
    }

  return out_rln;
}


/* 
* converts a domain matrix
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
    //scoplib_matrix_print(stdout,m);
    //allocate accroding to matrix's precision ??
    //osl_relation_p rl = osl_relation_malloc(m->NbRows, m->NbColumns);
    osl_relation_p rl = convert_matrix_scoplib2osl(ml->elt);
    rl->type = OSL_TYPE_DOMAIN; //TODO: set it outside??
    //rl->precision = //TODO: get it from scoplib

    //calc supplementary dimensions
    rl->nb_parameters = ctx->NbColumns-2;
    rl->nb_output_dims = m->NbColumns-rl->nb_parameters-2; // iteratiors
    rl->nb_input_dims = 0;  // none
    rl->nb_local_dims = 0;    //none
    
    //copy content
    //int i=0;
    //int j=0;
    //for(i=0; i< m->NbRows; i++)
     // for(j=0; j< m->NbColumns; j++){
      //  osl_int_set_si(rl->precision, rl->m[i], j, m->p[i][j]);
      //}

    //osl_relation_print(stdout, rl);

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
        //osl_int_set_si(out_rln->precision, out_rln->m[i], j, m->p[i][j]);
        convert_int_assign_scoplib2osl(out_rln->precision,out_rln->m[i], j,
                                                m->p[i][j]); 
      else if(j>0 && j<=(nb_output_dims)) // for output dimentions
        if( j==(i+1) )   //identification diagonal
          //osl_int_set_si(out_rln->precision, out_rln->m[i], j, -1);
          convert_int_assign_scoplib2osl(out_rln->precision,out_rln->m[i], j,
                                             (scoplib_int_t)-1); 
        else  // non diagonal zeros
          //osl_int_set_si(out_rln->precision, out_rln->m[i], j, 0);
          convert_int_assign_scoplib2osl(out_rln->precision,out_rln->m[i], j,
                                             (scoplib_int_t)0); 
      else  // input dimensions + parameters + constant
        //osl_int_set_si(out_rln->precision, out_rln->m[i], j, m->p[i][j-nb_output_dims]);
          convert_int_assign_scoplib2osl(out_rln->precision,out_rln->m[i], j,
                                            m->p[i][j-nb_output_dims]); 
    }

  return out_rln;
}

/*
*  Determine heuristically, whether a given reference is an array or a variable
   In scoplib matrix, [   12    0    0    0    0    0 ]
   it is not clear whether A=...., or A[0]=....
   We try to look at all the references to A (id=12 here) in the scop,
   and we try to see if it is accessed as A[x] (x!=0) in any statement.
   If yes, we convert the above matrix into A[0].
   Otherwise, we translate it as A=..., though there is still a slight chance
   that it is A[0] and used as A[0] in all its occrences! Converter should 
   issue a warning in such a conversion.
*
*
*
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
        if(in_stmt->read->p[i][0] == id){  //if found our id
          //check for multidimensionality
          if(i< in_stmt->read->NbRows-1 && in_stmt->read->p[i+1][0] == 0){
              is_array=1;
          } //end if not last row

          //one dimensional array; single row entry in matrix
          int j=0;
          for(j=1; j< in_stmt->read->NbColumns; j++){//search after the 1st col
            if(in_stmt->read->p[i][j] != 0) //non-zero first-dimension access
              is_array=1;
          }

        } //end if our type of id
      } //end for all rows
    } // end if read!=NULL

    // check in write accesses
    if(in_stmt->write != NULL){
      int i=0;
      for(i=0; i< in_stmt->write->NbRows; i++){ //for all rows
        if(in_stmt->write->p[i][0] == id){  //if found our id
          //check for multidimensionality
          if(i< in_stmt->write->NbRows-1 && in_stmt->write->p[i+1][0] == 0){
              is_array=1;
          } //end if not last row

          //one dimensional array; single row entry in matrix
          int j=0;
          for(j=1; j< in_stmt->write->NbColumns; j++){//after the 1st col
            if(in_stmt->write->p[i][j] != 0) //non-zero first-dimension access
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
* get array_access dimensions info from scoplib_read/write matrix
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
    if(m->p[i][0] != 0){
      nb_arrays++;
      }
  //printf("nb_arrays %d\n", nb_arrays);
  

  *num_arrays = nb_arrays;


  int *array_id = malloc(nb_arrays*sizeof(int));
  int *array_has_dims = malloc(nb_arrays*sizeof(int));
  int *array_dims = malloc(nb_arrays*sizeof(int));

  *array_identities = array_id;
  *array_dimensions = array_dims;
  *is_array_booleans= array_has_dims;


  for(i=0; i< nb_arrays; i++) array_has_dims[i]=0;



  //get array infos
  int array_start = 0;
  nb_arrays=0;
  array_id[nb_arrays] = SCOPVAL_get_si(m->p[0][0]);  // get array_id

  //check for zero/one/multi-dimonsional arrays
  if(m->NbRows>1 && m->p[1][0] == 0){ //multi-dimensions
    array_has_dims[nb_arrays] = 1;
  }
  else{ //last row, or single_row entry in matrix; at-most one dimension
    int tmp=0;
    int j=0;
    for(j=1; j< m->NbColumns; j++)
      if(m->p[0][j] != 0 ) //non-zero first dimention discovered
        tmp=1;

    if(tmp==0) // differentiate b/w:  A=...  &   A[0]=....
      tmp = is_array(m->p[0][0], in_stmt);

    if(tmp==0) //possible zero-dimension
      CONVERTER_warning("Guessing access is zero-dimensional array\n");

    array_has_dims[nb_arrays] = tmp;
  }

      

  for(i=1; i < m->NbRows; i++){ //findout the dimensionn of each array
    if(m->p[i][0] != 0){
      array_dims[nb_arrays++] = i - array_start; //curr. array dims
      array_id[nb_arrays] = SCOPVAL_get_si(m->p[i][0]); //next array_id


      //int tmp=0;
      //for(j=1; j< m->NbColumns-1; j++) //NbColumns ou NbColumns-1?
       // if(m->p[i][j] != 0 )
        //  tmp=1;

      //if(i<m->NbRows-1){
       // if(m->p[i+1][0] == 0)
        //  array_has_dims[nb_arrays] = 1;
        //else
         // array_has_dims[nb_arrays] = tmp;
      //}
      //else
       // array_has_dims[nb_arrays] = tmp;
      if(i<m->NbRows-1 && m->p[i+1][0] == 0){ //multi-dimensional case
        array_has_dims[nb_arrays] = 1;
      }
      else{ //last row, or single_row entry in matrix; atmost one dimensin
        int tmp=0;
        int j=0;
        for(j=1; j< m->NbColumns; j++)
          if(m->p[i][j] != 0 ) //non-zero first dimention discovered
            tmp=1;

        if(tmp==0) // differentiate b/w:  A=...  &   A[0]=....
          tmp = is_array(m->p[i][0], in_stmt);

        if(tmp==0) //possibly zero-dimensional array
          CONVERTER_warning("Guessing access is zero-dimensional array\n");

        array_has_dims[nb_arrays] = tmp;
      }
      


      array_start = i;
    }
  }
  array_dims[nb_arrays++] = i - array_start; //last array dims
  
  //for(i=0; i < nb_arrays; i++){ //count number of access arrays 
   // printf("array %d: %d: %d\n", array_id[i], array_dims[i], array_has_dims[i]);
  //}

  return nb_arrays;
}


/*
* converts access matrix
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
    //printf("arr %d: num_rows %d, num_columns %d\n", i, nb_rows,nb_cols);
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
        //osl_int_set_si(rl->precision, rl->m[j], k, 0); //0
        convert_int_assign_scoplib2osl(rl->precision, rl->m[0], k,
                                           (scoplib_int_t)0);
      else if( k==(0+1) )   // Arr
        //osl_int_set_si(rl->precision, rl->m[j], k, -1);
        convert_int_assign_scoplib2osl(rl->precision, rl->m[0], k,
                                           (scoplib_int_t)-1);
      else if(k==(nb_cols-1))  //Arr_id
        //osl_int_set_si(rl->precision, rl->m[j], k, array_id[i]);
        convert_int_assign_scoplib2osl(rl->precision, rl->m[0], k, 
                                            (scoplib_int_t)array_id[i]);
      else // non diagonal zeros
        //osl_int_set_si(rl->precision, rl->m[j], k, 0);
        convert_int_assign_scoplib2osl(rl->precision, rl->m[0], k,
                                           (scoplib_int_t)0);
    }
     
    if(array_has_dims[i]){ //one/multi-dimensional case
      //for each row
      for(j=0; j< array_dims[i]; j++,m_row++){
          //for each column
        for(k=0; k< nb_cols; k++){
          if(k==0)  // eq/neq (first) column
            //osl_int_set_si(rl->precision, rl->m[j+1], k, 0); //0
            convert_int_assign_scoplib2osl(rl->precision, rl->m[j+1], k,
                                               (scoplib_int_t)0);
          else if(k>0 && k<=(nb_output_dims)){ // for output dimentions
            if( k==(j+2) )   //identification diagonal
              //osl_int_set_si(rl->precision, rl->m[j+1], k, -1);
              convert_int_assign_scoplib2osl(rl->precision, rl->m[j+1], k,
                                                (scoplib_int_t)-1);
            else  // non diagonal zeros
              //osl_int_set_si(rl->precision, rl->m[j+1], k, 0);
              convert_int_assign_scoplib2osl(rl->precision, rl->m[j+1], k,
                                                (scoplib_int_t)0);
          }
          else  // input dimensions + parameters + constant
            //osl_int_set_si(rl->precision, rl->m[j+1], k, m->p[m_row][k-nb_output_dims]);
            convert_int_assign_scoplib2osl(rl->precision, rl->m[j+1], k,
                                              m->p[m_row][k-nb_output_dims]);
        }

      } // end for each row

    }  // end if(array_has_dims)
    else{ // zero-dimensional array handling
        m_row++;
    }// if(array_has_dims)



    //printf("relation_found %d:\n", i);
    //osl_relation_print(stdout, rl);
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

  //TODO: free the above allocated arrays
  return out_rln_list;
}

osl_generic_p convert_body_scoplib2osl(int nb_iterators, char **iter,
                                      char* body){
  int i=0;
  if(iter==NULL && body==NULL)
    return NULL;

  osl_generic_p out_gen = osl_generic_malloc();
  osl_body_p out_body = osl_body_malloc();
  out_gen->interface = osl_body_interface();
  
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
    //osl_relation_print(stdout,tmp_stmt->domain);

    //scattering
    tmp_stmt->scattering = convert_scattering_scoplib2osl(s->schedule, in_ctx);
    //osl_relation_print(stdout,tmp_stmt->scattering);

    //access  //TODO: make generic append(read) append(write) ?
    //scoplib_matrix_print_structure(stdout, s->read, 0);
    tmp_stmt->access = convert_access_scoplib2osl(s->read, in_ctx, in_stmt,
                                                  OSL_TYPE_READ);
    //printf("read list\n");
    //osl_relation_list_print(stdout, tmp_stmt->access);
    osl_relation_list_p tmp_rl = tmp_stmt->access;
    for(; tmp_rl && tmp_rl->next; ) 
      tmp_rl = tmp_rl->next;
    if(!tmp_rl){
      //scoplib_matrix_print_structure(stdout, s->write, 0);
      tmp_stmt->access = convert_access_scoplib2osl(s->write, in_ctx, in_stmt, 
                                                          OSL_TYPE_WRITE);
      //printf("write list, after readlist=NULL\n");
      //osl_relation_list_print(stdout, tmp_stmt->access);
    }
    else{
      //scoplib_matrix_print_structure(stdout, s->write, 0);
      tmp_rl->next = convert_access_scoplib2osl(s->write, in_ctx, in_stmt, 
                                                         OSL_TYPE_WRITE);
      //printf("write list, after readlist\n");
      //osl_relation_list_print(stdout, tmp_rl->next);
    }

    //body
    //printf("Scoplib body:\n%s\n", s->body);
    //printf("Num iterators = %d\n", s->nb_iterators);
    //int i=0;
    //for(i=0; i< s->nb_iterators; i++)
     // printf("%d %s\n", i, s->iterators[i]);

    tmp_stmt->body = convert_body_scoplib2osl(s->nb_iterators, s->iterators,
                                              s->body);
    //if(tmp_stmt->body){
    //  printf("body\n");
    //  osl_body_print(stdout, tmp_stmt->body->data);
    //}

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
* Translates  scoplib_scop to osl_scop
* Return: NULL if unsuccessful
*  : pointer to osl_scop if successful
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
    //printf("old_context:\n");
    //scoplib_matrix_print(stdout, inscop->context);
    tmp_scop->context = convert_matrix_scoplib2osl(inscop->context);
    //printf("new_context:\n");
    //osl_relation_print(stdout, tmp_scop->context);
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
    //printf("nb_parameters:%d\n", inscop->nb_parameters);
    for(i=0; i< inscop->nb_parameters; i++){
      OSL_strdup(params_cpy[i], inscop->parameters[i]);
      //printf("parameters[%d]: %s\n", i, params_cpy[i]);
    }
    osl_strings_p params = osl_strings_malloc();
    params->string = params_cpy;
    
    tmp_scop->parameters = osl_generic_malloc();
    tmp_scop->parameters->data = params;
    tmp_scop->parameters->interface = osl_strings_interface();
    //printf("parameters:\n");
    //osl_generic_print(stdout, tmp_scop->parameters);
  }
  else
    CONVERTER_warning("No parameters detected\n");
  
  //statement
  if(inscop->statement){
    tmp_scop->statement = convert_statement_scoplib2osl(inscop->statement,
                                                        inscop->context);
    //osl_statement_print(stdout, tmp_scop->statement);
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
    osl_arrays_p arrays = osl_arrays_sread( &content );
    tmp_scop->extension = osl_generic_malloc();
    tmp_scop->extension->data = arrays;
    tmp_scop->extension->interface = osl_arrays_interface();
    //printf("arrays_content: %s\n", content);

  }

  //usr
  
  output_scop = tmp_scop;
  return output_scop;
}
