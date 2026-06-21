#include <stdio.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/un.h> // unix sockets

#define DEF_SOCK_FILENAME   "/tmp/daemon.sock"
#define BUFF_SIZE           1024

char sockfile[ 200 ];
char progress [ 4 ] = { '-', '\\', '|', '/' };
char p = 0;

/*
  Подключение к демону
  Результат:
    0 - OK
   -1 - ошибка
 */
int get_data_from_daemon( void )
{
  int odf;
  int ret_val;
  struct sockaddr_un un_addr;
  char answ[ 512 ];


  odf = socket( AF_UNIX, SOCK_STREAM, 0 );
  if( odf == -1 ){
    perror( "Не создан сокет" );
    return -1;
  }

  memset( &un_addr, 0, sizeof( un_addr ) );
  un_addr.sun_family = AF_UNIX;
  memcpy( &un_addr.sun_path, sockfile, strlen( sockfile ) );
  ret_val = connect( odf, ( struct sockaddr * )&un_addr, sizeof( un_addr ) );
  if( ret_val == -1 ) {
    fprintf( stderr, "Не удалось подключиться к демону через файл '%s' или демон не запущен\n", sockfile );
    perror( "connect" );
    close( odf );
    return -1;
  }

  memset( answ, 0, sizeof( answ ) );
  ret_val = read( odf, answ, sizeof( answ ) );
  if( ret_val == -1 )
  {
    perror( "read" );
  }
  else
  {
    printf( "[%c] ответ от демона: '%s'        \r", progress[(p=( ++p & 0x3 ))], answ );
    fflush( stdout );
  }

  close( odf );
  return ret_val;
}

/*
  Тестирование сервера
 */
int main( int argc, char *argv[] )
{
  char buff[ BUFF_SIZE ];
  int i, ret;

  if( argc < 2 ) {
    fprintf( stderr, "По-умолчанию для подключения используется файл '%s'\n"
                     "Вы можете уточнить имя файла через команду запуска:\n"
                     "  %s <ваш UNIX-сокет для подключения>\n"
                     , DEF_SOCK_FILENAME, argv[ 0 ] );
  }
  memset( sockfile, 0, sizeof( sockfile ) );

  if( argc > 1 ) { // если задан аргумент в командной строке, используем его
    strncpy( sockfile, argv[ 1 ], strlen( argv[ 1 ] ) );
  } else {         // иначе используем по-умолчанию
    strncpy( sockfile, DEF_SOCK_FILENAME, strlen( DEF_SOCK_FILENAME ) );
  }

  printf( "Для остановки тестирования нажмите CTRL+C\n" );
  while( 1 )
  {
    ret = get_data_from_daemon();
    if( ret == -1 ) {
      fprintf( stderr, "Не выполнено. Для остановки нажмите CTRL+C\n" );
      usleep( 500000 );
    } else {
      usleep( 10 );
    }
  }

  return ret;
}
