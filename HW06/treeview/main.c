#include <gtk/gtk.h>
#include <glib.h>
#include <glib/gprintf.h>

enum
{
  COL_NAME = 0,
  COL_ID,
  NUM_COLS
} ;

void open_dir( const char *dirname, GtkTreeIter *parent );

static GtkTreeStore *store = NULL;

int main( int argc, char **argv )
{
  GtkWidget *window = NULL;
  GtkWidget *treeview = NULL;
  GtkWidget *scrollw = NULL;
  GtkCellRenderer *renderer = NULL;
  GtkTreeViewColumn *column = NULL;
  gtk_init( &argc, &argv );

  store = gtk_tree_store_new( NUM_COLS
                              , G_TYPE_STRING
                              , G_TYPE_STRING
                            );

  open_dir( ".", NULL ); /* открываем от текущего положения пользователя в файловой системе */

  window = gtk_window_new( GTK_WINDOW_TOPLEVEL );
  gtk_window_set_default_size( GTK_WINDOW( window ), 800, 500 );
  gtk_window_set_title( GTK_WINDOW( window ), "Домашнее задание 6");
  g_signal_connect( G_OBJECT( window ), "destroy", gtk_main_quit, NULL );

  treeview = gtk_tree_view_new();
  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes( "Название",
                                                     renderer,
                                                     "text",
                                                     COL_NAME,
                                                     NULL );

  gtk_tree_view_append_column( GTK_TREE_VIEW( treeview ), column );

  gtk_tree_view_insert_column_with_attributes( GTK_TREE_VIEW( treeview ),
                                               -1,
                                               "DeviceID:inode (для защиты от 'рекурсивных' директорий)",
                                               renderer,
                                               "text", COL_ID,
                                               NULL );

  gtk_tree_view_set_model( GTK_TREE_VIEW( treeview ), GTK_TREE_MODEL( store ) );

  scrollw = gtk_scrolled_window_new(NULL, NULL);

  gtk_container_add( GTK_CONTAINER( window ), scrollw );
  gtk_container_add( GTK_CONTAINER( scrollw ), treeview );

  gtk_widget_show_all( window );

  gtk_main();
  return 0;
}

/*
static int level = 0;
 */
void open_dir( const char *dirname, GtkTreeIter *parent )
{
  GDir   *dir = NULL;
  GError *err = NULL;
  const gchar  *name = NULL;


  if( dirname == NULL ) return;

  dir = g_dir_open( dirname, 0, &err );
  if( dir == NULL )
  {
    if( err != NULL && err->message ) {
      g_fprintf( stderr, "%s\n", err->message );
      g_error_free( err );
      err = NULL;
    }
    return;
  }
/*
  ++level;
 */

  g_dir_rewind( dir );

  while( ( name = g_dir_read_name( dir ) ) != NULL )
  {
    GFile     *file = NULL;
    GFileInfo *info = NULL;
/*
    for( int i = 0; i < level ; i++ )
      g_print( " " );
 */

    gchar *dir_and_name = g_strdup_printf( "%s%c%s", dirname, G_DIR_SEPARATOR, name );
    if( dir_and_name == NULL )
      continue;

    if(    g_file_test( dir_and_name, G_FILE_TEST_IS_DIR ) == TRUE
/*      && g_file_test( dir_and_name, G_FILE_TEST_IS_SYMLINK ) == FALSE */
      )
    {
      GtkTreeIter iter;
      gchar *file_id = NULL;

      gtk_tree_store_append( store, &iter, parent );
      gtk_tree_store_set( store, &iter, COL_NAME, name, -1);

      /* для директории выясним уникальный идентификатор */
      file = g_file_new_for_path( dir_and_name );
      if( file != NULL ) {
        info = g_file_query_info(file,
                                 G_FILE_ATTRIBUTE_ID_FILE,
                                 G_FILE_QUERY_INFO_NONE,
                                 NULL, &err);
        g_object_unref(file);
      }
      if (err == NULL && info != NULL ) {
        /* это уникальный идентификатор - "device_id:inode" */
        const char *file_id_private = g_file_info_get_attribute_string(info, G_FILE_ATTRIBUTE_ID_FILE);

        if( file_id_private != NULL )
          file_id = g_strdup( file_id_private ); /* копируем его себе, т.к. он станет мусором через 2 строки кода ниже */
        /*
           Изначально была идея использовать только inode директории
           и хранить в невидимой колонке как тип G_TYPE_ULONG,
           что очень бы упростило код (не было бы работы со строками и динамической памятью для них).

           Но могуть встретиться одинаковые inode на разных файловых системах,
           которые в свою очередь могут ссылаться каталогами друг на друга.

           Поэтому, идентификатор каталога - составной (ИД_устройства:inode)
         */

        gtk_tree_store_set( store, &iter, COL_ID, file_id, -1 ); 
        g_object_unref(info);
      }
      if(err!=NULL)
      {
        g_error_free(err);
        err=NULL;
      }

/*
      g_printf( "+-%s\n", name );
 */

      /* Защита от 'рекурсивных' директорий, создаваемых символьными ссылками.
         При обработке вложенной директории проверим,
         вдруг она уже была выше */

      gboolean found = FALSE; /* данная директория пока не найдена выше */

      { /* область локальных переменных - лень тащить переменные в начало ветвления */
        GtkTreeIter up_iter;
        GtkTreeIter this_iter;

        /* Двигаемся по директориям вверх - до исходного */
        for( this_iter = iter;
             gtk_tree_model_iter_parent( GTK_TREE_MODEL( store ), &up_iter, &this_iter ) == TRUE ;
             this_iter = up_iter )
        {
          gchar *up_file_id = NULL;

          gtk_tree_model_get( GTK_TREE_MODEL( store ), &up_iter, COL_ID, &up_file_id, -1);
          if (up_file_id != NULL) {
            /* g_printf( "ID vs ID: %s vs %s\n", file_id, up_file_id ); */
            if( g_strcmp0( file_id, up_file_id ) == 0 ) /* если данный каталог уже есть выше по иерархии */
              found = TRUE; /* то отмечаем, что нашли */
            g_free(up_file_id);
          }
          if( found ) /* если нашли, то дальше нет смысла искать */
            break;    /* прерываем цикл */
        }
      }

      /* освобождаем свою строю копию, которую использовали для сравнения в поиске */
      if( file_id != NULL ) { 
        g_free( file_id );
        file_id = NULL;
      }

      if( ! found ) // если это новый каталог - обрабатываем его
        open_dir( dir_and_name, &iter );
    }
    else
    {
      GtkTreeIter iter;

      gtk_tree_store_append( store, &iter, parent );
      gtk_tree_store_set( store, &iter, COL_NAME, name, -1);

      /*
         Для файлов нет смысла обрабатывать идентификаторы
       */

/*
      g_printf( "--%s\n", name );
 */
    }

    g_free( dir_and_name );
    dir_and_name = NULL;
  }
  g_dir_close( dir );
/*
  --level;
 */
  return;
}
