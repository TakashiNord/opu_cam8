//////////////////////////////////////////////////////////////////////////////
//
//  cam1.cpp
//
//  Description:
//      Contains Unigraphics entry points for the application.
//
//////////////////////////////////////////////////////////////////////////////

//  Include files
#include <uf.h>
#include <uf_exit.h>
#include <uf_ui.h>
#if ! defined ( __hp9000s800 ) && ! defined ( __sgi ) && ! defined ( __sun )
# include <strstream>
  using std::ostrstream;
  using std::endl;
  using std::ends;
#else
# include <strstream.h>
#endif
#include <iostream.h>

#include <uf_ui_ont.h>
#include <uf_oper.h>
#include <uf_obj.h>
#include <uf_part.h>
#include <uf_param.h>
#include <uf_ugopenint.h>
#include <uf_param_indices.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cam8.h"

#define UF_CALL(X) (report( __FILE__, __LINE__, #X, (X)))

static int report( char *file, int line, char *call, int irc)
{
  if (irc)
  {
     char    messg[133];
     printf("%s, line %d:  %s\n", file, line, call);
     (UF_get_fail_message(irc, messg)) ?
       printf("    returned a %d\n", irc) :
       printf("    returned error %d:  %s\n", irc, messg);
  }
  return(irc);
}

int _run_grip (char *, double , double  );
int do_cam8();
char *grip_exe = "opu_cam8.grx";

//----------------------------------------------------------------------------
//  Activation Methods
//----------------------------------------------------------------------------

//  Explicit Activation
//      This entry point is used to activate the application explicitly, as in
//      "File->Execute UG/Open->User Function..."
extern "C" DllExport void ufusr( char *parm, int *returnCode, int rlen )
{
    /* Initialize the API environment */
    int errorCode = UF_initialize();

    if ( 0 == errorCode )
    {
        /* TODO: Add your application code here */
        do_cam8();

        /* Terminate the API environment */
        errorCode = UF_terminate();
    }

    /* Print out any error messages */
    PrintErrorMessage( errorCode );
}

//----------------------------------------------------------------------------
//  Utilities
//----------------------------------------------------------------------------

// Unload Handler
//     This function specifies when to unload your application from Unigraphics.
//     If your application registers a callback (from a MenuScript item or a
//     User Defined Object for example), this function MUST return
//     "UF_UNLOAD_UG_TERMINATE".
extern "C" int ufusr_ask_unload( void )
{
     /* unload immediately after application exits*/
    return ( UF_UNLOAD_IMMEDIATELY );

     /*via the unload selection dialog... */
     //return ( UF_UNLOAD_SEL_DIALOG );
     /*when UG terminates...              */
     //return ( UF_UNLOAD_UG_TERMINATE );
}

/* PrintErrorMessage
**
**     Prints error messages to standard error and the Unigraphics status
**     line. */
static void PrintErrorMessage( int errorCode )
{
    if ( 0 != errorCode )
    {
        /* Retrieve the associated error message */
        char message[133];
        UF_get_fail_message( errorCode, message );

        /* Print out the message */
        UF_UI_set_status( message );

        fprintf( stderr, "%s\n", message );
    }
}



