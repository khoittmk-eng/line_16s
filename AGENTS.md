# Repository working instructions

Before editing firmware, read `docs/README.md`, `docs/hardware.md`,
`docs/bringup.md`, and `docs/error-log.md`.
Review the error log for known causes and prevention checks before implementing related code.
When a new reproducible defect is found, record its symptom, evidence, root cause,
fix, validation, and status. Do not claim a hardware test that was not performed.
Keep documentation in Vietnamese with standard technical terms in English.
Do not modify vendor sources or the old reference project without a concrete need.
Keep STBY tied to 3V3 in the hardware model; no STBY GPIO exists.
Build using `scripts/build.ps1`. A build pass does not establish physical operation.
The application has the legacy menu, Motor Test and explicit N3 Start/Stop RUN. Read docs/menu.md and docs/run-test.md. The owner verified calibration/Motor Test and B02 tracking on circles/gentle curves at Base 20%; B03 corner/dash handling and higher duty still need hardware verification. Read docs/recovery.md before changing recovery timings.
Preserve calibration/fresh-frame start interlocks, fixed-period frame control, watchdog fault latching and explicit operator Start. Never auto-start on boot or after faults.

Owner reports B03 improves corners/gaps but cuts diagonally on repeated square turns. Sensor-to-axle offset is approximately 16 cm; inter-corner straight is 40 cm. Read docs/square-wave.md. B04 ADVANCE/ALIGN timings are trial parameters, not odometry; retain explicit calibration and watchdogs. Base/Max and corner timing are editable on OLED and currently RAM-only.

B05 supersedes B04 after owner-reported hardware regression. Read docs/b05-recovery.md. Never reintroduce geometry-based open-loop advance or mandatory white-clear/minimum-spin gates without new measurements. Valid single-line data must keep forward PD authority. Recovery uses symmetric counter-rotation and yields to valid line data; preserve LAST RUN diagnostics and all start/watchdog interlocks. Emulation is not hardware validation.

B06 current: read docs/high-speed.md. Owner reports Max 40% tracks, higher speed overshoots; trial gains 2/0/5 oscillate and fail corners. Defaults 12/0/0.12, Base 30/Max 60 are unverified trial values. Preserve sensor-based speed planning, no full brake on valid line/centered gaps, bounded BRAKE pulse only at confirmed high-duty recovery; valid line cancels brake immediately. BRAKE shares the recovery deadline. Explicit motor brake uses IN1=IN2=High, PWM=Low; STBY remains hardwired. PWM/speed variables and emulator MMIO tests do not establish measured speed/braking distance. Back up owner gain edits before changing defaults.

B07 current: read docs/speed-b07.md. Owner reports B06 gentle curves work; straight slow and 90-degree recovery slow. Preserve PD gains/15 ms D filter and brake/gap policies. Planner-only 40 ms rate filtering, 0.65-pitch straight corridor and persistent risk gates avoid boost starvation for central-eye masks/jitter. A centered 0x3C group is not a corner by width alone. Do not restore the 0.25-pitch gate without quantization regression tests. Pivot target 24%/ramp 350 still yields immediately to valid line and respects Max. Preserve V/T/reason telemetry and LAST RUN. Actual speed/pivot performance needs hardware testing.

B08 current: read docs/speed-b08.md. Owner says braking works and requests a faster Base/curves/90-degree profile. Default Base/Max 40/75, acceleration 80, curve caps 32/28, pivot 30/ramp 450, RECENTER min(Base,26). Preserve brake driver/entry threshold/pulse/deceleration and line-loss policy; owner has not validated the new speed profile. Keep B07 planner, PD gains, telemetry and interlocks. Source equality checks confirm brake policy unchanged, not identical physical stopping distance at a higher duty.

B09 supersedes boost/planner instructions above at the owner's request: read docs/cruise-gap-b09.md. Owner reports dashed gaps/gentle curves hesitate and chooses fixed Base cruise over auto-boost. Default Base45/Max75; Max is only a wheel-output ceiling. Keep PD gains, startup ramp, smooth positional sharp-curve blend and brake/recovery hardware policy. GAP must hold exact recent trusted L/R commands (not lower them toward Base/PROBE15); central PD D-kicks must not alone reject a gap. Guide age/edge invalidation, 300 ms continuous-loss limit and 12 ms central split grace are mandatory bounds; no auto-restart after fault. Do not restore boost/cooldown gates without an explicit owner request. Passing nonzero-PWM tests does not establish mechanical motion; assert held commands and preserve watchdog tests.

B10 current: read docs/recovery-b10.md. Preserve B09 cruise/gap policy. Asymmetric invalid masks may supply a direction only after 6 ms consecutive same-side confirmation; full-width/symmetric noise must not invent direction. Valid line retains PD authority. Fault labels must take precedence over Start readiness. No physical validation claimed.

B11 current: read docs/recovery-b11.md. Owner confirms B10 isolated turns/dashes, reports consecutive-turn LINE LOST. Retarget confirmed split direction during recovery only after visible line in that episode, without renewing deadline. Seed fresh eligible guide on completed RECENTER. Preserve speed/PID/brake and all previous valid-line/watchdog bounds. Test physical response before raising Base.

B12 current: read docs/recovery-b12.md. User requests last-seen-side search. B12 supersedes unconditional GAP-timeout fault: forward GAP remains <=300ms, then fresh side memory may start bounded recovery (1200ms). Memory TTL600ms, trusted-center clear100ms; update only observed valid side or accepted split cue. Preserve no-auto-restart, watchdogs and valid-line authority; do not extend recovery indefinitely. B11 square-wave still fails per owner; no hardware validation claimed for B12.

B13 current: read docs/recovery-b13.md. Preserve post-turn memory200ms, release after trusted center100ms. Immediate reloss must not inherit straight GAP300ms; confirm6ms and retry observed/remembered side. Preserve recovery deadline through short exit/retry; valid-line PD remains authoritative. Genuine gaps directly after turns can trigger recovery: document this heuristic limitation. Defaults restored to released B12 profile12/0/0.12,45/75 after backing up owner edits. No hardware validation claimed.

B14 current: read docs/diagnosis-b14.md. Confirm valid broad edge-reaching asymmetric shapes for6ms, cap target28 while shape present without forcing pivot. Pending200ms, trusted center100ms clear; fresh loss retries after6ms instead of GAP. Preserve centered3C/gentle PD and watchdogs. CRN indicates heuristic, not proven90-degree corner. DIAG per accepted RUN survives Stop/fault; N1 cycles Live/Last Run/Diag. Counters are not a temporal trace. No measured ADC or vehicle dynamics claims from emulator.
