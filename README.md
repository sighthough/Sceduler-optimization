# Sceduler-optimization
this is a sceduler optimization so that it runs smooth while still saving power (improves stability on 90-95-99 percent lows too in 3d)

*Co-authored by [sighthough](https://youtu.be/UtPiUGwu-0Q) and Gemini 3.6*

In this repository you will find the code for the sceduler to work for windows and linux kernels but also a .ps1 file you can
test on a windows pc with powershell or the updated terminal that is powershell anyway
to test the .ps1 file what you need to do is run a powershell or terminal as administator
then paste  

Set-ExecutionPolicy -ExecutionPolicy RemoteSigned -Scope CurrentUser

hit enter and then find the sceduler.ps1 , it should be in your downloads by default
with a lil cd magic you get there and then to run the script it should be like 

.\sceduler.ps1 (example C:\folder\.\sceduler.ps1)

and it will run sceduling everything so you can test it out yourselves , tho know its performance will probably increase once the big boys implement it

(Ctrl + C)

+-----------------------------------------------------------------------------------+
|                            SCHEDULER LOOP (Every 250ms)                            |
+-----------------------------------------------------------------------------------+
                                          |
                                          v
                         +---------------------------------+
                         |  Search Active 3D Processes     |
                         | (SecondLife, Firestorm, etc.)   |
                         +---------------------------------+
                                          |
                        +-----------------+-----------------+
                        |                                   |
                [Process Found]                   [No Process Found]
                        |                                   |
                        v                                   v
        +-------------------------------+   +-------------------------------+
        | Measure Delta CPU Time        |   | Set Clock Target = 0 (Idle)   |
        | Over Interval Across Cores    |   +---------------+---------------+
        +---------------+---------------+                   |
                        |                                   v
                        v                   +-------------------------------+
        +-------------------------------+   | Restore Windows Power Default |
        | Calculate Normalized Load     |   +---------------+---------------+
        | Load = CPU_Time / Elapsed_Time|                   |
        +---------------+---------------+                   v
                        |                   +-------------------------------+
                        v                   | Sleep 2.0s & Retry Search     |
        | Scaled Workload Budget (0-5ms)|   +-------------------------------+
        +---------------+---------------+
                        |
                        v
        +-------------------------------------------------------------------+
        | Digital Shock Absorber (EMA Filter)                               |
        | Budget_New = (0.3 * Budget_Observed) + (0.7 * Budget_Previous)    |
        +-------------------------------+-----------------------------------+
                                        |
                                        v
        +-------------------------------------------------------------------+
        | Hardware Ramp Compensation                                        |
        | Adjusted_Budget = Budget_New + 0.015ms (15us Ramp Latency)        |
        +-------------------------------+-----------------------------------+
                                        |
                                        v
                       +---------------------------------+
                       | Target Evaluation               |
                       | Is Adjusted_Budget >= 1.0ms?    |
                       +----------------+----------------+
                                        |
                    +-------------------+-------------------+
                    |                                       |
                [YES]                                      [NO]
                    |                                       |
                    v                                       v
    +-------------------------------+       +-------------------------------+
    | Target = MAX_BOOST (100%)     |       | Target = Proportional Speed   |
    +---------------+---------------+       | Target = MAX * (Budget / 1.0) |
                    |                       +---------------+---------------+
                    +-------------------+-------------------+
                                        |
                                        v
        +-------------------------------------------------------------------+
        | Active Clock Floor Guard                                          |
        | Clamped_Target = MAX(65% Max Frequency, Target)                   |
        +-------------------------------+-----------------------------------+
                                        |
                                        v
                       +---------------------------------+
                       | State Caching Check             |
                       | Has Clamped_Target Changed?     |
                       +----------------+----------------+
                                        |
                    +-------------------+-------------------+
                    |                                       |
                [YES]                                      [NO]
                    |                                       |
                    v                                       v
    +-------------------------------+       +-------------------------------+
    | Execute System Call:          |       | Bypass PowerCFG System Call   |
    | powercfg /setacvalueindex     |       | (Prevents Overhead / Stalls)  |
    | powercfg /setactive           |       +---------------+---------------+
    +---------------+---------------+                       |
                    +-------------------+-------------------+
                                        |
                                        v
                        +-------------------------------+
                        | Output Log Feedback           |
                        +---------------+---------------+
                                        |
                                        v
                        +-------------------------------+
                        | Loop Repeat (Sleep 250ms)     |
                        +-------------------------------+

=====================================================================================
                             INTERRUPT HANDLER (Ctrl + C)
=====================================================================================
                                        |
                                        v
                        +-------------------------------+
                        | Trap Termination Signal       |
                        +---------------+---------------+
                                        |
                                        v
                        +-------------------------------+
                        | Reset Power Limits to Default |
                        | (Set Max Throttle = 100%)     |
                        +---------------+---------------+
                                        |
                                        v
                        +-------------------------------+
                        | Exit Cleanly & Output Success |
                        +-------------------------------+
