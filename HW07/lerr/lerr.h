#ifndef    __LOGGERR_H__
#define    __LOGGERR_H__

#include <signal.h>


#ifdef __cplusplus
extern "C" {
#endif

#define MAX_STACK_TRACE_DEPTH (20)

/*
 Подглядел в RFC 5424, п. 6.2.1

 Numerical         Severity
             Code

              0       Emergency: system is unusable
              1       Alert: action must be taken immediately
              2       Critical: critical conditions
              3       Error: error conditions
              4       Warning: warning conditions
              5       Notice: normal but significant condition
              6       Informational: informational messages
              7       Debug: debug-level messages

              Table 2. Syslog Message Severities
 */
typedef enum
{
  LERR_FATAL = 0,
  LERR_ALERT,
  LERR_CRIT,
  LERR_ERROR,
  LERR_WARN,
  LERR_NOTICE,
  LERR_INFO,
  LERR_DEBUG,
  LERR_UNKN
} lerr_level_t;


/*
 * Инициализация библиотеки сообщений
 *   out_log_file - путь/до/файла_журнала
 * 
 * В случае ошибки доступа к файлу журнала - аварийное завершение программы
 */
int  lerr_init        (const char *out_log_file);


/*
 * Освободить ресурсы библиотеки сообщений
 */
void lerr_exit        ();

/*
 * Требуется остановка программы
 */
sig_atomic_t
     lerr_is_need_stop();

/*
 * Включить дублирование на стандартный вывод об ошибках
 */
void lerr_stderr_on   ();


/*
 * Отключить дублирование на стандартный вывод об ошибках
 */
void lerr_stderr_off  ();


/*
 * Записать сообщение в журнал
 *   level  - уровень значимости
 *   ...   - "printf-формат сообщения" [, аргументы согласно формату сообщения ...]
 */
#define \
     lerr_mess(level, ...) \
     lerr_mess_intern ((level), __FILE__, __func__, __LINE__, ##__VA_ARGS__)

/*
#define \
     lerr_mess(level, format, ...) \
     lerr_mess_intern ((level), __FILE__, __func__, __LINE__, (format), ##__VA_ARGS__)
 */


/*
 * Записать сообщение в журнал (полный вызов с предопределенными макросами)
 */
void lerr_mess_intern (lerr_level_t  level,
                       const char   *file,
                       const char   *func,
                       int           line,
                       const char   *fmt, ... );

#ifdef __cplusplus
};
#endif

#endif  /* __LOGGERR_H__ */
