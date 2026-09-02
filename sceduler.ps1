# Requires -RunAsAdministrator
<#
.SYNOPSIS
    Real-Time Dynamic DVFS & Workload Sequencing Engine for Second Life / 3D Viewers.
.DESCRIPTION
    Monitors live viewer processes, applies EMA workload estimation, maintains an active 
    frequency floor, and eliminates Windows PPM frequency flapping.
#>

# -----------------------------------------------------------------------------
# 1. INITIALIZATION & HARDWARE DETECTION
# -----------------------------------------------------------------------------
# Unhide Processor Maximum Frequency GUID in Windows Power Options
powercfg -attributes SUB_PROCESSOR 54533251-075e-4d8d-a73c-8264d569f980 -ATTRIB_HIDE | Out-Null

$CpuInfo = Get-CimInstance Win32_Processor
$MAX_FREQ_MHZ = [int]$CpuInfo.MaxClockSpeed
$ACTIVE_FLOOR_MHZ = [int]($MAX_FREQ_MHZ * 0.65) # 65% clock speed floor during gameplay
$TARGET_WINDOW_MS = 1.0                         # 1.0ms target budget window
$RAMP_LATENCY_MS = 0.015                        # 15us hardware voltage ramp penalty
$EMA_ALPHA = 0.3                                # Smooth factor for workload spikes

# Viewer process targets
$ViewerNames = @("SecondLife", "SecondLifeViewer", "Firestorm-bin", "Firestorm", "Alchemy", "Singularity")

Write-Host "=========================================================" -ForegroundColor Cyan
Write-Host "   Tickless Event-Driven Scheduler Engine for 3D Viewers " -ForegroundColor Cyan
Write-Host "=========================================================" -ForegroundColor Cyan
Write-Host "Max CPU Clock    : $MAX_FREQ_MHZ MHz" -ForegroundColor Gray
Write-Host "Active Clock Floor: $ACTIVE_FLOOR_MHZ MHz" -ForegroundColor Gray
Write-Host "Target Window    : $TARGET_WINDOW_MS ms" -ForegroundColor Gray
Write-Host "Press CTRL + C to STOP and automatically restore default power state.`n" -ForegroundColor Yellow

# State Tracking Variables
$Script:LastSetFreq = -1
$Script:EmaBudgetMs = 0.5 # Initial neutral seed value (0.5ms)

# Helper: Set CPU Max Frequency Limit via PowerCFG
function Set-CpuFrequencyLimit {
    param ([int]$TargetFreqMhz)
    
    # Return to OS Defaults if 0
    if ($TargetFreqMhz -eq 0) {
        if ($Script:LastSetFreq -ne 0) {
            powercfg /setacvalueindex SCHEME_CURRENT SUB_PROCESSOR 54533251-075e-4d8d-a73c-8264d569f980 0 | Out-Null
            powercfg /setactive SCHEME_CURRENT | Out-Null
            $Script:LastSetFreq = 0
            Write-Host "[PPM] Restored default Windows Power Plan settings." -ForegroundColor Green
        }
        return
    }

    # Clamp frequency between Active Floor and Max Hardware Boost
    $ClampedFreq = [Math]::Max($ACTIVE_FLOOR_MHZ, [Math]::Min($MAX_FREQ_MHZ, $TargetFreqMhz))
    
    # State Caching Guard: Only invoke system call if target frequency changed
    if ($ClampedFreq -ne $Script:LastSetFreq) {
        powercfg /setacvalueindex SCHEME_CURRENT SUB_PROCESSOR 54533251-075e-4d8d-a73c-8264d569f980 $ClampedFreq | Out-Null
        powercfg /setactive SCHEME_CURRENT | Out-Null
        $Script:LastSetFreq = $ClampedFreq
        Write-Host "[DVFS PPM] Set Max Clock Floor/Ceiling -> $ClampedFreq MHz" -ForegroundColor Yellow
    }
}

# -----------------------------------------------------------------------------
# 2. MAIN SCHEDULER MONITORING LOOP
# -----------------------------------------------------------------------------
try {
    $LoopIntervalMs = 250 # 250ms evaluation cycle
    
    while ($true) {
        # Search for active viewer process
        $ViewerProc = Get-Process -Name $ViewerNames -ErrorAction SilentlyContinue | Select-Object -First 1

        if ($null -ne $ViewerProc) {
            # Capture initial CPU sample
            $CpuTimeStart = $ViewerProc.TotalProcessorTime.TotalMilliseconds
            $TimeStart = [DateTime]::UtcNow
            
            Start-Sleep -Milliseconds $LoopIntervalMs
            
            # Refetch process metrics
            $ViewerProc.Refresh()
            $CpuTimeEnd = $ViewerProc.TotalProcessorTime.TotalMilliseconds
            $TimeEnd = [DateTime]::UtcNow
            
            # Calculate actual CPU usage during the interval across logical cores
            $ElapsedMs = ($TimeEnd - $TimeStart).TotalMilliseconds
            $CpuMsUsed = ($CpuTimeEnd - $CpuTimeStart)
            $CoreCount = [Environment]::ProcessorCount
            
            # Measure thread execution load normalized per core
            $NormalizedLoadRatio = ($CpuMsUsed / ($ElapsedMs * $CoreCount)) * $CoreCount
            $ObservedBudgetMs = [Math]::Min(5.0, $NormalizedLoadRatio * 2.0) # Scaled workload budget

            # Exponential Moving Average (EMA) Calculation
            $Script:EmaBudgetMs = ($EMA_ALPHA * $ObservedBudgetMs) + ((1 - $EMA_ALPHA) * $Script:EmaBudgetMs)
            $AdjustedBudgetMs = $Script:EmaBudgetMs + $RAMP_LATENCY_MS

            # Determine Required Clock Rate
            if ($AdjustedBudgetMs -ge $TARGET_WINDOW_MS) {
                $CalculatedFreq = $MAX_FREQ_MHZ
            } else {
                $CalculatedFreq = [int]($MAX_FREQ_MHZ * ($AdjustedBudgetMs / $TARGET_WINDOW_MS))
            }

            Set-CpuFrequencyLimit -TargetFreqMhz $CalculatedFreq

            # Debug Feedback
            Write-Host ("[WORKLOAD] Process: {0} | CPU Load Ratio: {1:P1} | EMA Budget: {2:F2} ms | Clock Target: {3} MHz" -f `
                $ViewerProc.ProcessName, $NormalizedLoadRatio, $Script:EmaBudgetMs, $Script:LastSetFreq) -ForegroundColor Gray

        } else {
            # Viewer process not running; maintain active default state
            Write-Host "[IDLE] Waiting for 3D Viewer process ($($ViewerNames -join ', '))..." -ForegroundColor DarkGray
            Set-CpuFrequencyLimit -TargetFreqMhz 0
            Start-Sleep -Seconds 2
        }
    }
}
finally {
    # -------------------------------------------------------------------------
    # 3. GUARANTEED CLEANUP ON EXIT
    # -------------------------------------------------------------------------
    Write-Host "`n[CLEANUP] Stopping Engine and resetting Windows Power Plan..." -ForegroundColor Green
    Set-CpuFrequencyLimit -TargetFreqMhz 0
    Write-Host "[SUCCESS] Windows defaults restored." -ForegroundColor Green
}