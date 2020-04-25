cam

Purpose:
  Программа для изменения параметров операции.
  Меняет тип движения (встречное<->попутное).

Requirements for this package:
  UG V17/V18/NX1/NX2/....
  WindowsNT/2000/XP
  UG/Gateway
  UG/Manufacturing
  UG/Open API Execute
  UG/Grip
  UG/Grip Execute

Restrictions:
  not

Author:
  че
 

Quick installation:
   Move 'cam.dll cam.grx' to one of the following directories:
     %UGII_USER_DIR%\application 
     %UGII_SITE_DIR%\application 
     %UGII_VENDOR_DIR%\application 
  - for calling 'cam.dll' via File->Execute UG/Open->User Function or User Tool.

  The required enironment-variable (i.e. UGII_USER_DIR or UGII_SITE_DIR or
  UGII_VENDOR_DIR) must be set properly on your system.

Most common problem:
  проблема была обнаружена при создании.
  Суть : есть группа разнородных операций
   Fixed
   Planar
   Cavity
    каждая из них имеет параметры
     Cut Flag           =  Conventional
     Cut Direction Flag =  Climb
     Cut Direction      =  Forward

    средствами UG/Open, используя параметр UF_PARAM_CUT_DIR_TYPE
    меняется только 'Cut Flag', и фактически происходит изменение типа резания
    только в операциях типа Fixed. В операцих Planar изменяется 'Cut Flag',но 
    'Cut Direction Flag' не изменяется (как было 'встречное' так и осталося(например)).
    для изменения данного параметра была написана дополнительная программа на Grip, 
    вызываемая из UG/Open (.dll).

  MILL_PLANAR 
    FACE_MILLING     (261)  -
    PLANAR_MILL      (110)  +
  MILL_CONTOUR
    CAVITY_MILL      (260)  +
    FIXED_CONTOUR    (210)  +
    PLUNGE_MILLING   (265)  -
    ZLEVEL_PROFILE   (263)  -
  MILL_MULTI-AXIS
    VARIABLE_CONTOUR (265)  +
    SEQUENTIAL_MILL  (265)  -


Copyright & remarks:
  Thanks to my collegs.
  If you have added an interesting enhancement/port or want to report
  an error, please contact the author at mail

History:
  29-june-2007: V1.0.0, Initial release V1.0, UG V18
