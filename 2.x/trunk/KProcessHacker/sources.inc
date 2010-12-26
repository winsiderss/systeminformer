TARGETTYPE=DRIVER

!IF !DEFINED(TARGETNAME)
TARGETNAME=kprocesshacker
!ENDIF

!IF !DEFINED(TARGETPATH)
TARGETPATH=..\bin
!ENDIF

INCLUDES=$(DDK_INC_PATH);..\include;..\..\phlib\include
LIBS=%BUILD%\lib

SOURCES= \
    ..\main.c \
    ..\devctrl.c \
    ..\dyndata.c \
    ..\dynimp.c \
    ..\object.c \
    ..\process.c \
    ..\qrydrv.c \
    ..\thread.c \
    ..\vm.c \
    ..\resource.rc
