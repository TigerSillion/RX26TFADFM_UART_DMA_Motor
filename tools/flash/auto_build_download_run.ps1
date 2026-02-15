param(
    [string]$ProjectRoot = ".",
    [string]$BuildDir = "HardwareDebug",
    [string]$MakeExe = "C:\Users\a5140073\.eclipse\com.renesas.platform_1025384504\Utilities\make.exe",
    [string]$CcrxBinDir = "C:\Program Files (x86)\Renesas\RX\3_7_0\bin",
    [string]$CcrxUtilDir = "C:\Users\a5140073\.eclipse\com.renesas.platform_1025384504\Utilities\ccrx",
    [string]$GdbServerExe = "C:\Users\a5140073\.eclipse\com.renesas.platform_1025384504\DebugComp\RX\e2-server-gdb.exe",
    [string]$GdbExe = "C:\Users\a5140073\.eclipse\com.renesas.platform_1025384504\DebugComp\RX\rx-elf-gdb.exe",
    [string]$Target = "R5F526TF",
    [string]$Probe = "E2LITE",
    [int]$PtimerClock = 120000000,
    [int]$UseFine = 1,
    [string]$FineBaudRate = "1.50",
    [int]$ClockSrcHoco = 1,
    [int]$ConnectionTimeoutSec = 30,
    [int]$ServerWaitSec = 15
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

function Write-Step([string]$msg) {
    $ts = Get-Date -Format "yyyy-MM-dd HH:mm:ss"
    Write-Host "[$ts] $msg"
}

$root = (Resolve-Path $ProjectRoot).Path
$buildPath = Join-Path $root $BuildDir
$elfPath = Join-Path $buildPath "RX26TFADFM_UART_DMA_Motor.x"
$logDir = Join-Path $buildPath "logs"
$serverOutLog = Join-Path $logDir "gdb_server_out.log"
$serverErrLog = Join-Path $logDir "gdb_server_err.log"
$gdbCliLog = Join-Path $logDir "gdb_client.log"
$gdbCmds = Join-Path $buildPath "auto_gdb_cmds.txt"

if (-not (Test-Path $buildPath)) {
    throw "Build directory not found: $buildPath"
}

if (-not (Test-Path $MakeExe)) { throw "make.exe not found: $MakeExe" }
if (-not (Test-Path $GdbServerExe)) { throw "e2-server-gdb.exe not found: $GdbServerExe" }
if (-not (Test-Path $GdbExe)) { throw "rx-elf-gdb.exe not found: $GdbExe" }

New-Item -ItemType Directory -Force -Path $logDir | Out-Null

$env:Path = "$CcrxBinDir;$CcrxUtilDir;$env:Path"

Write-Step "Build start: $buildPath"
Push-Location $buildPath
try {
    & $MakeExe all
    if ($LASTEXITCODE -ne 0) {
        throw "Build failed with exit code $LASTEXITCODE"
    }
} finally {
    Pop-Location
}

if (-not (Test-Path $elfPath)) {
    throw "ELF not found after build: $elfPath"
}
Write-Step "Build success: $elfPath"

@"
set pagination off
set confirm off
set remotetimeout 30
target extended-remote :55023
monitor reset
load
monitor reset
continue
quit
"@ | Set-Content -Path $gdbCmds -Encoding ascii

if (Test-Path $serverOutLog) { Remove-Item $serverOutLog -Force }
if (Test-Path $serverErrLog) { Remove-Item $serverErrLog -Force }
if (Test-Path $gdbCliLog) { Remove-Item $gdbCliLog -Force }

$serverArgs = @(
    "-g", $Probe,
    "-t", $Target,
    "-uConnectionTimeout=$ConnectionTimeoutSec",
    "-uClockSrcHoco=$ClockSrcHoco",
    "-uPTimerClock=$PtimerClock",
    "-uAllowClockSourceInternal=1",
    "-uUseFine=$UseFine",
    "-uFineBaudRate=$FineBaudRate",
    "-w", "0",
    "-z", "0",
    "-uRegisterSetting=0",
    "-uModePin=0",
    "-uChangeStartupBank=0",
    "-uStartupBank=0",
    "-uDebugMode=0",
    "-uExecuteProgram=0",
    "-uIdCode=FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF",
    "-uresetOnReload=1",
    "-n", "0",
    "-uWorkRamAddress=1000",
    "-uverifyOnWritingMemory=0",
    "-uProgReWriteIRom=0",
    "-uProgReWriteDFlash=0",
    "-uhookWorkRamAddr=0xfdd0",
    "-uhookWorkRamSize=0x230",
    "-uOSRestriction=0",
    "-l",
    "-uCore=SINGLE_CORE|enabled|1|main",
    "-uSyncMode=async",
    "-uFirstGDB=main",
    "--english",
    "--gdbVersion=16.2"
)

Write-Step "Starting GDB server..."
$server = Start-Process -FilePath $GdbServerExe -ArgumentList $serverArgs -PassThru -RedirectStandardOutput $serverOutLog -RedirectStandardError $serverErrLog

try {
    $connected = $false
    for ($i = 0; $i -lt $ServerWaitSec; $i++) {
        Start-Sleep -Seconds 1
        $out = if (Test-Path $serverOutLog) { Get-Content $serverOutLog -Raw } else { "" }
        if ($out -match "Target connection status - OK" -or $out -match "Finished target connection") {
            $connected = $true
            break
        }
        if ($out -match "has failed with error report") {
            break
        }
        if ($server.HasExited) { break }
    }

    if (-not $connected) {
        $outTail = if (Test-Path $serverOutLog) { Get-Content $serverOutLog -Tail 60 } else { @() }
        $errTail = if (Test-Path $serverErrLog) { Get-Content $serverErrLog -Tail 60 } else { @() }
        throw ("GDB server failed to connect.`nOUT:`n" + ($outTail -join "`n") + "`nERR:`n" + ($errTail -join "`n"))
    }

    Write-Step "Server connected. Start GDB client download+run..."
    Push-Location $buildPath
    try {
        & $GdbExe -q -batch -x $gdbCmds $elfPath *>&1 | Tee-Object -FilePath $gdbCliLog
        if ($LASTEXITCODE -ne 0) {
            throw "GDB client failed with exit code $LASTEXITCODE. See $gdbCliLog"
        }
    } finally {
        Pop-Location
    }

    Write-Step "Download and run completed."
    Write-Step "Logs: $logDir"
}
finally {
    if ($server -and -not $server.HasExited) {
        Stop-Process -Id $server.Id -Force
    }
}
