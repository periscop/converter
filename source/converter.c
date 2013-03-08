
#include <stdio.h>
#include <stdlib.h>

#include "converter/converter.h"


void display_usage(){
printf("Usage\n");
printf("./converter 0 scoplib_scop_filename\n");
printf("./converter 1 osl_scop_filename\n");
}

int main( int argc, char* argv[]){

  if(argc<3 || argc>3 || 
     (strtol(argv[0],NULL,10)!=0 && strtol(argv[0],NULL,10)!=1) ){
    display_usage();
    return 1;
  }

  char o_filename[100];
  //read osl scop from file
  FILE * infile = fopen( argv[2], "r");
  if(!infile){
    fprintf(stderr, "Error: Unable to open infile:%s\n", argv[2]);
    return 1;
  }


  scoplib_scop_p scop0 = NULL;
  osl_scop_p scop1 = NULL;

  if(strtol(argv[1],NULL, 10)==0){
      printf("reading scoplib_scop\n");
      scop0 = scoplib_scop_read( infile );
      //scoplib_scop_print_structure(stdout, scop0, 0);
      //scoplib_scop_print_dot_scop(stdout, scop0);
      printf("read scoplib_scop\n");
      scop1 = convert_scop_scoplib2osl(scop0);
      printf("converted scoplib_scop to osl_scop\n");
	  // print osl scop to file
	  sprintf(o_filename, "%s.osl", argv[2]);
	  FILE *outfile0 = fopen(o_filename, "w+");
	  osl_scop_print(outfile0, scop1);
	  fclose(outfile0);
	  printf("OSL scop printed to file\n");
  }
  else{
    printf("reading osl_scop\n");
    scop1 = osl_scop_read ( infile );
    printf("read osl_scop\n");
  }

  fclose(infile);
  
  //printf("oritinal scop:\n");
  //osl_relation_print(stdout, scop1->context);

  //convert osl to scoplib
  scoplib_scop_p scop2 = convert_scop_osl2scoplib(scop1);
  if(scop2==NULL)
  	printf("OSL to Scoplib conversion failed\n");
  //scoplib_scop_print_structure(stdout, scop2, 0);
  //print scoplib scop to file
  sprintf(o_filename, "%s.scoplib", argv[2]);
  FILE *outfile = fopen(o_filename, "w+");
  scoplib_scop_print_dot_scop(outfile, scop2);
  //printf("scoptagoptions:\n%s\n", scop2->optiontags);
  fclose(outfile);
  printf("Scoplib scop printed to file\n");
  //manually generate code and compare to original
  //read scoplib scop from file

  FILE *infile2 = fopen(o_filename, "r");
  scoplib_scop_p scop3 = NULL;
  scop3 = scoplib_scop_read( infile2);
  fclose(infile2);

  //scoplib_scop_print_structure(stdout, scop3, 0);
  //printf("Scoplib scop read from file\n");




  // convert scoplib to osl
  osl_scop_p scop4 = convert_scop_scoplib2osl( scop2 );

  //exit(0);
  printf("Scoplib scop converted to OSL scop\n");


  // print osl scop to file
  sprintf(o_filename, "%s.scoplib.osl", argv[2]);
  FILE *outfile2 = fopen(o_filename, "w+");
  osl_scop_print(outfile2, scop4);
  fclose(outfile2);
  printf("OSL scop printed to file\n");
  //manually generate code and compare to original




  
  if(strtol(argv[1],NULL, 10)==1){

    printf("scop1 language = %s\n", scop1->language);
    CONVERTER_strdup(scop4->language, scop1->language);

    if( convert_osl_scop_equal(scop1, scop4) )
    	printf("OSL -> Scoplib -> OSL works  :D\n");
    else
    	printf("OSL -> Scoplib -> OSL failed :(\n");

  }
  else{

    if( convert_scoplib_scop_equal(scop0, scop2) )
    	printf("Scoplib -> OSL -> Scoplib works  :D\n");
    else
    	printf("Scoplib -> OSL -> Scoplib failed :(\n");

  }



  if(scop0) scoplib_scop_free( scop0 );
  if(scop1){ 
    osl_statement_p stmt = scop1->statement;
    while(stmt){ 
      candl_statement_usr_cleanup(stmt); 
      stmt = stmt->next;
    }
    osl_scop_free( scop1 );
  }
  if(scop4){ 
    osl_statement_p stmt = scop4->statement;
    while(stmt){ 
      candl_statement_usr_cleanup(stmt); 
      stmt = stmt->next;
    }
    osl_scop_free( scop4 );
  }
  if(scop2) scoplib_scop_free (scop2 );
  if(scop3) scoplib_scop_free (scop3 );

  return 0;
}
