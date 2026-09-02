# Sceduler-optimization
this is a sceduler optimization so that it runs smooth while still saving power (improves stability on 90-95-99 percent lows too in 3d)

*Co-authored by [sighthough](https://youtu.be/UtPiUGwu-0Q) and Gemini 3.6*

In this repository you will find the code for the sceduler to work for windows and linux kernels
(warning! vibe coded and untested!)
but also a .ps1 file you can test on a windows pc with powershell or the updated terminal that is powershell anyway
(notion! vibe coded and tested ! )
to test the .ps1 file what you need to do is run a powershell or terminal as administator
then paste  

Set-ExecutionPolicy -ExecutionPolicy RemoteSigned -Scope CurrentUser

hit enter and then find the sceduler.ps1 , it should be in your downloads by default
with a lil cd magic you get there and then to run the script it should be like 

.\sceduler.ps1 (example C:\folder\.\sceduler.ps1)

and it will run sceduling everything so you can test it out yourselves , tho know its performance will probably increase once the big boys implement it



```text
+-------------------------------------------------------+
|             SCHEDULER LOOP (Every 250ms)              |
+-------------------------------------------------------+
                            |
                            v
           +---------------------------------+
           |  Search Active 3D Processes     |
           | (SecondLife, Firestorm, etc.)   |
           +---------------------------------+
                            |
             +--------------+--------------+
             |                             |
      [Process Found]              [No Process Found]
             |                             |
             v                             v
+--------------------------+  +--------------------------+
| Measure Delta CPU Time   |  | Set Clock Target = 0     |
| Across Cores             |  +------------+-------------+
+------------+-------------+               |
             |                             v
             v                +--------------------------+
+--------------------------+  | Restore Windows Defaults |
| Calculate Load Ratio     |  +------------+-------------+
+------------+-------------+               |
             |                             v
             v                +--------------------------+
+--------------------------+  | Sleep 2.0s & Retry       |
| Scaled Budget (0-5ms)    |  +--------------------------+
+------------+-------------+
             |
             v
+-------------------------------------------------------+
| Digital Shock Absorber (EMA Filter)                   |
| Budget = (0.3 * Observed) + (0.7 * Previous)          |
+---------------------------+---------------------------+
                            |
                            v
+-------------------------------------------------------+
| Hardware Ramp Compensation                            |
| Adjusted = Budget + 0.015ms                           |
+---------------------------+---------------------------+
                            |
                            v
           +---------------------------------+
           | Is Adjusted_Budget >= 1.0ms?     |
           +----------------+----------------+
                            |
             +--------------+--------------+
             |                             |
           [YES]                          [NO]
             |                             |
             v                             v
+--------------------------+  +--------------------------+
| Target = MAX_BOOST       |  | Target = Proportional    |
| (100% Frequency)         |  | Target = MAX * (B / 1.0) |
+------------+-------------+  +------------+-------------+
             |                             |
             +--------------+--------------+
                            |
                            v
+-------------------------------------------------------+
| Active Clock Floor Guard                              |
| Clamped = MAX(65% Max Frequency, Target)              |
+---------------------------+---------------------------+
                            |
                            v
           +---------------------------------+
           | Has Clamped_Target Changed?     |
           +----------------+----------------+
                            |
             +--------------+--------------+
             |                             |
           [YES]                          [NO]
             |                             |
             v                             v
+--------------------------+  +--------------------------+
| Execute System Call:     |  | Bypass PowerCFG          |
| powercfg /setacvalueindex|  | (Prevents Overhead)      |
| powercfg /setactive      |  +------------+-------------+
+------------+-------------+               |
             |                             |
             +--------------+--------------+
                            |
                            v
+-------------------------------------------------------+
| Output Log Feedback & Sleep 250ms                     |
+---------------------------+---------------------------+
                            |
                            v
                 [Loop Back to Top]


=========================================================
              INTERRUPT HANDLER (Ctrl + C)
=========================================================
                            |
                            v
+-------------------------------------------------------+
| Trap Termination Signal                               |
+---------------------------+---------------------------+
                            |
                            v
+-------------------------------------------------------+
| Reset Power Limits to Default (Max Throttle = 100%)   |
+---------------------------+---------------------------+
                            |
                            v
+-------------------------------------------------------+
| Exit Cleanly & Output Success                         |
+-------------------------------------------------------+
```
