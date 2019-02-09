@echo off
cl /D_DEBUG shifter.c
IF %ERRORLEVEL% == 0 (
	shifter.exe Readme.txt 簡易マニュアル＆キーコンフィグ.txt
)