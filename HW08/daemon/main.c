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

#define CONF_FILE          "/etc/daemon.conf"
#define KEYVAL_LEN         512

#define FILE_STAT_KEY      "file_stat"
#define UNIX_SOCK_KEY      "unix_sock"

/* по-умолчанию наблюдаем за размером системного журнала */
#define DEFAULT_FILE_STAT  "/var/log/syslog"

/* по-умолчанию создаем файл /tmp/daemon.sock для подключения к нашему демону */
#define DEFAULT_UNIX_SOCK  "/tmp/daemon.sock"

#define BUFF_SIZE          1024

int   idf;
int   flWork = 0;

char  file_stat[ KEYVAL_LEN *2 ];
char  unix_sock[ KEYVAL_LEN *2 ];

void stop_proc( int sig_num )
{
  psignal(sig_num, "Получен сигнал" );
  flWork = 0;
}

void usage( void )
{
  fprintf( stderr,
"Необходимо создать файл конфигурации " CONF_FILE "\n"
"с параметрами:\n"
"  " FILE_STAT_KEY "=/path/file\n"
"  " UNIX_SOCK_KEY "=/path/unix-sock\n"
"где\n"
"  " FILE_STAT_KEY " - наблюдаемый файл (по-умолчанию " DEFAULT_FILE_STAT ")\n"
"  " UNIX_SOCK_KEY " - UNIX-сокет для подключения (по-умолчанию " DEFAULT_UNIX_SOCK ")\n"
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
    fprintf( stderr, "Нет доступа к конфигурации: " CONF_FILE "\n" );
    usage();
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
    sprintf( file_stat, "%s", DEFAULT_FILE_STAT );
    fprintf( stderr, "Внимание, в конфигурационном файле не указан параметр " FILE_STAT_KEY ".\n"
                     "Используется по-умолчанию значение: " FILE_STAT_KEY "=" DEFAULT_FILE_STAT ".\n" );
  }

  if( unix_sock[0] == '\0' ) {
    sprintf( unix_sock, "%s", DEFAULT_UNIX_SOCK );
    fprintf( stderr, "Внимание, в конфигурационном файле не указан параметр " UNIX_SOCK_KEY ".\n"
                     "Используется по-умолчанию значение: " UNIX_SOCK_KEY "=" DEFAULT_UNIX_SOCK ".\n" );
  }

  return;
}

int main( int argc, char **argv )
{
  struct sockaddr_un  sa_unix;            /* описание адреса UNIX domain socket */
  char                buff[ BUFF_SIZE ];  /* буфер приема данных */

  flWork = 0;

  struct sigaction sa;
  sa.sa_handler = stop_proc;
  sigemptyset( &sa.sa_mask );
  sa.sa_flags = 0;

  if( sigaction( SIGTERM, &sa, NULL ) < 0 )
    return -1;
  if( sigaction( SIGABRT, &sa, NULL ) < 0 )
    return -1;
  if( sigaction( SIGINT,  &sa, NULL ) < 0 )
    return -1;
  if( sigaction( SIGQUIT, &sa, NULL ) < 0 )
    return -1;


  int   fc;

  for( fc = 1; fc < argc ; fc++ ) {
    if( strcmp( argv[ fc ], "-h" ) == 0 ||
        strcmp( argv[ fc ], "--help" ) == 0 ) {
      usage();
      return EXIT_SUCCESS;
    }
  }

  memset( file_stat, 0, sizeof( file_stat ) );
  memset( unix_sock, 0, sizeof( unix_sock ) );

  fprintf( stdout, "Запуск %s\n"
                   "Демон мониторинга файла\n"
                  , argv[0] );

  read_conf();

  idf = socket( AF_UNIX, SOCK_STREAM, 0 );
  if( idf == -1 ) {
    perror( "Создание unix-сокета" );
    return EXIT_FAILURE;
  }


  memset( &sa_unix, 0, sizeof( sa_unix ) );
  sa_unix.sun_family=AF_UNIX;
  memcpy(  &sa_unix.sun_path , unix_sock, strlen( unix_sock ) );
  if( ( bind( idf, ( struct sockaddr * ) &sa_unix, sizeof( sa_unix ) ) ) == -1 ) {
    perror( "Связывание" );
    close( idf );
    return EXIT_FAILURE;
  }

  if( ( listen( idf, 1 ) ) == -1 )    {
    perror( "Слушание" );
    close( idf );
    return EXIT_FAILURE;
  }

  flWork = 1;
  while( flWork == 1 ) { /* рабочий цикл */

    int        iifd;
    socklen_t  size = sizeof( sa_unix );

    /* printf("Прием входящего подключения (accept)\n"); */
    iifd = accept( idf, ( struct sockaddr * ) &sa_unix, &size );
    /* printf("Результат accept=%i\n", iifd ); */

    if( iifd > 0 ) {
      struct stat statbuff;

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

  close( idf );
  unlink( unix_sock );

  fprintf(stderr, "Процесс %s остановлен\n", argv[0] );

  return EXIT_SUCCESS;
}
