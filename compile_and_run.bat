@echo off
cl shifter.c
IF %ERRORLEVEL% == 0 (
	shifter.exe
)