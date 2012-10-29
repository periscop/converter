#include "converter/converter.h"
#include "converter/converter_dep.h"
//#include "converter_int.h"
#include "osl/scop.h"
#include "osl/body.h"
#include "osl/generic.h"
#include "osl/util.h"
#include "osl/extensions/arrays.h"
#include <string.h>  // warnings for strlen() incomatible implicit declaration
#include "osl/macros.h"

#include "scoplib/scop.h"
#include "scoplib/matrix.h"


#include <stdio.h> 
#include <stdlib.h> 

/*
* this functions converts an osl relation into a scoplib matrix
*
* \param[in] in_rln     osl relation to convert
* \return               equivalent coverted scoplib matrix
*/
scoplib_matrix_p convert_relation_osl2scoplib(osl_relation_p in_rln){

  int i = 0;
  int j = 0;

  if(in_rln==NULL){
    return NULL;
  }

  if(in_rln->nb_local_dims)
    CONVERTER_error("Cannot handle Local Dimensions in a relation. Abort.\n");

  scoplib_matrix_p ctx = scoplib_matrix_malloc(in_rln->nb_rows,
                                                in_rln->nb_columns);

  ctx->NbRows = in_rln->nb_rows;
  ctx->NbColumns = in_rln->nb_columns; 

  //copy matrix
  for(i=0; i < in_rln->nb_rows; i++){
    for(j=0; j < in_rln->nb_columns; j++){
       convert_int_assign_osl2scoplib(&(ctx->p[i][j]),
                                     in_rln->precision, in_rln->m[i],j);
    }
  }

  return ctx;
}

/*
* this functions converts an list of osl relations into a scoplib_matrix_list
*
* \param[in] in_rln     osl relation to convert
* \return               equivalent coverted scoplib_matrix_list
*/
scoplib_matrix_list_p convert_relation_list_osl2scoplib(osl_relation_p in_rln){

    scoplib_matrix_list_p m_list = NULL;
    scoplib_matrix_list_p ml_head= NULL;

    if(in_rln == NULL)
      return NULL;

      osl_relation_p rln = in_rln;
      for(;rln; rln=rln->next){ 
      
        scoplib_matrix_list_p ml_tmp=scoplib_matrix_list_malloc();
        ml_tmp->elt = convert_relation_osl2scoplib(rln);

        if(!m_list){
          m_list = ml_head = ml_tmp;
        }
        else{
          ml_head->next = ml_tmp;
          ml_head = ml_head->next;
        }
      }

      return m_list;
}



/*
* this functions converts scattering osl relation into a scoplib schedule 
*
* \param[in] scattering osl access relation to convert
* \return               equivalent coverted scoplib schedule
*/
scoplib_matrix_p convert_scattering_osl2scoplib(osl_relation_p scattering){

    int i=0;
    int j=0;

    if(scattering==NULL){
      return NULL;
    }

    if(scattering->nb_local_dims)
      CONVERTER_error("Cannot handle Scattering Local Dimensions. Abort.\n");

    int nb_cols = scattering->nb_columns - 
        scattering->nb_output_dims;
    int nb_rows = scattering->nb_rows;

    scoplib_matrix_p scat = scoplib_matrix_malloc(nb_rows, nb_cols);
    scat->NbRows = nb_rows;
    scat->NbColumns = nb_cols;

    // TODO: FIND the order of the scat_dims

    //copy the eq/neq column
    for(i=0; i < scattering->nb_rows; i++)
      convert_int_assign_osl2scoplib(&scat->p[i][0],
                      scattering->precision, scattering->m[i], 0); 

    //copy input_dims and the rest
    for(i=0; i < scattering->nb_rows; i++)
      for(j=scattering->nb_output_dims+1; j < scattering->nb_columns; j++)
        convert_int_assign_osl2scoplib(
                            &scat->p[i][j-(scattering->nb_output_dims)],
                            scattering->precision, scattering->m[i], j); 

    return scat;
}

