@echo off

set LIBS=-lmsvcrt -lraylib -lOpenGL32 -lGdi32 -lWinMM -lkernel32 -lshell32 -lUser32 
set LNK_OPT=-Wl,/NODEFAULTLIB:libcmt,/SUBSYSTEM:WINDOWS

clang -o %~n1.exe %1 %LIBS% %LNK_OPT%
