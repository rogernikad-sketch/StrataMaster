Set objShell = CreateObject("Shell.Application")
objShell.ShellExecute "powershell.exe", "-NoProfile -ExecutionPolicy Bypass -File ""C:\dev\StrataMaster\tools\deploy_vst3.ps1""", "", "runas", 1
