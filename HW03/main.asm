    bits 64
    extern malloc, puts, printf, fflush, abort
    global main

    section   .data
empty_str: db 0x0           ; \0 ("")
int_format: db "%ld ", 0x0  ; спецификатор для long int и один пробел для отделения чисел друг от друга
data: dq 4, 8, 15, 16, 23, 42, 43, 21, 44, 24, 3, 2 ; dq - размер элемента данных 8 байт
                                                    ; массив целых чисел
data_length: equ ($-data) / 8 ; $    - текущий адрес (т.е. адрес data_length)
                              ; data - начальный адрес массива целых чисел
                              ; ($ - data) разница в адресах, т.е. размер массива в байтах
                              ; 8 - размер одного элемента
                              ; ($-data) / 8 - это количество элементов в массиве

    section   .text
;;; print_int proc
print_int:               ; print_int( int arg1 )
    push rbp
    mov rbp, rsp
    sub rsp, 16

    mov rsi, rdi         ; arg2; arg1
    mov rdi, int_format  ; arg1 "%ld"
    xor rax, rax         ;
    call printf          ; printf( arg1, arg2).. printf( "%ld", agr1 )
                         ; в AX сохранится число выведеных на стандартный вывод символов
                         ; т.е. результат вызова printf() см. man 3 printf

    xor rdi, rdi         ; arg1=0
    call fflush          ; fflush(0) или fflush(NULL)
                         ; AX=0 (возвращаемое значение 0 - если fflush() успешен)

                         ; fflush(NULL), Если аргумент NULL, то согласно man 3 fflush,
                         ; идет сброс буферов всех открытых выводов

    mov rsp, rbp
    pop rbp
    ret


