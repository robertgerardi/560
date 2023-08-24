@echo off
REM This script runs by  MDK, so is run in <Project>
set PATH=%PATH;..\Tools\GetRegions
@echo on
GetRegions.exe %1 -c -s -oSource\Profiler\region.c 