/*
* given a list of osl access relations, this fucntions calculates the 
* number of rows and columns for the scoplib read/write matrices based
* on the dimensions of read/write array accesses in the statement
*
* \param[in] access               osl access relation list
* \param[out] nb_rows_read        number of rows for the read access matrix
* \param[out] nb_columns_read     number of columns for the read access matrix
* \param[out] nb_rows_write       number of rows for the write access matrix
* \param[out] nb_columns_write    number of columns for the write access matrix
* \param[out] nb_rows_maywrite    number of rows for the maywrite access matrix
* \param[out] nb_columns_maywrite number of columns for the maywrite acc matrix
* \return                         void
*/
void convert_access_calc_dimensions( osl_relation_list_p access,
                                     int *nb_rows_read,
                                     int *nb_columns_read,
                                     int *nb_rows_write,
                                     int *nb_columns_write,
                                     int *nb_rows_may_write,
                                     int *nb_columns_may_write){
  if(access==NULL)
    CONVERTER_warning("NULL access relation pointer passed\n");


  osl_relation_list_p head = access;

  while (head) {
    if (head->elt != NULL) {
      if (head->elt->type == OSL_TYPE_READ) {
        if (head->elt->nb_rows == 1)            
          (*nb_rows_read)++;
        else
          (*nb_rows_read) += head->elt->nb_rows - 1; // remove the 'Arr'
          
        (*nb_columns_read) = head->elt->nb_columns - 
                                      head->elt->nb_output_dims;
        
      } else if (head->elt->type == OSL_TYPE_WRITE) {
        if (head->elt->nb_rows == 1)
          (*nb_rows_write)++;
        else
          (*nb_rows_write) += head->elt->nb_rows - 1; // remove the 'Arr'


          (*nb_columns_write) = head->elt->nb_columns - 
                                      head->elt->nb_output_dims;
      } else if (head->elt->type == OSL_TYPE_MAY_WRITE) {
        if (head->elt->nb_rows == 1)
          (*nb_rows_may_write)++;
        else
          (*nb_rows_may_write) += head->elt->nb_rows - 1; // remove the 'Arr'

          (*nb_columns_may_write) = head->elt->nb_columns -
                                      head->elt->nb_output_dims;
      }
    }

    if(head->elt->next != NULL)
      CONVERTER_error("Union of Access relation detected. Abort.\n");

    head = head->next;
  }

  return;
}


/*
* this functions converts an osl access relation into a scoplib access matrix
*
* \param[in] head       osl access relation to convert
* \param[in] type       type of acccess relation: read/write
* \param[in] type2      mayrite accesses to be combined with write type
* \param[in] nb_rows    number of rows for the [out] scoplib_matrix
* \param[in] nb_columns    number of columns for the [out] scoplib_matrix
* \return               new scoplib access matrix
*/
scoplib_matrix_p convert_access_osl2scoplib(osl_relation_list_p head,
                                            int type, int type2,
                                            int nb_rows,
                                            int nb_columns){

  if(head==NULL)
    return NULL;

  scoplib_matrix_p m_read = scoplib_matrix_malloc(nb_rows,
              nb_columns);
  // copy
  //head = p->access;
  int i = 0;
  int j = 0;
  while(head){
    if(head->elt->nb_local_dims)
      CONVERTER_error("Cannot handle Access Local Dimensions. Abort.\n");

    if (  head->elt != NULL 
          && (head->elt->type == type || head->elt->type == type2)){
      
      //type2 only may_write
      if(type==OSL_TYPE_READ && head->elt->type == type2) 
        CONVERTER_error("Unknown Access type!! Abort.");

      // get array id
      // assuming first row, last element: TODO:search for it ??
      scoplib_int_t array_id;
      SCOPVAL_init_set_si(array_id, 0);

      convert_int_assign_osl2scoplib(&array_id, 
                    head->elt->precision, head->elt->m[0],
                        head->elt->nb_columns-1);

      int first_access = 1;
      // arrange dimensions in order ????
      int k=0;
      // assign array id here, as some matrices have only one row!
      SCOPVAL_assign(m_read->p[i][0], array_id);
      if(head->elt->nb_rows==1) i++;  //single row matrix

      //skip the frist row; array_id already been recovered
      for(k=1; k< head->elt->nb_rows; k++,i++){

        if(first_access){
          first_access = 0;// do nothing. array_id assigned above
        }
        else{
          //m_read->p[i][0] = 0;
          SCOPVAL_set_si(m_read->p[i][0], 0);
        }

        for(j=head->elt->nb_output_dims+1; j< head->elt->nb_columns; j++){
      //copy matrix, but skip output_dims
          convert_int_assign_osl2scoplib(
                                &m_read->p[i][j-head->elt->nb_output_dims],
                                head->elt->precision, head->elt->m[k], j);
        }
      }
    }
    head = head->next;    
  }

  return m_read;
}