;;; p proc
p:                   ; int parity(int arg1) {
    mov rax, rdi     ;   int ret = agr1;
    and rax, 1       ;   ret &= 1;  проверка на НЕчетность, p - это parity
    ret              ;   return ret; // через rax
; ^ здесь нет манипуляций со стеком... 

;;; add_element proc
add_element:             ; add_element( data[s]. i )
                         ; add_element( long int, int )
                         ; add_element( long int, struct list *list)
                         ;              num     , prev_element
                         ;              rdi       rsi

    push rbp             ; сохраняем адрес возврата
    push rbx             ; и другие регистры, которые будем здесь
    push r14             ; модифицировать

    mov rbp, rsp         ; стековый фрейм
    sub rsp, 16          ; область локальных переменных

    mov r14, rdi         ; arg1; data[s], num
    mov rbx, rsi         ; arg2; i,       prev_element

    mov rdi, 16          ; arg1; 16
    call malloc          ; long int *ptr = malloc( 16 );
                         ; list_t   *ptr = (list_t*)malloc( sizeof( list_t) );
                         ; результат в AX - это адрес на в куче, который выделен
                         ;                  или 0 (NULL) в случае неудачи
    test rax, rax        ; if( ptr == NULL ) если NULL; то вызываем 
    jz abort             ;   abort();        ОСТАНОВ программы (функция ничего не возвращает)
                         ;                   и из нее нет возврата (безусловный переход)

    mov [rax], r14       ; ptr->num = num - нечетный элемент
    mov [rax + 8], rbx   ; ptr->prev = arr | NULL (на первой итерации)

    mov rsp, rbp         ; восстанавливаем стековый фрейм

    pop r14              ; регистры, которые модифицировали и
    pop rbx              ; использовали в вычислениях
    pop rbp              ; адрес возврата (базовый адрес прошлой функции)
    ret

;;; m(ap) - аналог MAP функции высшего порядка,
;;;         вызывающей функцию для элементов списка массива
;;; m proc
m:                   ; m( *list ,  *print_int )
                     ;     rdi      rsi

    push rbp         ; адрес возврата
    mov rbp, rsp     ; стековый фрейм
    sub rsp, 16      ; область локальных переменных

    test rdi, rdi    ; if( arg1 == 0 )
    jz outm          ; {}
                     ; else
                     ; {
    push rbp         ; сохраним регистры,

    push rbx         ; т.к. будем их изменять
    mov rbx, rdi     ; *list
    mov rbp, rsi     ; говорим, что мы находимся теперь в функции print_int

    mov rdi, [rdi]   ; arg1 = list->num
    call rsi         ; делаем вызов print_int

    mov rdi, [rbx + 8] ; list->prev
    mov rsi, rbp       ; print_int
    call m             ; m( list->prev, print_int ) ; рекурсия

    pop rbx          ; восстанавливаем
    pop rbp          ; регистры

outm:                ; }
    mov rsp, rbp
    pop rbp
    ret

;;; f(ilter) - аналог FILTER-функции высшего порядка
;;; f proc
f:                       ; f( list, new_list,  (int)(*parity(int)) )
                         ;    rdi   rsi        rdx

    mov rax, rsi         ; arg2; *ret = new_list

    test rdi, rdi        ; if( list == NULL)
    jz outf              ;   return;

    push rbx             ; бэкапим регистры (в rbx был указатель на голову значение list)
    push r12             ; сохраняем регистры,
    push r13             ; т.к. будем их использовать в алгоритмах

    mov rbx, rdi         ; arg1; list
    mov r12, rsi         ; arg2; new_list
    mov r13, rdx         ; arg3 &parity()

    mov rdi, [rdi]       ; arg1, list->num
    call rdx             ; четность=p( list->num ) //вызывается функция по ссылке p(с аргументом s)
    test rax, rax        ; if( четность == 0) 
    jz z                 ; {}
                         ; else
                         ; { если элемент нечетный, 
                         ;   то надо добавить его в новый список

    mov rdi, [rbx]       ; arg1; list->num
    mov rsi, r12         ; arg2; new_list (на начало итерации)
    call add_element     ; add_element( data[s], new_list)
                         ; в AX = указатель на новый элемент списка
    mov rsi, rax         ; arg2;  new_list (новый)

    jmp ff

z:                       ; если data[s] четная

    mov rsi, r12         ; arg2; new_list (исходный)

ff:
    mov rdi, [rbx + 8]   ; arg1, list->prev
    mov rdx, r13         ; arg3 адрес на функцию p()
    call f               ; new_list = f( list->prev, new_list, p() ) // рекурсивный вызов

    pop r13              ; восстанавливаем "испорченные" регистры
    pop r12
    pop rbx              ; здесь снова указатель на *list

outf:                    ; в AX - новый new_list или неизменный new_lists
    ret

;;; main proc
main:
    mov rbp, rsp; for correct debugging
    push rbx                       ; сохраняем значение регистра, т.к. мы его "испортим", задействовав в вычислениях.

    xor rax, rax                   ; *list = NULL;
    mov rbx, data_length           ; s = data_len
adding_loop:                       ; do {
    mov rdi, [data - 8 + rbx * 8]  ; arg1: 
                                   ; готовим 1й аргумент по формуле
                                   ; long int ptr* = &data[0] + s*(sizeof(long int)) - (sizeof(long int))
                                   ; data + ( rbx * 8 - 8 ) = data + 8 ( rbx - 1 )
                                   ; &data[ 0 ] + sizeof( long int )*(s - 1)
                                   ;  data[ s - 1 ]
                                   ; на первой итерации - это последний элемент массива,
                                   ; на последующих итерациях двигаемся к началу массива
                                   ; см. dec rbx далее

    mov rsi, rax                   ; arg2, это *list

    call add_element               ; list = add_element( data[ s - 1 ], list )

    dec rbx                        ; s--

    jnz adding_loop                ; } while (s!=0) // if( s!= 0 ) goto loop

    mov rbx, rax                   ; // сохраним начальный элемент списка list

    mov rdi, rax                   ; arg1, list
    mov rsi, print_int             ; arg2, это указатель на функцию
    call m                         ; m( list, print_int() )

    mov rdi, empty_str             ; arg1, empty_str
    call puts                      ; puts( "" \0 )

    mov rdx, p                     ; arg3; unsigned int(*p)(unsigned int)
    xor rsi, rsi                   ; arg2; *new_list (сейчас равен NULL)
    mov rdi, rbx                   ; arg1; *list
    call f                         ; new_list = f( list, new_list, (int)(*parity(int)) )
                                   ; в AX расположен new_list

    mov rdi, rax                   ; arg1; list = new_list
                                   ; мы только что потеряли указаталь на изначальный список list
                                   ; тем самым допустим утечку памяти
                                   ; и освободить его уже не получится))
                                   ; хотя нет.. ссылка на изначальный массив

                                   ; вызываем MAP нового списка для функции print_int
    mov rsi, print_int             ; arg2; указатель на ф-цию print_int
    call m                         ; m( new_list , print_int )

    mov rdi, empty_str             ; arg1; empty_str
    call puts                      ; puts(empty_str)

    pop rbx                        ; восстанавливаем регистр, т.к. мы его "попортили"

    xor rax, rax                   ; return 0; // возвращаем 0
    ret