int do_cam8()
{
/*  Variable Declarations */
    tag_t display_part_tag;
    char str[133];

    tag_t  prg = NULL_TAG;
    int i , count = 0 ;
    int   obj_count = 0;
    tag_t *tags = NULL ;
    char  prog_name[UF_OPER_MAX_NAME_LEN+1];
    int type, subtype ;
    int cut_dir , set_cut_dir ;
    logical generated;
    int generat;
    int response ;
    char *mes1[]={
      "Программа предназначена для изменения параметров в операциях.",
      "Проводит изменение ( Попутного<->Встречного ) движения инструмента.",
      "Для этого, Вы должны нажать : кнопку 'Далее..'.. ",
      "   1) выбрать необходимые операции ",
      "   2) выбрать 'Попутное' или 'Встречное'. ",
      NULL
    };
    UF_UI_message_buttons_t buttons1 = { TRUE, FALSE, TRUE, "Далее....", NULL, "Отмена", 1, 0, 2 };
    char *mes2[]={
      "Производить генерацию операции после изменения?",
      NULL
    };
    UF_UI_message_buttons_t buttons2 = { TRUE, FALSE, TRUE, "Генерировать..", NULL, "Нет", 1, 0, 2 };
    char menu[14][38];

    /***********************************************************************/

    response=0;
    UF_UI_message_dialog("Cam.....", UF_UI_MESSAGE_INFORMATION, mes1, 5, TRUE, &buttons1, &response);
    if (response!=1) { return (0) ; }

    int module_id;
    UF_ask_application_module(&module_id);
    if (UF_APP_CAM!=module_id) {
       // UF_APP_GATEWAY UF_APP_CAM UF_APP_MODELING UF_APP_NONE
       uc1601("Запуск DLL - производится из модуля обработки\n - 2005г.",1);
       return (-1);
    }

    /* Ask displayed part tag */
    display_part_tag = UF_PART_ask_display_part();
    if (display_part_tag==NULL_TAG) {
      uc1601("Cam-часть не активна.....\n Операция прервана.",1);
      return (0);
    }

    /***********************************************************************/

    set_cut_dir=-9999;

    strcpy(&menu[0][0], "->Попутное  (Climb Cut)\0");
    strcpy(&menu[1][0], "->Встречное (Conventional Cut)\0");
    //strcpy(&menu[2][0], "->неопределенное (Undefined)\0");

    response=3;

    while (response >= 3 || response < 19 ) {

       response = uc1603("Преобразовать тип движения в->.",1,menu, 1+1);
       if ( response == 1 || response == 2 || response == 19 ) break;

       switch (response)
       {
          case 5 :set_cut_dir=1; // UF_PARAM_cut_dir_climb
          break ;
          case 6 :set_cut_dir=2; // UF_PARAM_cut_dir_conventional
          break ;
          case 7 :set_cut_dir=0; // UF_PARAM_cut_dir_undefined
          break ;
          default : break ;
        }

       // выбранные обьекты и их кол-во
       UF_CALL( UF_UI_ONT_ask_selected_nodes(&obj_count, &tags) );
       if (obj_count<=0) { uc1601("Не выбрано оперций!\n ...",1); }

       if (obj_count) {

         generat=1;
         UF_UI_message_dialog("Cam.....", UF_UI_MESSAGE_QUESTION, mes2, 1, TRUE, &buttons2, &generat);
         if (generat==2) { generat=0; }

         UF_UI_toggle_stoplight(1);

         for(i=0,count=0;i<obj_count;i++)
         {
            prg = tags[i]; // идентификатор объекта

            /* type =               subtype =
            //     программа=121              160
            //     операция =100              110 */
            UF_CALL( UF_OBJ_ask_type_and_subtype (prg, &type, &subtype ) );
            if (type!=UF_machining_operation_type) continue ;

            prog_name[0]='\0';
            //UF_OBJ_ask_name(prg, prog_name);// спросим имя обьекта
            UF_CALL( UF_OPER_ask_name_from_tag(prg, prog_name) );
            printf("%d) oper=%s ",i,prog_name);

            UF_CALL( UF_PARAM_ask_int_value(prg,UF_PARAM_CUT_DIR_TYPE,&cut_dir) );
            printf(" type =%d \n",cut_dir);

            // изменяем, если в операции другой тип
            if (cut_dir!=set_cut_dir){
              cut_dir=set_cut_dir;
              UF_CALL( UF_PARAM_set_int_value(prg,UF_PARAM_CUT_DIR_TYPE,cut_dir) );
              _run_grip ( prog_name, cut_dir, 0 ); // не будем генерировать в GRIP
              count++;
              if (generat==1) { UF_CALL( UF_PARAM_generate (prg,&generated ) ); }
            }

         }
         UF_free(tags);

         UF_DISP_refresh ();

         UF_UI_toggle_stoplight(0);

         str[0]='\0'; sprintf(str,"Изменено операций=%d \n ....",count);
         uc1601(str,1);

       }


    }

 return (0);
}

int _run_grip ( char *name, double cut_dir, double generat )
{
 int i , ready;
 char *path , envpath[255] , dllpath[255] ;

 char env_names[][25]={
  	"UGII_USER_DIR" ,
  	"UGII_SITE_DIR" ,
  	"UGII_VENDOR_DIR" ,
  	"UGII_GROUP_DIR" ,
  	"USER_UFUN" ,
  	"UGII_INITIAL_UFUN_DIR" ,
  	"UGII_INITIAL_GRIP_DIR" ,
  	"UGII_TMP_DIR" ,
  	"HOME" ,
  	"TMP" } ;

    double       user_response = 0;
    int          status = -1;
    int          grip_arg_count = 4;
    UF_args_t    grip_arg_list[4];

  /* Define the argument list for UG/Open API calling GRIP */
    grip_arg_list[0].type    = UF_TYPE_CHAR;
    grip_arg_list[0].length  = 0;
    grip_arg_list[0].address = name;
    grip_arg_list[1].type    = UF_TYPE_DOUBLE;
    grip_arg_list[1].length  = 0;
    grip_arg_list[1].address = &cut_dir;
    grip_arg_list[2].type    = UF_TYPE_DOUBLE;
    grip_arg_list[2].length  = 0;
    grip_arg_list[2].address = &generat;
    grip_arg_list[3].type    = UF_TYPE_DOUBLE;
    grip_arg_list[3].length  = 0;
    grip_arg_list[3].address = &user_response;

 path = (char *) malloc(255+50);

 ready=-1;
  for (i=0;i<10;i++) {
    dllpath[0]='\0';
    envpath[0]='\0';
    path=envpath;
    UF_translate_variable(env_names[i], &path);
    if (path!=NULL) {
       strcpy(dllpath,path);
       strcat(dllpath,"\\application\\");
       strcat(dllpath,grip_exe);
       UF_print_syslog(dllpath,FALSE);
       // работа с файлом
       UF_CFI_ask_file_exist (dllpath, &status );
       if (!status) { ready=1; break ; }
     } //if (envpath!=NULL) {
  } // for

 if (ready==-1) {
    printf ("Файл %s - не существует \n ...",grip_exe);
    return (-1);
 }

 /* Execute the GRIP program */
 status = UF_CALL(UF_call_grip (dllpath, grip_arg_count, grip_arg_list));

 if ( (status == 0) && (user_response == 0) )
 {
 	printf("param grip - change well\n");
 }

  return (0);
}