/**
 * convert_osl_strings_sprint function:
 * this function prints the content of an osl_strings_t structure
 * (*strings) into a string (returned) in the OpenScop textual format.
 *
 * \param[in] strings The strings structure which has to be printed.
 * \return A string containing the OpenScop dump of the strings structure.
 */
// commented the '\n' at the end of the string
char * convert_osl_strings_sprint(osl_strings_p strings) {
  int i;
  int high_water_mark = OSL_MAX_STRING;
  char * string = NULL;
  char buffer[OSL_MAX_STRING];

  OSL_malloc(string, char *, high_water_mark * sizeof(char));
  string[0] = '\0';
   
  if (strings != NULL) {
    for (i = 0; i < osl_strings_size(strings); i++) {
      sprintf(buffer, "%s", strings->string[i]);
      osl_util_safe_strcat(&string, buffer, &high_water_mark);
      if (i < osl_strings_size(strings) - 1)
        osl_util_safe_strcat(&string, " ", &high_water_mark);
    }
    //sprintf(buffer, "\n");
    //osl_util_safe_strcat(&string, buffer, &high_water_mark);
  }
  else {
    sprintf(buffer, "# NULL strings\n");
    osl_util_safe_strcat(&string, buffer, &high_water_mark);
  }

  return string;
}


/*
* this functions converts an osl statemetn into a scoplib statement
*
* \param[in] p          osl statemetn to convert
* \param[in] precision  input statmenet precsion  TODO: get rid of it
* \return               equivalent coverted scoplib statement
*/
scoplib_statement_p convert_statement_osl2scoplib(osl_statement_p p,
                                                  int precision){

  scoplib_statement_p stmt_head = NULL;
  scoplib_statement_p last_stmt = NULL;

  int nb_stmt = 1;

  if(p==NULL)
    return NULL;

  for( ; p; p = p->next, nb_stmt++){

    scoplib_statement_p stmt = scoplib_statement_malloc();

    //domain
    //domain list
    stmt->domain = convert_relation_list_osl2scoplib(p->domain);

    //scattering
    stmt->schedule = convert_scattering_osl2scoplib(p->scattering);

    // calculate the read/write dimensions
    int nb_rows_write = 0;
    int nb_columns_write = 0;
    int nb_rows_may_write = 0;
    int nb_columns_may_write = 0;
    int nb_rows_read = 0;
    int nb_columns_read = 0;

    //access list
    convert_access_calc_dimensions(p->access, 
                                  &nb_rows_read, &nb_columns_read,
                                  &nb_rows_write, &nb_columns_write,
                                  &nb_rows_may_write, &nb_columns_may_write);

    // allocate read
    //TODO: could be done more neatly with a relation_filter!
    stmt->read = convert_access_osl2scoplib( p->access, OSL_TYPE_READ,
                                             0, //dummy
                                             nb_rows_read, nb_columns_read);

    // allocate write
    //if(nb_rows_write || nb_rows_may_write){
    stmt->write = convert_access_osl2scoplib( p->access, OSL_TYPE_WRITE,
                       OSL_TYPE_MAY_WRITE, nb_rows_write+nb_rows_may_write,
                       nb_columns_write+nb_columns_may_write);

    // iterators
    osl_body_p  stmt_body=NULL;
    if(p->body)
    stmt_body = (osl_body_p)(p->body->data);

    if(stmt_body==NULL)
      CONVERTER_warning("Statement body not found!!\n");
    else{
      stmt->nb_iterators = osl_strings_size(stmt_body->iterators);
      osl_strings_p iters = osl_strings_clone(stmt_body->iterators);
      stmt->iterators = iters->string;
      //body
      stmt->body = convert_osl_strings_sprint(stmt_body->expression);
    }

      //usr  not used??? TODO:
    //stmt->usr = NULL;



    if(!stmt_head){ 
      stmt_head = last_stmt = stmt;
    }
    else{
      last_stmt->next = stmt;
      last_stmt = last_stmt->next;
    }


  }

  return stmt_head;
}


