#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <lerr.h>
#include "danger_code.h"
#include "log_spammer.h"

int main() {
  char key = 0;

  lerr_init ( "./_logs.txt" );

  lerr_stderr_on();

  lerr_mess( LERR_INFO, "%s", "Программа запущена" );

  do
  {
    if( key != 0x0a )
    {
      printf(
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
        null_ptr_example();
        break;
      case '2':
        double_free();
        break;
      case '3':
        div_by_zero( 1 );
        break;
      case '4':
        thread_logging( );
        break;
      case '5':
        deep_call_of_traceback( 7 );
        break;
      case 'q':
      case 'Q':
        key = 0;
        break;
      default:
        fprintf( stderr, "Не выбрано известное действие\n" );
        break;
    }

  } while( key != 0 );

  lerr_mess( LERR_INFO, "%s", "Программа штатно остановлена" );
  lerr_exit();

  return 0;
}
