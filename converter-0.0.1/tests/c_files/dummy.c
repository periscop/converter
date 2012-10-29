/* dummy.c  for testing purposes */
#include <stdio.h>

int main()
{
  int   i,j,k;
  float a[N][N], b[N][N], c[N][N]; 

  /* We read matrix a */
  for(i=0; i<N; i++)
    for(j=0; j<N; j++)
      scanf(" %f",&a[i][j]);

  /* We read matrix b */
  for(i=0; i<N; i++)
    for(j=0; j<N; j++)
      scanf(" %f",&b[i][j]);

  /* c = a * b */
#pragma scop
  for(i=0; i<N; i++)
  ;
#pragma endscop

  /* We print matrix c */
  for(i=0; i<N; i++)
  {
    for(j=0; j<N; j++)
      printf("%6.2f ",c[i][j]);
    printf("\n");
  }
  
  return 0;
}
