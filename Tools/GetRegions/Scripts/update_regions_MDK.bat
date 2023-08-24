@echo off
set PATH=%PATH;..\..\Tools\GetRegions\
@echo on
GetRegions.exe %1 -c -s -oSource\Profiler\region.c