/**
 * osl_scop_names function:
 * this function generates as set of names for all the dimensions
 * involved in a given scop.
 *
 * \param[in] scop The scop (list) we have to generate names for.
 * \return A set of generated names for the input scop dimensions.
 */
static
osl_names_p convert_osl_scop_names(osl_scop_p scop) {
  int nb_parameters = OSL_UNDEFINED;
  int nb_iterators  = OSL_UNDEFINED;
  int nb_scattdims  = OSL_UNDEFINED;
  int nb_localdims  = OSL_UNDEFINED;
  int array_id      = OSL_UNDEFINED;

  osl_scop_get_attributes(scop, &nb_parameters, &nb_iterators,
                          &nb_scattdims,  &nb_localdims, &array_id);
  
  return osl_names_generate("P", nb_parameters,
                            "i", nb_iterators,
                            "c", nb_scattdims,
                            "l", nb_localdims,
                            "A", array_id);
}

/**
 * osl_arrays_sprint function:
 * this function prints the content of an osl_arrays_t structure
 * (*arrays) into a string (returned) in the OpenScop textual format.
 *
 * \param[in] arrays The arrays structure to print.
 * \return A string containing the OpenScop dump of the arrays structure.
 */
char * convert_osl_arrays_sprint(osl_strings_p arrays) {
  int i;
  int high_water_mark = OSL_MAX_STRING;
  char * string = NULL;
  char buffer[OSL_MAX_STRING];

  int nb_names = osl_strings_size(arrays);

  if (arrays != NULL) {
    OSL_malloc(string, char *, high_water_mark * sizeof(char));
    string[0] = '\0';

    sprintf(buffer, "# Number of arrays\n");
    osl_util_safe_strcat(&string, buffer, &high_water_mark);

    sprintf(buffer, "%d\n", nb_names);
    osl_util_safe_strcat(&string, buffer, &high_water_mark);

    if (nb_names) {
      sprintf(buffer, "# Mapping array-identifiers/array-names\n");
      osl_util_safe_strcat(&string, buffer, &high_water_mark);
    }
    for (i = 0; i < nb_names; i++) {
      sprintf(buffer, "%d %s\n", i+1, arrays->string[i]);
      osl_util_safe_strcat(&string, buffer, &high_water_mark);
    }

    OSL_realloc(string, char *, (strlen(string) + 1) * sizeof(char));
  }

  return string;
}


/*
* this functions converts an osl strings structure into a scoplib strings
*
* \param[in] str        osl strings structure
* \return               scoplib strings
*/
char ** convert_strings_osl2scoplib(osl_strings_p str){

  if(str==NULL)
      return NULL;

  int i=0;
  char **out_str = NULL;
  int nb_strings=0;

  if ((nb_strings = osl_strings_size(str)) == 0){ 
    CONVERTER_malloc(out_str, char**, 1*sizeof(char*) );
    *out_str=NULL;
    return out_str;
  }

  CONVERTER_malloc(out_str, char **, (nb_strings + 1) * sizeof(char *));
  out_str[nb_strings] = NULL;
  for (i = 0; i < nb_strings; i++)
    CONVERTER_strdup(out_str[i], str->string[i]);


  return out_str;
}


