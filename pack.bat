@echo off

SET CC=gcc
SET CFLAGS=-Wall -Wextra -municode -DWIN_RELEASE
SET SRC=src/main.c src/glad.c
SET LIB=-Llibglfw -lglfw3dll -Lcglm -lcglm.dll -Wl,-subsystem,windows
SET INC=-Iinclude


call %CC% %SRC% %INC% %LIB% -o Wheeslide.exe