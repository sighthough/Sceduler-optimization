#include <ntddk.h>

#define TARGET_WINDOW_100NS  10000LL // 1.0 ms in 100-nanosecond units
#define RAMP_LATENCY_100NS   150LL   // 15 us hardware ramp penalty

static PEX_TIMER g_SchedTimer = NULL;
static ULONG g_MaxFreqMhz = 0;
static ULONG g_ActiveFloorMhz = 0;
static ULONG g_EmaBudget100ns = 5000; // 0.5 ms initial seed
static ULONG g_LastSetFreq = 0;

typedef struct _PROCESSOR_PERFORMANCE_STATE_POLICY {
    ULONG Revision;
    ULONG MaxThrottle;
    ULONG MinThrottle;
    UCHAR BusyAdjThreshold;
    UCHAR Spare[3];
    ULONG TimeCheck;
} PROCESSOR_PERFORMANCE_STATE_POLICY, *PPROCESSOR_PERFORMANCE_STATE_POLICY;

// Clamps and sets frequency limit across active processor cores
VOID SetSystemCpuFrequencyLimit(ULONG TargetFreqMhz) {
    if (TargetFreqMhz == g_LastSetFreq) return; // State Caching Guard

    ULONG ClampedFreq = TargetFreqMhz;
    if (ClampedFreq < g_ActiveFloorMhz) ClampedFreq = g_ActiveFloorMhz;
    if (ClampedFreq > g_MaxFreqMhz) ClampedFreq = g_MaxFreqMhz;

    PROCESSOR_PERFORMANCE_STATE_POLICY Policy = {0};
    Policy.Revision = 1;
    Policy.MaxThrottle = (ClampedFreq * 100) / g_MaxFreqMhz; // Percentage limit
    Policy.MinThrottle = (g_ActiveFloorMhz * 100) / g_MaxFreqMhz;

    // Update Processor Power Management via NtPowerInformation/ZwPowerInformation
    ZwPowerInformation(ProcessorPowerPolicyControl, &Policy, sizeof(Policy), NULL, 0);
    g_LastSetFreq = ClampedFreq;
}

// Timer Callback operating at IRQL = DISPATCH_LEVEL
VOID SchedTimerCallback(_In_ PEX_TIMER Timer, _In_ PVOID Context) {
    UNREFERENCED_PARAMETER(Timer);
    UNREFERENCED_PARAMETER(Context);

    ULONG ObservedBudget100ns = 8000; // Mock 0.8 ms thread budget
    
    // EMA Calculation: (3 * New + 7 * Old) / 10
    g_EmaBudget100ns = ((3 * ObservedBudget100ns) + (7 * g_EmaBudget100ns)) / 10;
    ULONG AdjustedBudget = g_EmaBudget100ns + RAMP_LATENCY_100NS;

    ULONG TargetFreq;
    if (AdjustedBudget >= TARGET_WINDOW_100NS) {
        TargetFreq = g_MaxFreqMhz;
    } else {
        TargetFreq = (ULONG)(((ULONGLONG)g_MaxFreqMhz * AdjustedBudget) / TARGET_WINDOW_100NS);
    }

    SetSystemCpuFrequencyLimit(TargetFreq);
}

VOID DriverUnload(_In_ PDRIVER_OBJECT DriverObject) {
    UNREFERENCED_PARAMETER(DriverObject);

    if (g_SchedTimer) {
        ExCancelTimer(g_SchedTimer, NULL);
        ExDeleteTimer(g_SchedTimer, TRUE, TRUE, NULL);
    }
    
    // Reset to 100% Max Frequency on unload
    SetSystemCpuFrequencyLimit(g_MaxFreqMhz);
    DbgPrint("Tickless Scheduler Driver Unloaded.\n");
}

NTSTATUS DriverEntry(_In_ PDRIVER_OBJECT DriverObject, _In_ PUNICODE_STRING RegistryPath) {
    UNREFERENCED_PARAMETER(RegistryPath);
    DriverObject->DriverUnload = DriverUnload;

    // Hardcoded hardware base target (Hook to HalGetProcessorInformation or WMI in production)
    g_MaxFreqMhz = 4000; 
    g_ActiveFloorMhz = (g_MaxFreqMhz * 65) / 100; // 65% Floor

    // Allocate High-Resolution Kernel Timer
    EXT_TIMER_PARAMETERS TimerParams;
    ExInitializeExtTimerParameters(&TimerParams);
    g_SchedTimer = ExAllocateTimer(SchedTimerCallback, NULL, &TimerParams);

    if (!g_SchedTimer) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    // Set periodic timer (1.0 ms interval = -10,000 in 100ns units)
    LARGE_INTEGER DueTime;
    DueTime.QuadPart = -10000LL;
    ExSetTimer(g_SchedTimer, DueTime.QuadPart, 10000LL, NULL);

    DbgPrint("Tickless Scheduler Driver Loaded Successfully.\n");
    return STATUS_SUCCESS;
}