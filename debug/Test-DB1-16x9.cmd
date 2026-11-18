@echo off
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0Test-DB1-16x9.ps1" -RomPath "%~1"
pause
