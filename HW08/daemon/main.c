/* Демон, отдающий размер файла, указанного в конфигурационном файле /etc/daemon.conf

  Формат файла - ini-подобный (без [секций]):
    file_stat=имя_файла              (имя файла, для которого мы возвращаем размер)
    unix_sock=/unix/socket/file.sock (файл для подключения)

 */
#define _POSIX_C_SOURCE 200809L /* для psignal() */

#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>

#include <fcntl.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>     /* unix sockets */
#include <signal.h>

#include <sys/stat.h>
#include <sys/file.h>   /* flock */

#include <syslog.h>
#include <sys/resource.h>
#include <sys/types.h>


#define CONF_FILE          "/etc/daemon.conf"
#define KEYVAL_LEN         512

#define FILE_STAT_KEY      "file_stat"
#define UNIX_SOCK_KEY      "unix_sock"

/* по-умолчанию наблюдаем за размером системного журнала */
#define DEFAULT_FILE_STAT  "/var/log/syslog"

/* по-умолчанию создаем файл /tmp/daemon.sock для подключения к нашему демону */
#define DEFAULT_UNIX_SOCK  "/tmp/daemon.sock"

/* */
#define PIDFILE_DIR        "/run/daemon/"
#define PIDFILE            "daemon.pid"

#define BUFF_SIZE          1024

int   idf    = -1;
int   pid_f  = -1;

volatile sig_atomic_t
      flWork =  0;

int   daemon_mode = 0;

char  file_stat[ KEYVAL_LEN *2 ];
char  unix_sock[ KEYVAL_LEN *2 ];


char **_argv = NULL;

void stop_proc( int sig_num )
{
  psignal(sig_num, "Получен сигнал" );
  flWork = 0;
}

void usage( void )
{
  fprintf( stderr,
"Формат запуска:\n"
" %s      Обычный режим.\n"
" %s -d   Режим демона.\n", _argv[0], _argv[0] );

  /* не стал делать одним вызовом, форматированный вывод
     с длинющими строками-шаблонами - не очень хорошая идея */
  fprintf( stderr,
"В режиме демона все сообщения выводятся в syslog.\n"
"Необходимо создать файл конфигурации " CONF_FILE "\n"
"с параметрами:\n"
"  " FILE_STAT_KEY "=/path/file\n"
"  " UNIX_SOCK_KEY "=/path/unix-sock\n"
"где\n"
"  " FILE_STAT_KEY " - наблюдаемый файл (по-умолчанию " DEFAULT_FILE_STAT ");\n"
"  " UNIX_SOCK_KEY " - UNIX-сокет для подключения (по-умолчанию " DEFAULT_UNIX_SOCK ").\n"
         );
}

/* удаляем начальные и завершающие пробелы */
void str_trim_spaces( char *str )
{  int i = 0, /* итератор */
       s = 0; /* начало строки (за пробелами) */

  if( str == NULL ) return;

  while( str[ i ] == ' ' ) /* пропускаем стартовые пробелы (если есть) */
    i++;

  s = i; /* сохраняем индекс начала строки без учета стартовых пробелов */
  do {
    str[ i - s ] = str[ i ]; /* сдвигаем всю строку к началу, замещая стартовые пробелы */
  }  while( str[ i++ ] != '\0' );

  i = i - s - 2;
  while( str[ i ] == ' ' || str[ i ] == '\n' ) {
    str[ i ] = '\0';
    i--;
  }
}

