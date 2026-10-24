# Implementation sequence
1. Build, flash heartbeat, verify board clock and wiring.
2. TB6612 driver: zero output, separate wheel tests, verify STBY and polarities.
3. ADC/DMA complete-frame capture, sensor monitor and validated calibration.
4. Fixed-period PD control at low speed; log error, delta error and wheel PWM.
5. Acceleration ramp and stability-based speed target.
6. Bounded gap crossing, line recovery and timeout.
7. Junction detection and route memory, if needed.
Do not add blocking display transfers or waits inside the control interrupt.