void convert_scoplib_strings_print(char** s1){

  int i= 0;
  int nb_strings = 0;
  char **s1_cpy = s1;

  while( *s1_cpy++ )
      nb_strings++;

  printf("scoplib strings size=%d\n", nb_strings);
  printf("scoplib strings with indexes:\n");
  for(i=0; i< nb_strings; i++){
  //strcmp returns non-zero if strings not equal
    printf("%d %s\n", i, s1[i]);
  }
}



/*
* this functions checks of two arrays of strings are equal
*
* \param[in] s1         array of strings
* \param[in] s2         array of strings
* \return               1 if equal, 0 therwise
*/
int convert_scoplib_strings_equal(char** s1, char** s2){

  if(s1==s2)
    return 1;

  int i= 0;
  int nb_strings_1 = 0;
  int nb_strings_2 = 0;
  char **s1_cpy = s1;
  char **s2_cpy = s2;

  while( *s1_cpy++) nb_strings_1++;
  while( *s2_cpy++) nb_strings_2++;

  if( nb_strings_1 != nb_strings_2){
    CONVERTER_info("scoplib sizes of strings not equal\n");
    printf("s1_size=%d, s2_size=%d\n", nb_strings_1, nb_strings_1);
    return 0;
  }

  for(i=0; i< nb_strings_1; i++){
  //strcmp returns non-zero if strings not equal
    if(strcmp(s1[i], s2[i]) ){
      CONVERTER_info("scoplib two strings not equal\n");
      printf("s1: %s, s2: %s\n", s1[i], s2[i]);
      return 0;
    }
  }

  return 1;
}


/**
 * scoplib_matrix_equal function:
 * this function returns true if the two matrices are the same, false
 * otherwise.
 *
 * \param m1  The first matrix.
 * \param m2  The second matrix.
 *
 */
int
convert_scoplib_matrix_equal(scoplib_matrix_p m1, scoplib_matrix_p m2)
{
  if (m1 == m2)
    return 1;
  if (m1 == NULL || m2 == NULL)
    return 0;
  if (m1->NbRows != m2->NbRows || m1->NbColumns != m2->NbColumns)
    return 0;
  int i, j;
  for (i = 0; i < m1->NbRows; ++i)
    for (j = 0; j < m1->NbColumns; ++j)
      if (SCOPVAL_ne(m1->p[i][j], m2->p[i][j]))
	      return 0;
  return 1;
}


/*
 * scoplib_matrix_list_equal function:
 * this function returns true if the two matric lists are the same, false
 * otherwise.
*
* \param[in] ml1        scoplib matrix list
* \param[in] ml2        scoplib matrix list
* \return               1 if equal, 0 otherwise
*/
int convert_scoplib_matrix_list_equal( scoplib_matrix_list_p ml1, 
                                       scoplib_matrix_list_p ml2){

  if(ml1==ml2)
    return 1;

  if (((ml1->next != NULL) && (ml2->next == NULL)) ||
     ((ml1->next == NULL) && (ml2->next != NULL))) { 
      CONVERTER_info("scoplib sizes of matrid_lists are not the same\n");
      return 0;
  } 

  if ((ml1->next != NULL) && (ml2->next != NULL)) {
    if (!convert_scoplib_matrix_list_equal(ml1->next, ml2->next)) {
      CONVERTER_info("scoplib matrid_lists are not the same\n");
      return 0;
    }
  }

  if( !convert_scoplib_matrix_equal(ml1->elt, ml2->elt) ){
    CONVERTER_info("scoplib matrixes not equal\n");
    return 0;
  }
  
  return 1;
}