void read_conf( void )
{
  FILE *cnf = NULL;
  char  str[ KEYVAL_LEN * 2 + 1 + 1 ], /* key=val\0                   */
                                       /* 512 512   <-- KEYVAL_LEN*2  */
                                       /*    1   1                    */
        key[ KEYVAL_LEN * 2 + 1 ],
        val[ KEYVAL_LEN * 2 + 1 ];

  int   fc;

  if( ( cnf = fopen( CONF_FILE, "rt" ) ) == NULL ) {
    if( daemon_mode ) {
      syslog( LOG_WARNING, "Нет доступа к конфигурации: " CONF_FILE "\n" );
    } else {
      fprintf( stderr, "Нет доступа к конфигурации: " CONF_FILE "\n" );
      usage();
    }
  } else {

    while( ! feof( cnf ) ) {

      memset( str, 0, sizeof( str ) );

      if( NULL != fgets( str, sizeof( str ) -1, cnf ) ) {

        memset( key, 0, sizeof( key ) );
        memset( val, 0, sizeof( val ) );

        fc = sscanf( str, "%[^=]=%[^=]", &key[0], &val[0] );
        if( fc == 2 ) {

          str_trim_spaces( key );
          str_trim_spaces( val );

          if( strcmp( key, FILE_STAT_KEY ) == 0 ) {
            strncpy( file_stat, val, sizeof( file_stat ) -1 );
            continue;
          }
          if( strcmp( key, UNIX_SOCK_KEY ) == 0 ) {
            strncpy( unix_sock, val, sizeof( unix_sock ) -1 );
            continue;
          }

        }
      }
    }
    fclose( cnf );
    cnf = NULL;

  }

  /* если один из параметров не считан, мы не можем продолжать */
  if( file_stat[0] == '\0' ) {
    snprintf( file_stat, KEYVAL_LEN, "%s", DEFAULT_FILE_STAT );
    if( daemon_mode ) {
      fprintf( stderr,  "Внимание, не задан конфигурационный параметр " FILE_STAT_KEY ".\n"
                        "Используется по-умолчанию значение: " FILE_STAT_KEY "=" DEFAULT_FILE_STAT ".\n" );
    } else {
      syslog( LOG_WARNING, "Внимание, не задан конфигурационный параметр " FILE_STAT_KEY ".\n"
                           "Используется по-умолчанию значение: " FILE_STAT_KEY "=" DEFAULT_FILE_STAT ".\n" );
    }
  }

  if( unix_sock[0] == '\0' ) {
    snprintf( unix_sock, KEYVAL_LEN, "%s", DEFAULT_UNIX_SOCK );
    if( daemon_mode ) {
      fprintf( stderr,  "Внимание, не задан конфигурационный параметр " UNIX_SOCK_KEY ".\n"
                        "Используется по-умолчанию значение: " UNIX_SOCK_KEY "=" DEFAULT_UNIX_SOCK ".\n" );
    } else {
      syslog( LOG_WARNING, "Внимание, не задан конфигурационный параметр " UNIX_SOCK_KEY ".\n"
                           "Используется по-умолчанию значение: " UNIX_SOCK_KEY "=" DEFAULT_UNIX_SOCK ".\n" );
    }
  }

  return;
}

/* функция демонизации */
void daemonize( void )
{
  int               i, fd0, fd1, fd2;
  pid_t             pid;
  struct rlimit     rl;
  struct sigaction  sa;

  /* Сброс маски режима создания файла */
  umask( 0 );

  /* Получение максимально возможного номера дескриптора файла */
  if( getrlimit(RLIMIT_NOFILE, &rl ) < 0 )
    perror( "невозможно получить максимальный размер дескриптора" );

  /* Станем лидером нового сеанса, чтобы утратить управлящий терминал вскоре */
  if( ( pid = fork() ) < 0 )
    perror( "ошибка вызова функции fork" );
  else {
    if( pid != 0 ) { /* родительский процесс */
      exit( EXIT_SUCCESS );
    }
  }

  setsid();

  /* Обеспечить невозможность обретения управляющего терминала в будущем */
  sa.sa_handler = SIG_IGN;
  sigemptyset( &sa.sa_mask );
  sa.sa_flags = 0;
  if( sigaction( SIGHUP, &sa, NULL ) < 0 )
    syslog( LOG_CRIT, "Невозможно игнорировать сигнал SIGHUP "
                      "(pid дочернего процесса будет другим)" );

  if( ( pid = fork()) < 0 )
    syslog( LOG_CRIT, "Ошибка вызова функции fork" );
  else {
    if( pid != 0 ) { /* родительский процесс */
      exit( EXIT_SUCCESS );
    }
  }

  /* Назначение корневого каталога текущим рабочим каталогом,
     чтобы в последствии можно было отмонтировать файловую систему */
  if( chdir( "/" ) < 0 )
    syslog( LOG_CRIT, "Невозможно сделать корневой каталог системы '/' текущим рабочим каталогом" );

  /* Закрыть все открытые файловые дескрипторы */
  if( rl.rlim_max == RLIM_INFINITY )
    rl.rlim_max = 1024;
  for( i = 0 ; i < (int)rl.rlim_max ; i++ )
    close( i );

  /* Присоединить стандартные файловые
     дескрипторы 0, 1 и 2 к /dev/null */
  fd0 = open( "/dev/null", O_RDWR );
  fd1 = dup( 0 );
  fd2 = dup( 0 );
  if( fd0 != 0 || fd1 != 1 || fd2 != 2 )
    syslog( LOG_CRIT, "ошибочные файловые дескрипторы %d %d %d",
                      fd0, fd1, fd2 );

}

