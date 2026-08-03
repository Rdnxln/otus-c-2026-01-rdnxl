#define _DEFAULT_SOURCE
/* Необходим madvise(), но он не входит в стандарт C11
   Надо, чтобы не "ругался" компилятор */

#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/stat.h>

#include "crc32_alg.h"

void usage( char *name )
{
  fprintf( stderr, "Использование: %s <имя_файла>\n", name );
  fputs( "Рассчет контрольной суммы CRC-32 по файлу\n", stderr );
}

int main( int argc, char **argv )
{
  int       fd = -1;
  uint32_t  crc = 0;

  if( argc < 2 ) {
    fputs( "Укажите входной файл для обработки\n", stderr );
    usage( argv[0] );
    goto err_exit;
  }

  if( !strcmp(argv[1], "--help") ||
      !strcmp(argv[1], "-h") ) {
    usage( argv[0] );
    return (EXIT_SUCCESS);
  }

  /* Пробуем открыть файл и выяснить его размер */
  fd = open( argv[1], O_RDONLY );
  if( fd == -1 ) {
    fprintf( stderr, "Не удалось открыть входной файл %s\n", argv[1] );
    goto err_exit;
  }

  struct stat info;
  if( fstat(fd, &info) == -1)
  {
    perror("fstat");
    goto err_exit;
  }

/* см. man fstat :
  switch (sb.st_mode & S_IFMT) {
    case S_IFBLK:  printf("block device\n");            break;
    case S_IFCHR:  printf("character device\n");        break;
    case S_IFDIR:  printf("directory\n");               break;
    case S_IFIFO:  printf("FIFO/pipe\n");               break;
    case S_IFLNK:  printf("symlink\n");                 break;
    case S_IFREG:  printf("regular file\n");            break;
    case S_IFSOCK: printf("socket\n");                  break;
    default:       printf("unknown?\n");                break;
  }
 */

  if( (info.st_mode & S_IFMT) == S_IFDIR )
  {
    fprintf( stderr, "is directory\t%s\n", argv[1] );
    goto err_exit;
  }

  size_t file_size = info.st_size;

  /* Пробуем выяснить оптимальный размер куска для отображения части файла */
  /* узнаем размер страницы памяти */
  size_t page_size = sysconf( _SC_PAGESIZE );
  /* хотим смотреть по 64 мегабайта */
  size_t part_size = 1024U * 1024U * 64U;
  /* выравниваемся по размеру страницы */
  part_size = (part_size / page_size ) * page_size;

  size_t offset = 0; /* сдвиг по файлу от его начала */

  /* цикл итераций */
  while( offset < file_size ) {
    size_t curr_size;

    if( file_size - offset < part_size )
      curr_size = file_size - offset;
    else
      curr_size = part_size;
/*
    printf( "Обработка данных с %ld по %ld байт\n", offset, curr_size );
 */
    void *data = mmap( NULL, curr_size, PROT_READ, MAP_PRIVATE, fd, offset );
    if( data == MAP_FAILED ) {
      perror( "map" );
      break;
    }

    madvise( data, curr_size, MADV_SEQUENTIAL );

    crc = crc32_alg( crc, data, curr_size );
    munmap(data, curr_size);

    offset += curr_size;
  }
  close(fd); fd = -1;

  printf("%08x\t%s\n", crc, argv[1]);

  return (EXIT_SUCCESS);
err_exit:
  if( fd != -1 )
    close( fd );
  return (EXIT_FAILURE);
}