/*
 * scoplib_statement_equal function:
 * this function returns true if the two scoplib statements are the same, false
 * otherwise.
*
* \param[in] s1        scoplib statment
* \param[in] s2        scoplib statment
* \return               1 if equal, 0 otherwise
*/
int convert_scoplib_statement_equal(scoplib_statement_p s1,
                                       scoplib_statement_p s2){

  if(s1==s2)
    return 1;

  if (((s1->next != NULL) && (s2->next == NULL)) ||
     ((s1->next == NULL) && (s2->next != NULL))) { 
      CONVERTER_info("scoplib number of statements are not the same\n");
      return 0;
  } 

  if ((s1->next != NULL) && (s2->next != NULL)) {
    if (!convert_scoplib_statement_equal(s1->next, s2->next)) {
      CONVERTER_info("scoplib statements is not the same\n");
      return 0;
    }
  }

  if( !convert_scoplib_matrix_list_equal(s1->domain, s2->domain) ){
    CONVERTER_info("scoplib statements domains not equal\n");
    return 0;
  }
  //else
  //  printf("scoplib statement domains are the same\n");

  if( !convert_scoplib_matrix_equal(s1->schedule, s2->schedule) ){
    CONVERTER_info("scoplib statements scatterings not equal\n");
    return 0;
  }
  //else
  //  printf("scoplib statement scatterings are the same\n");

  if( !convert_scoplib_matrix_equal(s1->read, s2->read) ){
    CONVERTER_info("scoplib statements read matrixes not equal\n");
    return 0;
  }
  //else
  //  printf("scoplib statement read_matrixes are the same\n");

  if( !convert_scoplib_matrix_equal(s1->write, s2->write) ){
    CONVERTER_info("scoplib statements write matrixes not equal\n");
    return 0;
  }
  //else
  //  printf("scoplib statement write_matrixes are the same\n");

  if( s1->nb_iterators != s2->nb_iterators ){
    CONVERTER_info("scoplib statements nb_iterators not equal\n");
    return 0;
  }
  //else
  //  printf("scoplib statement nb_tertaros are the same\n");

  if( !convert_scoplib_strings_equal(s1->iterators, s2->iterators) ){
    CONVERTER_info("scoplib statements iterators not equal\n");
    convert_scoplib_strings_print(s1->iterators);
    convert_scoplib_strings_print(s2->iterators);
    return 0;
  }
  //else
  //  printf("scoplib statement iterators  are the same\n");

  //strcmp returns non-zero if strings not equal
  if( strcmp(s1->body, s2->body) ){
    CONVERTER_info("scoplib statements bodies not equal\n");
    return 0;
  }
  //else
  //  printf("scoplib statement bodies are the same\n");

  return 1;
}


/*
 * scoplib_scop_equal function:
 * this function returns true if the scoplib scops are the same, false
 * otherwise.
*
* \param[in] s1        scoplib scop
* \param[in] s2        scoplib scop
* \return               1 if equal, 0 otherwise
*/
int convert_scoplib_scop_equal( scoplib_scop_p s1, scoplib_scop_p s2){

  if(s1!=NULL && s2!=NULL && s1==s2) // same scop
    return 1;

  if (((s1 == NULL) && (s2 != NULL)) || ((s1 != NULL) && (s2 == NULL))){
    CONVERTER_info("unequal scops! one is NULL!\n");
    return 0;
  }

  if( !scoplib_matrix_equal(s1->context, s2->context) ){
    CONVERTER_info("contexts not same! \n");
    return 0;
  }
  //else
  //  printf("scoplib scop contexts  are the same\n");

  if(s1->nb_parameters != s2->nb_parameters ){
    CONVERTER_info("nb_paramters not equal! \n");
    return 0;
  }
  //else
  //  printf("scoplib scop nb_parameters are the same\n");

  if(!convert_scoplib_strings_equal(s1->parameters, s2->parameters)){
    CONVERTER_info("paramters not equal! \n");
    return 0;
  }
  //else
  //  printf("scoplib scop parameters are the same\n");

  if(s1->nb_arrays != s2->nb_arrays ){
    CONVERTER_info("nb_arrasy not equal! \n");
    return 0;
  }
  //else
  //  printf("scoplib scop nb_arrays are the same\n");

  if(!convert_scoplib_strings_equal(s1->arrays, s2->arrays)){
    CONVERTER_info("arrays not equal! \n");
    return 0;
  }
  //else
  //  printf("scoplib scop arrays are the same\n");

  if(!convert_scoplib_statement_equal(s1->statement, s2->statement)){
    CONVERTER_info("statements not equal! \n");
    return 0;
  }
  //else
  //  printf("scoplib scop statements are the same\n");

  //strcmp returns non-zero if strings not equal
  if(strcmp(s1->optiontags, s2->optiontags)){
    CONVERTER_info("optiontags not equal! \n");
    return 0;
  }
  //else
  //  printf("scoplib scop optiontags are the same\n");

  // usr defined hj

  return 1;
}



