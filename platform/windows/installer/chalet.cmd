
@ECHO OFF

SETLOCAL

SET "CHALET_EXE=%~dp0\chalet.exe"
IF NOT EXIST "%CHALET_EXE%" (
  SET "CHALET_EXE=chalet"
)

"%CHALET_EXE%" %*
