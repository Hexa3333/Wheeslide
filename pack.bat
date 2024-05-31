@echo off

SET CC=gcc
SET CFLAGS=-DWIN_RELEASE
SET SRC=src/main.c src/glad.c
SET LIB=-Llibglfw -lglfw3dll -Lcglm -lcglm.dll -Wl,-subsystem,windows
SET INC=-Iinclude


call %CC% %SRC% %INC% %LIB% %CFLAGS% -o Wheeslide.exe