/***************************************************************************
* Translates osl_scop to scoplib_scop
* Return: NULL if unsuccessful
*  : pointer to scoplib_scop if successful
*
* \param[in] inscop     osl scop to be converted
* \return               equivalent scoplib scop
****************************************************************************/
scoplib_scop_p  convert_scop_osl2scoplib( osl_scop_p inscop){

//  int i=0;
//  int j=0;
  scoplib_scop_p out_scop = NULL; //return arg
//  scoplib_scop_p last_scop = NULL; 
  scoplib_scop_p tmp_scop = NULL;

  //NULL input pointer
  if (inscop == NULL)
    return out_scop;

  if (osl_scop_integrity_check(inscop) == 0)
    CONVERTER_warning("OSL integrity check failed. Something may go wrong.");
  //TODO: return here?


  tmp_scop = scoplib_scop_malloc();

  int precision = osl_util_get_precision();
  //version
  //language   //not used??
  //context
  tmp_scop->context = convert_relation_osl2scoplib(inscop->context);
 

  //parameters
  tmp_scop->nb_parameters = inscop->context->nb_columns - 2;
  if(tmp_scop->nb_parameters){
    osl_strings_p params = osl_strings_clone(inscop->parameters->data); 
    tmp_scop->parameters = convert_strings_osl2scoplib(params);
    osl_strings_free(params);
  }



  tmp_scop->statement = convert_statement_osl2scoplib(inscop->statement,
                                                      precision); 
  //registry
  //externsion
  osl_names_p names;
  names = convert_osl_scop_names(inscop);
  //externsion -> arrays
  osl_arrays_p arrays = osl_generic_lookup(inscop->extension, 
        OSL_URI_ARRAYS);
  if(arrays){
    tmp_scop->nb_arrays = arrays->nb_names; //??
    osl_strings_p arr = osl_arrays_to_strings(arrays);
    tmp_scop->arrays = convert_strings_osl2scoplib(arr);
    osl_strings_free(arr);
  }
  else{  //assign generated names
    tmp_scop->nb_arrays = osl_strings_size(names->arrays);
    tmp_scop->arrays = convert_strings_osl2scoplib(names->arrays);
  }


  //optoin tags
  // arrays
  char *string  = NULL;
  if(arrays){
    string = osl_arrays_sprint((osl_arrays_p) arrays);
  }
  else{
    string = convert_osl_arrays_sprint(names->arrays);
  }

  char *new_str = NULL;
  if (string != NULL) {
    OSL_malloc(new_str, char *, (strlen(string) + 20) * sizeof(char));
    sprintf(new_str, "<arrays>\n%s</arrays>\n", string);
  }
  tmp_scop->optiontags = new_str;
    

  // dependenceies
  convert_dep_osl2scoplib(inscop, tmp_scop);



  //usr  not used???
  tmp_scop->usr = NULL;



  if(inscop->next)
    CONVERTER_warning("Multiple SCoPs detected. Converting first only!!!\n");


  out_scop = tmp_scop;
  

  return out_scop;
}
