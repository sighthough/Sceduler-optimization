#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/hrtimer.h>
#include <linux/ktime.h>
#include <linux/cpufreq.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("SchedulerDev");
MODULE_DESCRIPTION("Tickless Event-Driven Scheduler with Dynamic DVFS");

#define TARGET_WINDOW_NS  1000000ULL // 1.0 ms in nanoseconds
#define RAMP_LATENCY_NS   15000ULL   // 15 us hardware ramp penalty
#define EVAL_INTERVAL_NS  1000000ULL // 1.0 ms evaluation cycle

static struct hrtimer sched_timer;
static u64 ema_budget_ns = 500000ULL; // Initialized to 0.5 ms
static unsigned int max_freq_khz = 0;
static unsigned int active_floor_khz = 0;

/* Calculates dynamic target frequency based on workload EMA */
static unsigned int calculate_target_freq(u64 observed_budget_ns) {
    u64 adjusted_budget;
    u32 target_freq;

    // Exponential Moving Average (Alpha = ~0.3 using integer arithmetic: (3*new + 7*old)/10)
    ema_budget_ns = ((3 * observed_budget_ns) + (7 * ema_budget_ns)) / 10;
    adjusted_budget = ema_budget_ns + RAMP_LATENCY_NS;

    if (adjusted_budget >= TARGET_WINDOW_NS) {
        return max_freq_khz;
    }

    target_freq = (u32)div_u64((u64)max_freq_khz * adjusted_budget, TARGET_WINDOW_NS);
    
    // Clamp between active floor (65%) and maximum boost clock
    if (target_freq < active_floor_khz) target_freq = active_floor_khz;
    if (target_freq > max_freq_khz) target_freq = max_freq_khz;

    return target_freq;
}

/* Tickless HRTimer Callback */
static enum hrtimer_restart sched_timer_callback(struct hrtimer *timer) {
    ktime_t now = ktime_get();
    struct cpufreq_policy *policy;
    
    // Simulated active workload budget (Hook real thread metrics here)
    u64 mock_observed_budget_ns = 800000ULL; // 0.8 ms
    
    unsigned int target_freq = calculate_target_freq(mock_observed_budget_ns);

    // Apply frequency constraint to CPU 0
    policy = cpufreq_cpu_get(0);
    if (policy) {
        if (policy->cur != target_freq) {
            __cpufreq_driver_target(policy, target_freq, CPUFREQ_RELATION_H);
        }
        cpufreq_cpu_put(policy);
    }

    hrtimer_forward_now(timer, ns_to_ktime(EVAL_INTERVAL_NS));
    return HRTIMER_RESTART;
}

static int __init scheduler_init(void) {
    struct cpufreq_policy *policy = cpufreq_cpu_get(0);
    if (!policy) return -ENODEV;

    max_freq_khz = policy->cpuinfo.max_freq;
    active_floor_khz = (max_freq_khz * 65) / 100; // 65% Active Floor
    cpufreq_cpu_put(policy);

    hrtimer_init(&sched_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
    sched_timer.function = &sched_timer_callback;
    hrtimer_start(&sched_timer, ns_to_ktime(EVAL_INTERVAL_NS), HRTIMER_MODE_REL);

    pr_info("Tickless Scheduler Loaded: Max = %u kHz, Floor = %u kHz\n", max_freq_khz, active_floor_khz);
    return 0;
}

static void __exit scheduler_exit(void) {
    hrtimer_cancel(&sched_timer);
    pr_info("Tickless Scheduler Unloaded.\n");
}

module_init(scheduler_init);
module_exit(scheduler_exit);