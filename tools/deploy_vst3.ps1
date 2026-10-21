# StrataMaster VST3 Automated Deployment & Cubase Sync Script
$ErrorActionPreference = "Stop"

$repoRoot = "C:\dev\StrataMaster"
$sourceDir = "$repoRoot\build\StrataMaster_artefacts\Release\VST3\StrataMaster.vst3"
$systemVst3Dir = "C:\Program Files\Common Files\VST3\StrataMaster.vst3"
$userVst3Dir = "$env:LOCALAPPDATA\Programs\Common\VST3\StrataMaster.vst3"
$logFile = "$repoRoot\build\deploy_status.txt"
$cubaseCacheFile = "$env:APPDATA\Steinberg\Cubase 14_64\Cubase Pro VST3 Cache\vst3plugins.xml"

"=== StrataMaster VST3 Deployment Started at $(Get-Date) ===" | Out-File $logFile

try {
    # 1. Close Cubase if running to release file locks
    $cubaseProc = Get-Process -Name "Cubase14" -ErrorAction SilentlyContinue
    if ($cubaseProc) {
        Write-Host "Closing Cubase 14 to release VST3 binary lock..." -ForegroundColor Yellow
        $cubaseProc | Stop-Process -Force -ErrorAction SilentlyContinue
        Start-Sleep -Seconds 1
        "Closed running Cubase 14 instance (PID: $($cubaseProc.Id))" | Out-File $logFile -Append
    }

    # 2. Check source binary exists
    $sourceDll = Join-Path $sourceDir "Contents\x86_64-win\StrataMaster.vst3"
    if (!(Test-Path $sourceDll)) {
        throw "Source VST3 binary does not exist at $sourceDll. Please build the project first."
    }
    $sourceHash = (Get-FileHash -Algorithm SHA256 $sourceDll).Hash
    $sourceLength = (Get-Item $sourceDll).Length
    Write-Host "Source VST3: $sourceLength bytes, SHA256: $sourceHash" -ForegroundColor Cyan
    "Source binary: $sourceLength bytes, Hash: $sourceHash" | Out-File $logFile -Append

    # 3. Clean duplicate or outdated .old files in system directory
    $oldFile = Join-Path $systemVst3Dir "Contents\x86_64-win\StrataMaster.vst3.old"
    if (Test-Path $oldFile) {
        Remove-Item $oldFile -Force -ErrorAction SilentlyContinue
        Write-Host "Removed duplicate outdated file: $oldFile" -ForegroundColor Green
        "Removed duplicate: $oldFile" | Out-File $logFile -Append
    }

    # 4. Copy to System VST3 Directory (C:\Program Files\Common Files\VST3\StrataMaster.vst3)
    if (!(Test-Path "C:\Program Files\Common Files\VST3")) {
        New-Item -Path "C:\Program Files\Common Files\VST3" -ItemType Directory -Force | Out-Null
    }
    
    # Copy files
    Write-Host "Installing to System VST3: $systemVst3Dir..." -ForegroundColor Cyan
    Copy-Item -Path $sourceDir -Destination "C:\Program Files\Common Files\VST3" -Recurse -Force
    "Copied bundle to $systemVst3Dir" | Out-File $logFile -Append

    # 5. Grant Full Control to Users group on the StrataMaster.vst3 directory
    # This ensures future CMake builds (COPY_PLUGIN_AFTER_BUILD) can overwrite without UAC
    Write-Host "Configuring folder permissions for future non-elevated builds..." -ForegroundColor Cyan
    & icacls "$systemVst3Dir" /grant "Users:(OI)(CI)F" /T /Q
    "Granted Users FullControl on $systemVst3Dir" | Out-File $logFile -Append

    # 6. Also sync to User VST3 directory (%LOCALAPPDATA%\Programs\Common\VST3)
    if (!(Test-Path "$env:LOCALAPPDATA\Programs\Common\VST3")) {
        New-Item -Path "$env:LOCALAPPDATA\Programs\Common\VST3" -ItemType Directory -Force | Out-Null
    }
    Copy-Item -Path $sourceDir -Destination "$env:LOCALAPPDATA\Programs\Common\VST3" -Recurse -Force
    "Copied bundle to $userVst3Dir" | Out-File $logFile -Append

    # 7. Invalidate Cubase VST3 cache so Cubase performs a full rescan
    if (Test-Path $cubaseCacheFile) {
        [xml]$xml = Get-Content $cubaseCacheFile
        $nodesToRemove = @($xml.plugins.plugin | Where-Object { $_.path -like "*StrataMaster*" })
        if ($nodesToRemove.Count -gt 0) {
            foreach ($node in $nodesToRemove) {
                [void]$node.ParentNode.RemoveChild($node)
            }
            $xml.Save($cubaseCacheFile)
            Write-Host "Cleared stale StrataMaster entry from Cubase VST3 cache." -ForegroundColor Green
            "Cleared stale cache entry from $cubaseCacheFile" | Out-File $logFile -Append
        }
    }

    # 8. Verify installed binary
    $targetDll = Join-Path $systemVst3Dir "Contents\x86_64-win\StrataMaster.vst3"
    $targetHash = (Get-FileHash -Algorithm SHA256 $targetDll).Hash
    $targetLength = (Get-Item $targetDll).Length

    if ($targetHash -eq $sourceHash) {
        Write-Host "SUCCESS: System VST3 successfully verified!" -ForegroundColor Green
        Write-Host "Length: $targetLength bytes" -ForegroundColor Green
        Write-Host "Hash: $targetHash" -ForegroundColor Green
        "VERIFIED: Installed system binary matches source ($targetLength bytes, $targetHash)" | Out-File $logFile -Append
    } else {
        throw "Hash mismatch! Source: $sourceHash, Target: $targetHash"
    }

    "=== Deployment Completed Successfully at $(Get-Date) ===" | Out-File $logFile -Append
} catch {
    Write-Host "ERROR: $($_.Exception.Message)" -ForegroundColor Red
    "ERROR: $($_.Exception.Message)" | Out-File $logFile -Append
    exit 1
}
