#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <string.h>
#include <unistd.h>

/* Что будет, если собрать код на BigEndian-архитектуре?
#define PK_LFH_SIGNATURE  (0x04034b50)
#define PK_CDFH_SIGNATURE (0x02014b50) */

#define FREE_SAFE(a)  do{ free((a)); a=NULL; } while(0)

typedef  unsigned char  byte;

/* Структуры данных, по описанию:
   https://users.cs.jmu.edu/buchhofp/forensics/formats/pkzip-printable.html
   Элементы переменной длины (имя, ...) не включены в структуры
 */

#pragma pack(push, 1)
typedef struct _LFH { /* Local File Header (Без полей переменной длины) */
/*uint32_t  Signature; */
  uint8_t   Sign1;
  uint8_t   Sign2;
  uint8_t   Sign3;
  uint8_t   Sign4;
  uint16_t  Version;
  uint16_t  Flags;
  uint16_t  Compression;
  uint16_t  Modtime;
  uint16_t  Modedate;
  uint32_t  Crc32;
  uint32_t  CompressedSize;
  uint32_t  UncompressedSize;
  uint16_t  FileNameLen;
  uint16_t  ExtraFieldLen;
} LFH;

typedef struct _CDFH { /* Central Directory File Header (без полей переменной длины) */
/*uint32_t  Signature; */
  uint8_t   Sign1;
  uint8_t   Sign2;
  uint8_t   Sign3;
  uint8_t   Sign4;
  uint16_t  Version;
  uint16_t  VersionNeeded;
  uint16_t  Flags;
  uint16_t  Compression;
  uint16_t  Modtime;
  uint16_t  Modedate;
  uint32_t  Crc32;
  uint32_t  CompressedSize;
  uint32_t  UncompressedSize;
  uint16_t  FileNameLen;
  uint16_t  ExtraFieldLen;
  uint16_t  FileCommLen;
  uint16_t  DiskStart;
  uint16_t  InternalAttr;
  uint32_t  ExternalAttr;
  uint32_t  OffsetLocalHeader;
} CDFH;
#pragma pack(pop)

static char path_name[ 0xFFFF +1 ]; /* 0x10000: массив под имя директории/файла */

int check_file( char *filename );

int main( int argc, char **argv )
{
  int i;
  if( argc < 2 )
  {
    fprintf( stderr, "ИСПОЛЬЗОВАНИЕ: %s <файл-1> [ ... <файл-i>]\n", argv[ 0 ] );
    return EXIT_FAILURE;
  }

  for( i = 1; i < argc ; i++ )
  {
    int ret = check_file( argv[i] );

    if( ret == 0 )
    {
      printf( "Файл %s не содержит в себе ZIP-архива\n", argv[i] );
      continue;
    }

    if( ret > 0 )
    {
      printf( "Файл %s содержит в себе ZIP-архив из %d файл(а,ов)\n", argv[i], ret );
      continue;
    }

    if( ret == -1 )
    {
      printf( "Файл %s не обработан\n", argv[i] );
      continue;
    }
  }

  return EXIT_SUCCESS;
}


/* Поиск ZIP-архива и файлов
   Результат:
   -1 - ошибка обработки файла
    0 - файл не содержит ZIP-архив
    N - число файлов в ZIP-архиве */
int check_file( char *filename )
{
  if( filename == NULL ) return -1;
  printf( "Обработка файла '%s'\n", filename );
  FILE *file = fopen( filename, "rb" );
  if( !file ) {
    fprintf( stderr, "Ошибка открытия файла '%s'\n", filename );
    perror( "fopen()" );
    return -1;
  }

  /* Определяем размер файла */
  fseek( file, 0, SEEK_END );
  ssize_t file_size = ftell( file );
  if( file_size == -1 )
  {
    fprintf( stderr, "Ошибка при работе с файлом '%s'\n", filename );
    perror( "ftell()\n" );
    fclose( file );
    return -1;
  }

  rewind( file );

  if( file_size < (ssize_t)sizeof( LFH ) ||
      file_size < (ssize_t)sizeof( CDFH )
    )
  {
    printf( "Файл '%s' не содержит ZIP архив\n", filename );
    fclose( file );
    return -1;
  }

  byte *buffer = (byte *)malloc( file_size );
  if( buffer == NULL ) {
    printf( "Нехватка памяти при обработке файла '%s' (%li байт)\n", filename, file_size );
    fclose( file );
    return -1;
  }

  fread( buffer, 1, file_size, file );
  fclose( file );

  byte  *ptr;    /* начальная граница файла */
  byte  *ptr_up; /* граница файла, до которой выполняется поиск сигнатур (P K 0x3 0x4 или P K 0x1 0x2) */

#if (1)
  /* код извлечения ZIP-архива в отдельный файл
     Извлечение данных через LOCAL FILE HEADERS */
  /* начало файла */
  ptr    = buffer;
  /* почти конец файла (со сдвигом к началу на размер структуры LFH) */
  ptr_up = buffer + ( file_size - (ssize_t)sizeof( LFH ));

  while( ptr < ptr_up )
  {
    LFH *P = (LFH*)ptr; /* двигаем окно нашей структуры вдоль файла и заглядываем в ее части */
/*  if( (uint32_t)P->Signature == (uint32_t)PK_LFH_SIGNATURE ) */
    if( P->Sign1 == (byte)'P' &&
        P->Sign2 == (byte)'K' &&
        P->Sign3 == (byte)0x3 &&
        P->Sign4 == (byte)0x4
      )
    {
      char tmpname[ 1024 ];
      sprintf( tmpname, "%s.zip", filename );
      FILE *zipout = fopen( tmpname, "wb" );
      if(zipout!=NULL)
      {
        fwrite( ptr, 1, file_size - (ssize_t)(ptr - buffer), zipout );
        fflush( zipout );
        fclose( zipout );
      }
      break; /* извлекли архив */
    }
    ptr++;
  }
#endif

  /* Извлечение имен директорий/ и файлов через CENTRAL DIRECTORY FILE HEADERS
     начало файла */
  ptr    = buffer;
  /* почти конец файла (со сдвигом к началу на размер структуры CDFH) */
  ptr_up = buffer + ( file_size - (ssize_t)sizeof( CDFH ));
/* printf( "Files from Central Directory File Headers:\n" ); */
  unsigned int item_count_CDFH = 0;
  while( ptr < ptr_up )
  {
    CDFH *P = (CDFH*)ptr; /* предположим */
/*  if( (uint32_t)P->Signature == (uint32_t)PK_CDFH_SIGNATURE ) */
    if( P->Sign1 == (byte)'P' &&
        P->Sign2 == (byte)'K' &&
        P->Sign3 == (byte)0x1 &&
        P->Sign4 == (byte)0x2
      )
    {
      memcpy(path_name, ptr
                        + sizeof( CDFH ), /* Имя файла начинается сразу за фиксированной частью структуры, и уже имеет переменную длину */
                        P->FileNameLen );
      path_name[ P->FileNameLen ] = '\0';

      printf( "%s\n", path_name );
      ptr += sizeof( CDFH ); /* сдвигаем указатель */
      item_count_CDFH++;
      continue;
    }
    ptr++;
  }

/*
  printf( "Файл '%s'%s содержит ZIP архив\n"
          , filename
          , item_count_CDFH == 0 ?
          " не" : "" );
 */

  FREE_SAFE(buffer);

/*return item_count_LFH; */
  return item_count_CDFH;
}