int main( int argc, char **argv )
{
  int                 ret_val = EXIT_SUCCESS;
  struct sockaddr_un  sa_unix;            /* описание адреса UNIX domain socket */
  char                buff[ BUFF_SIZE ];  /* буфер приема данных */
  int fc;

  _argv = argv;

  struct sigaction sa;
  sa.sa_handler = stop_proc;
  sigemptyset( &sa.sa_mask );
  sa.sa_flags = 0;

  if( ( sigaction( SIGTERM, &sa, NULL ) < 0 ) ||
      ( sigaction( SIGABRT, &sa, NULL ) < 0 ) ||
      ( sigaction( SIGINT,  &sa, NULL ) < 0 ) ||
      ( sigaction( SIGQUIT, &sa, NULL ) < 0 )
    )
  {
    ret_val = EXIT_FAILURE;
    goto app_exit;
  }


  for( fc = 1; fc < argc ; fc++ ) {

    if( strcmp( argv[ fc ], "-h" ) == 0 ||
        strcmp( argv[ fc ], "--help" ) == 0 ) {
      usage();
      goto app_exit;
    }

    if( strcmp( argv[ fc ], "-d" ) == 0 ||
        strcmp( argv[ fc ], "--daemon" ) == 0 ) {
      daemon_mode = 1;
      if( geteuid() != 0 ) {
        fprintf( stderr, "Попытка запуска в режиме демона не супер-пользователем.\n"
                         "Убедитесь, что у Вас есть право записи в "
                          PIDFILE_DIR PIDFILE ".\n"
                         "В данном режиме сообщения выводятся в файлы /var/log/.. syslog|user.log\n" );
      }
    }
  }

  if( daemon_mode ) {
    /* Инициализация файла журнала */
    openlog( argv[ 0 ], LOG_CONS, LOG_DAEMON );

    daemonize( );

    syslog( LOG_INFO, "Процесс запущен" );

    /* создаем каталог для pid-файла */
    if( mkdir( PIDFILE_DIR, 0755 ) == -1 &&
        errno != EEXIST ) {
      /* perror("невозможно создать каталог " PIDFILE_DIR " через mkdir");
         комментируем perror, т.к. у нас теперь stderr смотрит в /dev/null */
      syslog( LOG_WARNING, "Невозможно создать каталог " PIDFILE_DIR "! Возможно, у Вас нет прав root" );
      ret_val = EXIT_FAILURE;
      goto app_exit;
    }

    /* если каталог создался или уже был ранее создан,
       создаем файл ... */
    pid_f = open( PIDFILE_DIR PIDFILE, O_RDWR | O_CREAT, 0644 );
    if( pid_f == -1 ) {
      /*perror( "не удалось создать pid-файл " PIDFILE_DIR PIDFILE ); */
      syslog( LOG_WARNING, "Не удалось создать pid-файл " PIDFILE_DIR PIDFILE );
      ret_val = EXIT_FAILURE;
      goto app_exit;
    }

    /* если файл создался, делаем "неблокирующую" блокировку */
    /* иначе дублирующий запуск получит доступ к файлу, 
       "поправив", удалив его и т.д. */
    if( flock( pid_f, LOCK_EX | LOCK_NB ) == -1 ) {
      if( errno == EWOULDBLOCK ) {
        syslog( LOG_CRIT, "Демон уже запущен, pid-файл: " PIDFILE_DIR PIDFILE " уже заблокирован" );
      } else {
        /*perror( "Ошибка блокировки pid-файла" );*/
        syslog( LOG_WARNING, "Ошибка блокировки pid-файла" );
      }
      ret_val = EXIT_FAILURE;
      close( pid_f ); /* закрываем здесь, а удалять нельзя,
                         т.к. не нами занят */
      goto app_exit;
    }

    if( ftruncate( pid_f, 0 ) == -1 )
    {
      /*perror( "Не удалось очистить pid-файл" );*/
      syslog( LOG_CRIT, "Не удалось очистить pid-файл" );
      ret_val = EXIT_FAILURE;
      goto app_exit_cleanup;
    }

   /* ref to `man pid_t`
  DESCRIPTION
         pid_t  is a type used for storing process IDs, process group IDs,
         and session IDs.  It is a signed integer type.
                                   ~~~~~~~~~~~~~~~~~~~
    */
    if( dprintf( pid_f, "%d\n", getpid() ) < 0 ) {
      perror( "Ошибка записи в " PIDFILE_DIR PIDFILE );
      syslog( LOG_WARNING, "Ошибка записи в " PIDFILE_DIR PIDFILE );
      ret_val = EXIT_FAILURE;
      goto app_exit_cleanup;
    }
    fsync(pid_f);
  }

  memset( file_stat, 0, sizeof( file_stat ) );
  memset( unix_sock, 0, sizeof( unix_sock ) );

  if( !daemon_mode )
    fprintf( stdout, "Запуск %s\n"
                     "Демон мониторинга файла\n"
                    , argv[ 0 ] );

  read_conf();

  idf = socket( AF_UNIX, SOCK_STREAM, 0 );
  if( idf == -1 ) {
    perror( "Создание unix-сокета" );
    ret_val = EXIT_FAILURE;
    goto app_exit;
  }

  memset( &sa_unix, 0, sizeof( sa_unix ) );
  sa_unix.sun_family=AF_UNIX;
  memcpy(  &sa_unix.sun_path , unix_sock, strlen( unix_sock ) );
  if( ( bind( idf, ( struct sockaddr * ) &sa_unix, sizeof( sa_unix ) ) ) == -1 ) {
    perror( "Связывание" );
    ret_val = EXIT_FAILURE;
    goto app_exit_cleanup;
  }

  if( ( listen( idf, 1 ) ) == -1 )    {
    perror( "Слушание" );
    ret_val = EXIT_FAILURE;
    goto app_exit_cleanup;
  }

  flWork = 1;
  while( flWork == 1 ) { /* рабочий цикл */

    int        iifd;
    socklen_t  size = sizeof( sa_unix );

    /* printf("Прием входящего подключения (accept)\n"); */
    iifd = accept( idf, ( struct sockaddr * ) &sa_unix, &size );
    /* printf("Результат accept=%i\n", iifd ); */

    if( iifd > 0 ) {
      struct stat  statbuff;

      memset( buff, 0, sizeof( buff ) );
      if( -1 == stat( file_stat, &statbuff ) )
      {
        sprintf( buff, "-1" );
      }
      else
      {
        /* printf( "размер файла: %li (%i)\n", (int)statbuff.st_size , sizeof(off_t) ); */
        sprintf( buff, "%li", statbuff.st_size );
      }

      write( iifd, buff, strlen(buff) );
      close( iifd );

    }
    /* printf( "\n===== ---\n" ); */
  }

app_exit_cleanup:

  if( idf != -1 )
    close( idf );
  idf = -1;

  unlink( unix_sock );

  if( pid_f != -1 )
    close( pid_f );

  unlink( PIDFILE_DIR PIDFILE );


app_exit:

  if( daemon_mode ) {
    syslog( LOG_INFO, "Процесс %sзавершен\n",
                      ret_val == EXIT_FAILURE ? "аварийно " : "" );

    closelog();
  } else {
    fprintf(stderr, "Процесс %s %sзавершен\n",
                     argv[0],
                     ret_val == EXIT_FAILURE ? "аварийно " : "" );
  }
  exit( ret_val );
}
