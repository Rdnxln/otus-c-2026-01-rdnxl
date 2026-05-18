#include <lerr.h>
#include <threads.h>
#include <stdio.h>
#include "danger_code.h"

int thread_func( void* arg )
{
  char* name = (char*)arg;

  for( int i = 0; i < 10; i++ )
  {
    if( *name == 'A' )
      lerr_mess( LERR_INFO, "AAAAAAAAA" );
    else
      lerr_mess( LERR_INFO, "BBBBBBBBB" );

    thrd_sleep( &(struct timespec){.tv_sec=0, .tv_nsec= ( *name=='A' ) ? i*100000 : 900000-i*100000 }, NULL );
  }

  return thrd_success;
}

/*
 * Проверка доступа к функциям журналирования из нескольких потоков
 */
void thread_logging( void )
{
  thrd_t  thread1, thread2;

  lerr_mess( LERR_DEBUG, ">> IN"  );

  if( thrd_create( &thread1, thread_func, "A" ) != thrd_success )
  {
    lerr_mess( LERR_ERROR, "Can't create thread A!" );
    return;
  }

  if( thrd_create( &thread2, thread_func, "B" ) != thrd_success ) {
    lerr_mess( LERR_ERROR, "Can't create thread B!" );
    return;
  }

  thrd_join(thread1, NULL);
  thrd_join(thread2, NULL);

  lerr_mess( LERR_DEBUG, "<< OUT" );

  return;
}
