#define _POSIX_C_SOURCE 200809L /* можно же? для backtrace_... :-) */
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <threads.h>
#include <time.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <stdarg.h>
#include <execinfo.h>

#include "lerr.h"

static volatile sig_atomic_t lerr_need_stop = 0;


/*
 * Глобальные мьютексы для синхронизации потоков
 */
/* доступ к дескрипторам вывода */

static mtx_t      log_mtx;
static once_flag  log_init_flag  = ONCE_FLAG_INIT;

/* доступ к потокоНЕбезопасной функции localtime(),
   чтобы не использовать localtime_r из POSIX,
   иначе мы получим warnings! */
/*
static mtx_t      time_mtx;
static once_flag  time_init_flag = ONCE_FLAG_INIT;
 */

/*
 * File descriptor for error logs output
 * Дескриптор файла для вывода ошибок
 */
int fd_log         = -1;

/*
 * Дублировать вывод на стандартный вывод об ошибках
 * 0  - Не дублировать (по-умолчанию)
 * !0 - Дублировать
 */
int to_stderr_copy =  0;

/*
void lerr_init_time_mtx( void ) {
  if( mtx_init( &time_mtx, mtx_plain ) != thrd_success )
  {
    write(2, "Ошибка инициализации mutex\n", sizeof("Ошибка инициализации mutex\n") -1 );
  }
}
 */

void lerr_log_time_mtx( void ) {
  if( mtx_init( &log_mtx, mtx_plain ) != thrd_success )
  {
    write(2, "Ошибка инициализации mutex\n", sizeof("Ошибка инициализации mutex\n") -1 );
  }
}


void lerr_print_stack_trace( int fd, int dup2stderr ) {
/*char  output[ 512 ]; */
  void *array [ MAX_STACK_TRACE_DEPTH  ];

  int size = backtrace( array, MAX_STACK_TRACE_DEPTH );
/*
  int sz = 0;
  sz = snprintf( output, sizeof(output), "\n  Извлечено %d вложенных вызовов:\n", size );

  if( fd != -1 )
    write( fd, output, sz );
  if( dup2stderr != 0 )
    write( STDERR_FILENO , output, sz );
 */
  if( fd != -1 )
    write( fd, "\nТрассировка вызовов:\n", sizeof( "Трассировка вызовов:\n" ) -1 );
  if( dup2stderr != 0 )
    write( STDERR_FILENO, "\nТрассировка вызовов:\n", sizeof( "Трассировка вызовов:\n" ) -1 );


  if( fd != -1 )
    backtrace_symbols_fd( array, size, fd );
  if( dup2stderr != 0 )
    backtrace_symbols_fd( array, size, STDERR_FILENO );
}


/*
 * Обработчик сигналов (исключений)
 */
