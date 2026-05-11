#include <lerr.h>
#include <time.h>
#include <malloc.h>

void null_ptr_example( void )
{
  int A;
  int *ptr_to_A = &A;
  *ptr_to_A = 25;
  printf( "original A=%d\n", A);

  ptr_to_A = NULL;
  *ptr_to_A = 25;

  return;
}

void double_free( void )
{
  int *mem = (int*)malloc( 10 * sizeof(int) );
  free( mem );
  free( mem );

  return;
}

void div_by_zero( int c )
{
  c--;
  int b = 2 / c;

  return;
}

void deep_call_of_traceback( int a )
{
  if( a > 0 )
    deep_call_of_traceback( a - 1 );
  else
    lerr_mess( LERR_ERROR, "Message from deep calling function" );

  return;
}
