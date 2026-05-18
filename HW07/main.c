#define _POSIX_C_SOURCE 200809L /* можно же? для backtrace_... :-) */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>

#include <lerr.h>
#include "danger_code.h"
#include "log_spammer.h"

static volatile sig_atomic_t need_stop = 0;

/*
 * Обработчик сигналов (исключений)
 */
void my_handle_signal( int sig )
{
  char *msg = NULL;

  switch( sig )
  {
    case SIGFPE:
      msg = "Исключение SIGFPE";
      break;
    case SIGSEGV:
      msg = "Исключение SIGSEGV";
      break;
    case SIGABRT:
      msg = "Исключение SIGABRT";
      break;
    default:
      msg = "Исключение";
      break;
  }

  /* Пишем в журнал с ключом LERR_FATAL, чтобы выдать стек вызовов перед крахом программы */

  lerr_mess( LERR_FATAL, "%s (signal #%d)", msg, sig );

  switch( sig )
  {
    /* При получении данных сигналов работу программы сложно  */
    case SIGFPE:
    case SIGSEGV:
    case SIGABRT:
      _exit( EXIT_FAILURE );
      break;
    default:
      break;
  }

  need_stop = 1;

}


int main()
{
  char key = 0;
  int  dbg = 0;

  dbg = lerr_init( "./_logs.txt" );
  if( dbg < 0 )
  {
    fprintf( stderr, "Ошибка инициализации библиотеки lerr\n" );
    exit( EXIT_FAILURE );
  }

  struct sigaction sa;
  sa.sa_handler = my_handle_signal;
  sigemptyset( &sa.sa_mask );
  sa.sa_flags = 0;

  if( sigaction( SIGSEGV, &sa, NULL ) < 0 )
    return -1;
  if( sigaction( SIGFPE , &sa, NULL ) < 0 )
    return -1;
  if( sigaction( SIGABRT, &sa, NULL ) < 0 )
    return -1;
  if( sigaction( SIGINT , &sa, NULL ) < 0 )
    return -1;

  lerr_stderr_on();

  lerr_mess( LERR_INFO, "Программа запущена" );

  do
  {
    int hide_key = 0;

    puts(
  "\n\nЧто будем делать?:\n"
  " 1<Enter> - обращение через NULL-указатель (фатально)\n"
  " 2<Enter> - двойное освобождение памяти (фатально)\n"
  " 3<Enter> - деление на 0 (фатально)\n"
  " 4<Enter> - потоковые гонки для журналирования\n"
  " 5<Enter> - трассировка вызовов из рекурсии\n"
  " q<Enter> - выход из программы\n"
        );

    key = getchar();
    while ((hide_key = getchar()) != '\n' && hide_key != EOF);

    switch(key)
    {
      case '1':
        key = 0;
        null_ptr_example();
        break;
      case '2':
        key = 0;
        double_free();
        break;
      case '3':
        key = 0;
        div_by_zero( 1 );
        break;
      case '4':
        key = 0;
        thread_logging( );
        break;
      case '5':
        key = 0;
        deep_call_of_traceback( 7 );
        break;
      case 'q':
      case 'Q':
        key = 'q';
        break;
      default:
        key = 0;
        fprintf( stderr, "Не выбрано известное действие\n" );
        break;
    }

    if( need_stop == (sig_atomic_t)1 ) break;

  } while( key != 'q' );

  lerr_mess( LERR_INFO, "Программа штатно остановлена" );

  lerr_exit();

  exit( EXIT_SUCCESS );
}
