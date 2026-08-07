# Determining what is causing crashes
- Espressif bluetooth binary blob might have bugs
    - ```btdm_controller_task``` says its running on the wrong core?
    - Random hangup which causes core 0 which has the bluetooth scanner session locks up triggering watchdog timer
- Collision between stack and heap caused by something going on with how embassy allocates the stack
    - Heap is a fixed size and shouldn't be the problem
    - Stack is somehow growing even though these are all compiled ahead of time into a state machine into rust futures for embassy?
    - Something else going on resulting in heap/stack corruption which triggers the stack guard