void lerr_handle_signal( int sig )
{
  char *msg = NULL;

  switch( sig )
  {
    case SIGFPE:
      msg = "Исключение по арифметической операции";
      break;
    case SIGSEGV:
      msg = "Исключение по доступу к памяти";
      break;
    case SIGABRT:
      msg = "Исключение SIGABRT";
      break;
    default:
      msg = "Неизвестное исключение";
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

  lerr_need_stop = 1;

}


/*
 * Инициализация библиотеки
 */
int   lerr_init        (const char *out_log_file)
{
  /* Гарантируем, что мьютекс инициализирован перед использованием */
/*
  call_once(&time_init_flag, lerr_init_time_mtx);
 */
  call_once(&log_init_flag , lerr_log_time_mtx );

  struct sigaction sa;
  sa.sa_handler = lerr_handle_signal;
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

  if( fd_log != -1 )
    lerr_exit();

  if( out_log_file == NULL )
  {
    const char msg[] = "Не указан файл для журнала ошибок\n";
    write( STDERR_FILENO, msg, sizeof(msg) -1 );
    return -1;
  }

  fd_log = open( out_log_file,
                 O_CREAT   /* Создаем, если файл отсутствует */
               | O_WRONLY  /* Только запись, без чтения */
               | O_APPEND  /* Дописываем */
               , 0644 );   /* rw-r--r */

  if( fd_log == -1 )
  {
    const char msg[] = "Нет доступа к файлу журнала: '";
    write( STDERR_FILENO, msg, sizeof(msg) -1 );

    while( *out_log_file != '\0' ) {
      write( STDERR_FILENO, out_log_file, 1 );
      out_log_file++;
    }
    write( STDERR_FILENO, &"'\n", 2 );
    return -1;
  }

  lerr_need_stop = 0;

/*
   const char msg[] = "\nНачало работы";
   write( fd_log, msg, sizeof(msg) -1 );
 */
  lerr_mess( LERR_INFO, "Начато журналирование в файл '%s', если это другой файл - логи подделаны:)\n"
                        "<date>-<time>.<nanosec> [level ] {file:line\tfunc_name()}\tСообщение в журнале... (<- формат записи в журнале)",
                        out_log_file );

  return 0;
}

void lerr_exit       ()
{
  if( fd_log != -1 )
  {
/*  lerr_mess( LERR_INFO, "Завершено журналирование (или переинициализация библиотеки)" ); */
    close( fd_log );
    fd_log = -1;
  }
}

sig_atomic_t
     lerr_is_need_stop()
{
  return lerr_need_stop;
}

void lerr_stderr_on   ()
{
  to_stderr_copy = 1;
}

void lerr_stderr_off  ()
{
  to_stderr_copy = 0;
}

void lerr_mess_intern (lerr_level_t  level,
                       const char   *file,
                       const char   *func,
                       int           line,
                       const char   *fmt, ...)
{
  struct timespec  ts;
  struct tm        local;

  char  time_str    [sizeof("yymmdd-HHMMSS.123456789")];
  char  message_buf [1024];
  char  final_buf   [ 2*sizeof(message_buf) + 1 ];

  if( fd_log == -1 && to_stderr_copy == 0 )
    return;


  memset( final_buf, 0, sizeof( final_buf ) );

  if( timespec_get( &ts, TIME_UTC ) != TIME_UTC )
  {
    ts.tv_sec = time( NULL );
    ts.tv_nsec = 0;
  }

  /* Получаем дату и время */
  /*
  localtime_s( &ts.tv_sec, &local );
  К сожалению, данная фукнция (Standard C11, Annex K), так и не попала в glibc (у меня Debian)
   */

  localtime_r( &ts.tv_sec, &local );

/*
  if( mtx_lock( &time_mtx ) == thrd_success )
  {
    struct tm *tmp = localtime(&ts.tv_sec);
    if (tmp) {
      local = *tmp;
    }
    mtx_unlock(&time_mtx);
  }
 */

  /* Форматируем основное время и добавляем наносекунды */
  strftime( time_str, sizeof(time_str), "%y%m%d-%H%M%S", &local );
  /* Дописываем .наносекунды */
  snprintf( time_str + strlen(time_str), sizeof(time_str), ".%09ld", ts.tv_nsec );

  /* Форматируем сообщение пользователя */
  va_list args;
  va_start( args, fmt );
    vsnprintf( message_buf, sizeof(message_buf), fmt, args );
  va_end( args );

  /* Собираем финальную строку */
  int sz = snprintf( final_buf,
                     sizeof(final_buf),
                     "\n%s [%s] {%s:%i\t%s()}\t%s",
                     time_str,
                       level == LERR_DEBUG  ? "DEBUG " :
                     ( level == LERR_INFO   ? "INFO  " :
                     ( level == LERR_NOTICE ? "NOTICE" :
                     ( level == LERR_WARN   ? "WARN  " :
                     ( level == LERR_ERROR  ? "ERROR " :
                     ( level == LERR_CRIT   ? "CRIT  " :
                     ( level == LERR_ALERT  ? "ALERT " :
                     ( level == LERR_FATAL  ? "FATAL " :
                                              "UNKWN "
                     ))))))),
                     file, line, func,
                     message_buf );

  /* Системный вызов атомарен, но ...
     согласуем все-же доступ к ресурсам (дескрипторам)
     через mutex */

  if( mtx_lock( &log_mtx ) == thrd_success )
  {
    if( to_stderr_copy != 0 )
      write( 2,      final_buf, sz );

    if( fd_log != -1 )
      write( fd_log, final_buf, sz );

    if( level <= LERR_ERROR )
      lerr_print_stack_trace( fd_log, to_stderr_copy );

    mtx_unlock(&log_mtx);
  }

  return;
}
