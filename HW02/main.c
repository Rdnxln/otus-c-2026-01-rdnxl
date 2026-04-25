/***************************************************************************
 Взаимодействие с cURL основано на примере из описания к ДЗ:
   https://curl.se/libcurl/c/getinmemory.html

 * Copyright (C) Daniel Stenberg, <daniel@haxx.se>, et al.
 ***************************************************************************/
/* <DESC>
 * Shows how the write callback function can be used to download data into a
 * chunk of memory instead of storing it in a file.
 * </DESC>

 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <curl/curl.h>
#include <cjson/cJSON.h>

struct MemoryStruct {
  char *memory;
  size_t size;
};

static size_t write_cb(char *contents, size_t size, size_t nmemb, void *userp)
{
  size_t realsize = size * nmemb;
  struct MemoryStruct *mem = (struct MemoryStruct *)userp;

  char *ptr = realloc(mem->memory, mem->size + realsize + 1);
  if(!ptr) {
    /* out of memory! */
    printf("not enough memory (realloc returned NULL)\n");
    return 0;
  }

  mem->memory = ptr;
  memcpy(&(mem->memory[mem->size]), contents, realsize);
  mem->size += realsize;
  mem->memory[mem->size] = '\0';

  return realsize;
}

void print_wind_dir( char *dir )
{
  if( dir == NULL )
    return;

  char *ptr = dir; // указатель на начало строки
  while(*ptr!=0) // пока не встретим нулевой байт окончания строки
  {
    switch(*ptr)
    {
      case 'N':
        printf( "С" );
        break;
      case 'E':
        printf( "В" );
        break;
      case 'S':
        printf( "Ю" );
        break;
      case 'W':
        printf( "З" );
        break;
      default:
        printf( "-" ); // не удалось найти локацию с полным штилем, где направление ветра отсутствует
        break;
    }
    ptr++;
  }
}

