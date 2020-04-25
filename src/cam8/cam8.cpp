//////////////////////////////////////////////////////////////////////////////
//
//  cam1.cpp
//
//  Description:
//      Contains Unigraphics entry points for the application.
//
//////////////////////////////////////////////////////////////////////////////

#define _CRT_SECURE_NO_DEPRECATE 1

//  Include files
#include <uf.h>
#include <uf_exit.h>
#include <uf_ui.h>

/*
#if ! defined ( __hp9000s800 ) && ! defined ( __sgi ) && ! defined ( __sun )
# include <strstream>
  using std::ostrstream;
  using std::endl;
  using std::ends;
#else
# include <strstream.h>
#endif
#include <iostream.h>
*/

#include <uf_ui_ont.h>
#include <uf_oper.h>
#include <uf_ncgroup.h>
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

/*----------------------------------------------------------------------------*/
#define COUNT_GRP 300

struct GRP
{
   int     num;
   tag_t   tag;
   char    name[UF_OPER_MAX_NAME_LEN+1];
} ;

static struct GRP grp_list[COUNT_GRP] ;
int grp_list_count=0;

/* */
/*----------------------------------------------------------------------------*/
static logical cycleGenerateCb( tag_t   tag, void   *data )
{
   logical  is_group ;
   char     name[UF_OPER_MAX_NAME_LEN + 1];
   int      ecode;

   ecode = UF_NCGROUP_is_group( tag, &is_group );
   //if( is_group != TRUE ) return( TRUE );//!!!!!!!!!!!!!!!!!! важное условие

   name[0]='\0';
   ecode = UF_OBJ_ask_name(tag, name);// спросим имя обьекта
   //UF_UI_write_listing_window("\n");  UF_UI_write_listing_window(name);

   if (grp_list_count>=COUNT_GRP) return( FALSE );
   grp_list[grp_list_count].num=grp_list_count;
   grp_list[grp_list_count].tag=tag;
   grp_list[grp_list_count].name[0]='\0';
   sprintf(grp_list[grp_list_count].name,"%s",name);
   grp_list_count++;

   return( TRUE );
}

char *grip_exe = "opu_cam8.grx";
char grip_exe_path[256];

/*----------------------------------------------------------------------------*/
int _run_grip_init ( )
{
 int i , ready , status ;
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
    uc1601("GRIP Файл не существует ",1);
    return (-1);
 }

 grip_exe_path[0]='\0';
 sprintf(grip_exe_path,"%s",dllpath);

 return (0);
}


/*----------------------------------------------------------------------------*/
int _run_grip ( char *name, double cut_dir, double generat )
{
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

 /* Execute the GRIP program */
    status = UF_call_grip (grip_exe_path, grip_arg_count, grip_arg_list) ;
    printf("UF_call_grip=%d user_response=%d \n",status,user_response);

  if ( (status == 0) && (user_response == 0) ) { ; }

  return ((int)user_response);
}


/*----------------------------------------------------------------------------*/
int _run_change ( tag_t prg , int set_cut_dir, int generat)
{
  int cut_dir ;
  char  str[256];
  int   type_oper ;
  int   ret ;
  char  grp_name[UF_OPER_MAX_NAME_LEN+1],prog_name[UF_OPER_MAX_NAME_LEN+1];
  int   type, subtype ;
  logical logresp;
  tag_t  group;

  ret = 0 ; // возвращаемое значение

  /* type =               subtype =
  //     программа=121              160
  //     операция =100              110 */
  UF_CALL( UF_OBJ_ask_type_and_subtype (prg, &type, &subtype ) );
  if (type!=UF_machining_operation_type) return(0);

  prog_name[0]='\0';
  //UF_OBJ_ask_name(prg, prog_name);// спросим имя обьекта
  UF_CALL( UF_OPER_ask_name_from_tag(prg, prog_name) );
  printf(" oper=%s ",prog_name);

/*
UF_machining_operation_type             100        UF_machining_parameter_set_type         107
   UF_mach_instanced_oper_subtype              1      UF_mach_mill_post_cmnds_subtype            11
   UF_mach_orphan_oper_subtype                 2      UF_mach_pocket_subtype                    110
   UF_mach_oldopr_subtype                     10      UF_mach_surface_contour_subtype           210
   UF_mach_pocket_subtype                    110      UF_mach_vasc_subtype                      211
   UF_mach_surface_contour_subtype           210      UF_mach_param_line_subtype                230
   UF_mach_vasc_subtype                      211      UF_mach_zig_zag_surf_subtype              240
   UF_mach_cavity_milling_subtype            260      UF_mach_rough_to_depth_subtype            250
   UF_mach_face_milling_subtype              261      UF_mach_cavity_milling_subtype            260
   UF_mach_volumn_milling_subtype            262      UF_mach_drill_subtype                     350
   UF_mach_zlevel_milling_subtype            263      UF_mach_point_to_point_subtype            450
   UF_mach_fb_hole_milling_subtype           264      UF_mach_seq_curve_mill_subtype            460
   UF_mach_plunge_milling_subtype            265      UF_mach_seq_curve_lathe_subtype           461
                                                      UF_mach_mill_ud_subtype                   800
                                                      UF_mach_mill_mc_subtype                  1100
*/
  UF_CALL( UF_OPER_ask_oper_type(prg,&type_oper) );
  printf(" (%d) \n",type_oper);

  if (type_oper==UF_mach_instanced_oper_subtype || \
       type_oper==UF_mach_orphan_oper_subtype || \
        type_oper==UF_mach_oldopr_subtype || \
         type_oper==UF_mach_pocket_subtype || \
          type_oper==UF_mach_vasc_subtype || \
           type_oper==UF_mach_surface_contour_subtype || \
            type_oper==UF_mach_cavity_milling_subtype) {

    UF_CALL( UF_PARAM_ask_int_value(prg,UF_PARAM_CUT_DIR_TYPE,&cut_dir) );
    printf("\t\tUF_PARAM_ask_int_value Cut Flag=%d (%d)\n",cut_dir,set_cut_dir);
    if (type_oper==UF_mach_surface_contour_subtype || \
         type_oper==UF_mach_vasc_subtype) {
        if (cut_dir!=set_cut_dir){ // изменяем, если в операции другой тип
          cut_dir=set_cut_dir;
          UF_CALL( UF_PARAM_set_int_value(prg,UF_PARAM_CUT_DIR_TYPE,cut_dir) );
          ret=1;
          printf(" \t\tUF_PARAM_set_int_value - change \n");
        }
     } else {
        cut_dir=set_cut_dir;
        ret=_run_grip ( prog_name, cut_dir, 0 ); // не будем генерировать в GRIP
        printf(" \t\t - ret=%d=_run_grip() ",ret);
    }
    //
    if (generat==1&&ret!=0) {
    	UF_CALL( UF_PARAM_generate (prg,&logresp ) );
    	printf(" \t\t - generate - %d \n",logresp);
    }


  } else {
  	UF_UI_is_listing_window_open (&logresp);
  	if (FALSE==logresp) {
  		 UF_UI_open_listing_window();
  		 UF_UI_write_listing_window("\n Программа изменения направления фрезерования.\n$$======================================\n");
  	}
    UF_CALL( UF_OPER_ask_program_group(prg,&group) ) ;
    grp_name[0]='\0'; UF_CALL(UF_OBJ_ask_name(group, grp_name));// спросим имя обьекта
    str[0]='\0'; sprintf(str,"\n Невозможно изменить операцию=%s (%d) Программа=%s",prog_name,type_oper,grp_name);
    UF_UI_write_listing_window(str);
  }

 return(ret);
}


