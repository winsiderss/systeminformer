TARGETTYPE=DRIVER

!IF !DEFINED(TARGETNAME)
TARGETNAME=kprocesshacker
!ENDIF

!IF !DEFINED(TARGETPATH)
TARGETPATH=..\bin
!ENDIF

TARGETLIBS=$(TARGETLIBS) $(DDK_LIB_PATH)\ksecdd.lib

INCLUDES=$(DDK_INC_PATH);..\include;..\..\phnt\include;..\..\phlib\include
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
	..\util.c \
	..\verify.c \
	..\vm.c \
	..\resource.rc