int main(int argc, char **argv)
{
  CURL *curl = NULL;
  CURLcode result;

  struct MemoryStruct chunk;
  char url_town[ 1024 ];

  if( argc < 2 )
  {
    fprintf( stderr, "Использование: %s Город\n"
                     "               %s Город%%20c%%20пробелами%%20в%%20имени\n",
                     argv[0], argv[0] );
    return -1;
  }

  if( ( strlen( "https://ru.wttr.in/?format=j2")
      + strlen( argv[1] )
      + 1 ) > sizeof( url_town ) )
  {
    fprintf( stderr, "Задано слишком длинное имя города!\n"
                     "Попробуйте использовать более короткую запись ;-)\n" );
    return -1;
  }
  memset( url_town, 0, sizeof( url_town ) );

  snprintf( url_town, sizeof( url_town )-1, "https://ru.wttr.in/%s?format=j2", argv[1] );

  result = curl_global_init(CURL_GLOBAL_ALL);
  if(result != CURLE_OK)
    return (int)result;

  chunk.memory = (char*)malloc(1); /* grown as needed by the realloc above */
  if( chunk.memory == NULL )
  {
    fprintf( stderr, "Недостаточно памяти для продолжения работы\n" );
    return -1;
  }
  chunk.memory[0] = '\0';
  chunk.size = 0;           /* no data at this point */

  /* init the curl session */
  curl = curl_easy_init();
  if(curl) {

    /* specify URL to get */
    curl_easy_setopt(curl, CURLOPT_URL, url_town );

    /* send all data to this function */
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_cb);

    /* we pass our 'chunk' struct to the callback function */
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk);

    /* some servers do not like requests that are made without a user-agent
       field, so we provide one */
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "libcurl-agent/1.0");

    /* get it! */
    result = curl_easy_perform(curl);

    /* check for errors */
    if(result != CURLE_OK) {
      fprintf(stderr, "curl_easy_perform() failed: %s\n",
              curl_easy_strerror(result));
    }
    else {
      /*
       * Now, our chunk.memory points to a memory block that is chunk.size
       * bytes big and contains the remote file.
       *
       * Do something nice with it!
       */

      // printf("%lu bytes retrieved\n", (unsigned long)chunk.size);

      // Для тестирования принятых данных
      //fwrite( chunk.memory, 1, chunk.size, stdout );
      //fflush( stdout );

      cJSON *root = cJSON_ParseWithLength( chunk.memory, chunk.size );
      if( !root ) {
        fprintf( stderr, "Не удалось разобрать ответ от сервера для местоположения %s\n", argv[1] );
        fprintf( stderr, "Ответ: " );
        fwrite( chunk.memory, 1, chunk.size, stdout );
        fflush( stdout );
      }
      else {
        printf( "Местоположение: %s\n", argv[1] );
        cJSON *current_condition = cJSON_GetObjectItemCaseSensitive(root, "current_condition");
        if( cJSON_IsArray( current_condition ) ) {
          cJSON *element0 = cJSON_GetArrayItem( current_condition, 0 );
          if( element0 ) {
            cJSON *data = NULL;

            // текстовое описание погоды
            data = cJSON_GetObjectItemCaseSensitive( element0, "lang_ru" );
            if( cJSON_IsArray( data ) ) {
              cJSON *el = cJSON_GetArrayItem( data, 0 );
              if( el ) {
                cJSON *value = cJSON_GetObjectItemCaseSensitive( el, "value" );
                if( cJSON_IsString(value) && value->valuestring )
                  printf( "Описание погоды: %s\n", value->valuestring );
              }
            }

            // температура
            data = cJSON_GetObjectItemCaseSensitive( element0, "FeelsLikeC" );
            if( cJSON_IsString( data ) && data->valuestring != NULL )
              printf( "Температура, ощущение: %s °C\n", data->valuestring );

            // влажность
            data = cJSON_GetObjectItemCaseSensitive( element0, "humidity" );
            if( cJSON_IsString( data ) && data->valuestring != NULL )
              printf( "Влажность: %s %%\n", data->valuestring );

            // ветер
            data = cJSON_GetObjectItemCaseSensitive( element0, "winddirDegree" );
            if( cJSON_IsString( data ) && data->valuestring != NULL )
              printf( "Направление ветра: %s ° ", data->valuestring );

            data = cJSON_GetObjectItemCaseSensitive( element0, "winddir16Point" );
            if( cJSON_IsString( data ) && data->valuestring != NULL )
              print_wind_dir( data->valuestring );
            printf( "\n" );

            data = cJSON_GetObjectItemCaseSensitive( element0, "windspeedKmph" );
            if( cJSON_IsString( data ) && data->valuestring != NULL )
              printf( "Скорость ветра: %s км/ч\n", data->valuestring );

          } // 0 element
        } // current_condition

        cJSON *weather = cJSON_GetObjectItemCaseSensitive(root, "weather");
        if( cJSON_IsArray( weather ) ) {
          // элементы 0, 1 и 2 - погода соответственно на сегодня, завтра и послезавтра
          cJSON *element0 = cJSON_GetArrayItem( weather, 0 ); // элемент 0 - наш
          if( element0 ) {
            cJSON *data = NULL;

            // температура как есть
            data = cJSON_GetObjectItemCaseSensitive( element0, "avgtempC" );
            if( cJSON_IsString( data ) && data->valuestring != NULL )
              printf( "Температура, средняя: %s °C\n", data->valuestring );
            data = cJSON_GetObjectItemCaseSensitive( element0, "maxtempC" );
            if( cJSON_IsString( data ) && data->valuestring != NULL )
              printf( "Температура, максим.: %s °C\n", data->valuestring );
            data = cJSON_GetObjectItemCaseSensitive( element0, "mintempC" );
            if( cJSON_IsString( data ) && data->valuestring != NULL )
              printf( "Температура, миним.: %s °C\n", data->valuestring );
          } // 0 element
        } // weather

        cJSON_Delete(root);
      }
    }

    /* cleanup curl stuff */
    curl_easy_cleanup(curl);
  }

  free(chunk.memory);

  /* we are done with libcurl, so clean it up */
  curl_global_cleanup();

  return (int)result;
}