/*----------------------------------------------------------------------------*/
int do_cam8()
{
/*  Variable Declarations */
    tag_t display_part_tag;
    char str[133];

    tag_t  prg = NULL_TAG;
    int   i , j , count = 0 , obj_count = 0;
    tag_t *tags = NULL ;
    int set_cut_dir , generat;
    int response ;
    char *mes1[]={
      "Программа для изменения ( Попутного<->Встречного ) параметров в операциях.",
      "Может изменять токо ограниченный набор операций (помечены знаком '+'),",
      " на остальные типы операций-нет документации..",
      "Важно! Перед запуском программы - рекомендуем сохранить ранее сделанные изменения.",
      " ",
      " шаблон MILL_PLANAR ",
      " \t\t  FACE_MILLING     (261)  -",
      " \t\t  PLANAR_MILL      (110)  +",
      " шаблон MILL_CONTOUR",
      " \t\t  CAVITY_MILL      (260)  +",
      " \t\t  FIXED_CONTOUR    (210)  +",
      " \t\t  PLUNGE_MILLING   (265)  -",
      " \t\t  ZLEVEL_PROFILE   (263)  -",
      " шаблон MILL_MULTI-AXIS",
      " \t\t  VARIABLE_CONTOUR (265)  +",
      " \t\t  SEQUENTIAL_MILL  (265)  -",
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
    UF_UI_message_dialog("Cam.....", UF_UI_MESSAGE_INFORMATION, mes1, 17, TRUE, &buttons1, &response);
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

 if (0<_run_grip_init()) { return 0 ; }

 set_cut_dir=-9999;

/*
 strcpy(&menu[0][0], "->Попутное  (Climb Cut)\0");
 strcpy(&menu[1][0], "->Встречное (Conventional Cut)\0");
 strcpy(&menu[2][0], "->неопределенное (Undefined)\0");
*/
 strcpy(&menu[0][0], "Climb Cut\0");
 strcpy(&menu[1][0], "Conventional Cut\0");
 strcpy(&menu[2][0], "Undefined\0");

 response=3;
 while (response >= 3 || response < 19 ) {

   response = uc1603(".Преобразовать тип движения в->.",1,menu, 2+1);
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

        grp_list_count=0;// заполняем структуру
        UF_CALL(  UF_NCGROUP_cycle_members( prg, cycleGenerateCb,NULL ) );

   	    // эти изменения мы могли проводить в cycleGenerateCb
   	    // но я оставил так как было.
        if (grp_list_count==0) {
        	count+=_run_change ( prg , set_cut_dir, generat);
        } else for (j=0;j<grp_list_count;j++) {
   	               count+=_run_change ( grp_list[j].tag , set_cut_dir, generat);
               }

     }
     UF_free(tags);
     //UF_DISP_refresh ();
     UF_UI_toggle_stoplight(0);

     str[0]='\0'; sprintf(str,"Изменено операций=%d \n ....",count);
     uc1601(str,1);
     // break ; // прерываем выполнении программы
    }

 }// end while

 return (0);
}


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
