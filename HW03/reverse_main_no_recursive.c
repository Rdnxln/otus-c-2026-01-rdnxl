#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

ssize_t   data[]       = { 4, 8, 15, 16, 23, 42, 43, 21, 44, 24, 3, 2 };
size_t    data_length  = ( sizeof(data) / sizeof(data[0]) );
char      empty_string = 0;

typedef  struct list  list_t;
struct list
{
  ssize_t   num;
  list_t    *prev; /* не next, а prev потому что сохраняется указатель
                      на ранее добавленные части списка */
};

/*
  Проверка четности
    0 - num четное
    1 - num нечетное
 */
/* inline */ ssize_t  p( ssize_t num )
{
  return  ( num & 0x1 );
}

/*
  Вывод целого числа с разделитетем
 */
void print_int( ssize_t num )
{
  printf( "%ld ", num );
  fflush( NULL );
}


/*
  MAP-функция для элементов списка
  с вызовом фукнции func_for_element
 */
void m( list_t *list, void (*func_for_element)(ssize_t num) )
{
  for( ; list != NULL ; list=list->prev )
    func_for_element( list->num );
}


/*
  Освобождение динамически выделенной памяти
 */
void free_list( list_t *list )
{
  list_t *it = list;
  while( it != NULL )
  {
    /* сохраняем указатель на соседний элемент из текущего элемента,
       пока память текущего элемента еще "наша" */
    list_t *prev = it->prev;

    free( it ); /* освобождаем память под текущий элемент */

    it = prev;  /* указатель на соседний элемент теперь берем
                   как текущий для следующей итерации */
  }
}


list_t *add_element( ssize_t num, list_t *list )
{
  list_t *new_head = (list_t*) malloc( sizeof(list_t) );
  if( new_head == NULL )
    abort();

  new_head->num  = num;
  new_head->prev = list;

  return new_head;
}

/*
  FILTER-функция, формирующая подмножество new_list
         от исходного списка list
         с использованием функции-условия (фильтра)
 */
list_t* f( list_t *list, list_t *new_list, ssize_t (*condition_func)(ssize_t num) )
{
  for( ; list != NULL; list = list->prev )
    if( condition_func( list->num ) != 0 )
      new_list = add_element( list->num, new_list );

  return new_list;
}

int main( int argc __attribute__((unused)), char **argv __attribute__((unused)) )
{
  list_t* list = NULL;
  size_t  s    = data_length;

  /* создаем односвязный список по массиву целых чисел */
  do
  {
    list = add_element( data[ s -1 ], list );
    s--;
  }
  while (s != 0);

  /* выводим значения в списке */
  m( list, print_int ); /* своебразный foreach_func() */
  /* вывод списка завершаем переводом строки */
  puts( &empty_string ); /* указатель на строку, из одного символа,
                            и это символ ('\0') конца строки.
                            Согласно man 3 puts, вызов выводит строку и
                            добавляет new_line */

  /* фильтруем исходный список list
     в целевой список odd_list нечетных целых 
     по условию с использованием функции p */
  list_t *odd_list = NULL; /* изначально список пустой */
  odd_list = f( list, odd_list, p ); 

  /* выводим отфильтрованный список ... */
  m( odd_list, print_int );
  /* ... и перевод строки*/
  puts( &empty_string );

  free_list( odd_list );
  odd_list = NULL;

  free_list( list );
  list = NULL;

  return 0;
}
