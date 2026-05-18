#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <lerr.h>
#include "danger_code.h"
#include "log_spammer.h"

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

  lerr_stderr_on();

  lerr_mess( LERR_INFO, "Программа запущена" );

  do
  {
    if( key != 0x0a )
    {
      puts(
  "\n\nЧто будем делать?:\n"
  " 1<Enter> - обращение через NULL-указатель (фатально)\n"
  " 2<Enter> - двойное освобождение памяти (фатально)\n"
  " 3<Enter> - деление на 0 (фатально)\n"
  " 4<Enter> - потоковые гонки для журналирования\n"
  " 5<Enter> - трассировка вызовов из рекурсии\n"
  " q<Enter> - выход из программы\n"
          );
    }

    key = getchar();
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

    if( lerr_is_need_stop() == (sig_atomic_t)1 ) break;
    /* Т.к. все обработчики назначаются не в main(),
       а во "внутренностях" библиотеки, то они и флажок у себя там выставляют
       в исключительных ситуациях, про который пользователь библиотеки может и не знать...
       Поэтому, подготовили для него задокументированный вызов,
       на который надо обращать внимание :)
       Признаю, что попытка занести в библиотеку обработчики
       исключительных ситуаций ошибочна, т.к. все случаи предусмотреть сложно ... */

  } while( key != 'q' );

  lerr_mess( LERR_INFO, "Программа штатно остановлена" );

  lerr_exit();

  exit( EXIT_SUCCESS );
}
