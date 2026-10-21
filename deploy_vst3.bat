@echo off
setlocal EnableDelayedExpansion
title StrataMaster VST3 Deployment Tool

echo ==============================================================================
echo StrataMaster VST3 Production Deployment & Cubase Sync Tool
echo ==============================================================================
echo.

:: Check for Administrator privileges
net session >nul 2>&1
if %errorLevel% neq 0 (
    echo [!] Administrator privileges are required to update C:\Program Files\Common Files\VST3.
    echo [!] Requesting elevation...
    powershell -NoProfile -ExecutionPolicy Bypass -Command "Start-Process cmd -ArgumentList '/k \"\"%~dpnx0\"\"' -Verb RunAs"
    exit /b
)

echo [*] Running with Administrator privileges...
powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0tools\deploy_vst3.ps1"
if %errorLevel% neq 0 (
    echo.
    echo [ERROR] Deployment failed. Check the error message above.
    pause
    exit /b 1
)

echo.
echo ==============================================================================
echo [SUCCESS] StrataMaster VST3 has been updated and verified!
echo           Cubase cache cleared. Reopen Cubase to load the new modular UI.
echo ==============================================================================
echo.
pause
