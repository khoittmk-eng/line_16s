# Repository working instructions

B29 current (2026-10-29): read docs/corner-return-b29.md. Owner reports B28
16.3s atBase45 and55/60 previously slower. Keep45/85, base gains12/0/0.12.
Extra P rises0.05/ms instead of0.04, same0.6 ceiling. Fresh valid same-side
confirmed-cue error decreasing by>0.01pitch/frame for6ms latches releasing;
fade extra toward0 at0.05/ms while filtered planner still outward. Plateau
keeps release;6ms clear outward movement rearms eligibility; cue/mode changes,
invalid, Stop/recovery reset trend. Never clear corner memory or alter deadlines
from this trend. Preserve B28 soft PID,15msD/40msrate, search50/36, arc28ms,
dash/driver/watchdogs/calibration and Flash48/108 schema. Baseline fail140035;
104149ARM+26Python tests and three-candidate command comparison pass; Keil0
errors/warnings. No B29 hardware/lap validation. BackupB28 source/header/HEX.

B28 current (2026-10-29): read docs/visible-corner-b28.md. Owner reports B27
Base45 lap17.30s, dash improved, requests earlier corner steering/multiple
approaches. Keep45/85 and base gains12/0/0.12. TRACK confirmed directional
shape6ms + same-side error>=1pitch + nonimproving planner rate schedules extra
P toward0.6*clamp(abs(e)-1,0,1), rise0.04/ms. Reset assist when ineligible,
invalid/Stop/recovery; no abrupt B24 floor or Kd switch. Keep output clipping
and antiwindup using effective P. Search pivot50 only when mask0; nonzero
invalid mask36; valid always immediate forward PD, rejoin cap40 preserved.
Keep B27 arc/dash/driver and all deadlines/interlocks. Three ARM command-only
candidates compared: B27, abrupt gain, gradual gain; gradual chosen for smaller
entry step, not measured lap improvement. Baseline fail130035;104022ARM+26Python
tests pass, Keil0errors/warnings, not hardware-tested. Extra gain is not a proven
90deg classifier. CSV logs base gains; format48/108 unchanged. BackupB27.

B27 current (2026-10-29): read docs/rolling-corner-b27.md. Owner reports B26
improvement at45, accepts more body overshoot for faster corners. Keep45/85,
12/0/0.12, fixed Base/valid forward PID. On confirmed loss from forward mean>=35,
TurnOutput uses entry common with0.8 steering for first28ms of the existing
recovery episode (clamp byMax/1.8), then pivot36 fallback. Any valid frame yields
immediately; successful arc rejoin usesBase instead of cap40. Post-turn retry
cannot renew arc/deadline; slow/reverse entry keeps pivot. Preserve1200ms/GAP
bounds and watchdogs. Gap-only correction filter0.04 per ms smooths stored dash
direction, not TRACK PID or ambiguity guide; keep clipping/deadband. Driver
stagger both reversals only for pivot retarget, capped to new forward request;
each reversing channel still gets zero, explicitStop remains immediate. B26
baselines fail120032/controller and50056/driver.102331ARM+26Python tests pass;
no hardware/lap validation. CSV/Flash layout unchanged. BackupB26.

B26 current (2026-10-29), superseding conflicting older profiles below: read
docs/corner-flow-b26.md and docs/keil-download.md. Owner reports Base45 more
stable than55 and requests quicker forward corner following. Defaults45/85,
gains12/0/0.12; fixed Base TRACK, no boost. Remove whole-correction slew, retain
15ms D filter/antiwindup. RECENTER preserves current command magnitude under
min(Base,40), post-turn cap40, existing center/deadline/memory bounds remain.
Valid line has immediate forward PD; signed pivot only after confirmed loss.
Do not impose an inner-wheel floor or change coast/brake/estimator without
evidence. Preserve55/85 regression preset alongside new45/85 tests. Board helper
writes known SWD-enabled SWJ_CFG with OLED remap, masking write-only readback.
No Save was pressed when owner saw download failure; no NRST wire exists yet.
ERR-043 Sleep explanation is unconfirmed, clarified by ERR-045; never claim
physical download fixed from emulator tests. No option-byte/full erase actions.
101895 ARM assertions+26 Python tests, Keil0errors/0warnings; not hardware-tested.
Backup B25 source/HEX; read error log before related edits.

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

B15: explicit owner authorization supersedes B09 fixed-cruise restriction. Read docs/speed-b15.md. Owner confirms B14 45/75 runs whole map. Preserve bounded bonus, central quantization tolerance, soft-curve interpolation, 300ms corner bonus guard, exact short GAP60ms then proportional excess-only reduction toward Base. Short centered reacquisition must retain earned duty. Preserve B14 recovery/brake/watchdogs. B15 is not hardware-verified.

B16: owner requests defaults50/80 only, retaining B15 controller/gains/absolute bonus ceiling55. Read docs/profile-b16.md. B15 owner reports ~0.5s improvement, no repeated-run validation. B16 source/emulator tested, not hardware-verified. Preserve baseline45/75 regressions and separate new-default tests.

B17 current: read docs/speed-b17.md. Defaults55/85, stable curve blend2..3/tight32, separate filtered outward risk; pivot36/ramp600/recenter30. Owner reports whole-map stability and ~1s faster, but soft curves hesitate and repeated squares remain slow;60/90 brakes more and is slower. Do not infer full-brake cause without trace. Preserve valid PD authority, GAP/memory/watchdog/deadline policies. Test TURN ramp from nonzero forward commands as well as brake-zero entry. 34734 assertions pass. Next improvement requires separating reduced PWM from PROBE/recovery dwell; no new firmware changes authorized by discussion alone.

B18 current: owner explicitly requests continued optimization. Read docs/speed-b18.md. Keep55/85 and B17 gains/pivot/brake. Separate curve snapshot: moderate valid no edge/pending/post-turn/hint, rate<=4, error<=2; exact bridge fragmented8ms/white4ms, age<=10ms. Never renew loss counters or invalid centroid. Post-turn central rejoin acceleration160 only below42, then80. TIME diagnostics command-frame durations are not measured mechanical brake time; CV subset TRACK, PROBE includes bridge. Preserve four-page stopped N1 cycle and diagnostics freeze/reset. B18 not hardware-verified.

B19 current: owner requests OLED Straight parameter. Read docs/speed-b19.md. Defaults Base55/Straight60/Max85. Straight replaces hard cap55/bonus10/headroom15; clamp between Base/Max, central corridor0.8/no outerC3/80ms and guards before bonus. Keep B18 curve/recovery/bridge/brake/watchdogs. Six PID rows at y10+7*i, speeds step5 and RAM-only. Tests explicitly set Straight for old profiles; no assumption that Base/Max alone disables boost. Owner confirms B18 whole map stable; B19 not hardware-verified.

B20 current: explicit owner authorization for proactive corner braking supersedes the old blanket ban on braking while valid. Read docs/speed-b20.md. Defaults55/60/85, gains12/0/0.12 confirmed by owner. ENTRY_BRAKE is separate from recovery: confirmed valid broad shape6ms + recent fast center150ms + mean commanded duty>=50 + latch/no-post-turn; bounded8ms, ordinary valid cancels, invalid transfers only remainder into the existing recovery budget. No timed pivot on valid line. Soft steering build-up slew800 only moderate/no-corner/no-outward conditions; release correction immediately, reverse from zero, edge keeps full PD. Post-turn cap35 until trusted-center60ms (existing TTL200ms applies). ZERO/ENTRY diagnostics survive Stop; five N1 stopped pages. Never claim line identity guaranteed from a single sensor row. Preserve B18 bridge/GAP/watchdogs. 34861 assertions; test suite instruction budget300M with60s timeout is not hardware WCET. B20 not hardware-verified.

B21 current: explicit owner request supersedes B20 valid-line ENTRY_BRAKE. Read docs/speed-b21.md. Remove entry pulse/history/latch; valid broad corners keep forward PD and cap32. Soft deceleration120 only when existing moderate smooth gate holds and target>=Base; edge/outward/corner/recovery keep600. Gains/default55/60/85 unchanged. ENTRY enum/diagnostics retained as reserved, never emitted. Preserve bounded loss brake12, repeated-square memory, post-turn cap, GAP/bridge/watchdogs. 37251 assertions; Keil0 errors/warnings; not hardware-verified. Do not label PWM easing as a measured parabolic trajectory.

B22 current: owner authorizes black-box telemetry and persistent export via CH340. Read docs/blackbox-b22.md. Preserve B21 control.c/control_tuning.h; do not infer physical speed or brake force from PWM requests. Record a bounded 160x48-byte ring from control frames (4ms + event/mode bypass), freeze on first LOSS/CORNER window or Stop. No Flash/CSV/UART work in control ISR. Only stopped Robot_Log APIs may Save/Export; motor brake, ADC pause/resume preserving pending/sticky capture failures, require a fresh post-maintenance frame before Start. Reserve final16KiB Flash in linker/HEX checker; two8KiB slots, CRC32 and commit-last, latest valid generation on boot. PB12 Software UART TX9600/8N1 -> CH340 RXD; GND common, no RX or power pin required. System N2 BLACK BOX; keep six main items. ARM discards RAM only; never silently overwrite frozen RAM or export old Flash while new RAM awaits Save. Keep CSV CRC/schema/parser and record layout static asserts. Test actual log drivers with fake Flash/clock, never claim that proves physical persistence/baud. Flash write stalls and UART mask IRQ only while stopped. Export before reflash/full erase; B21 backups are local. Need hardware timing, power-cycle and serial validation.

B23 current: explicit owner authorization supersedes B22's preserve-controller-only restriction. Read docs/speed-b23.md and hardware log analysis seq2 CRC B4544906. Bounded ambiguity continuation: latch once per loss after trusted center40ms/guide age<=10ms/error<=0.8/rate<=4, both guide commands positive, current central pair present, no entry SplitDirection/no guard/corner/recovery/post-turn. PROBE <=32ms, cap min(Base,32,current duty), preserve trusted guide ratio, high duty decelerates600, never increase on invalid or use its centroid. Confirmed split/pending retains priority; white/anchor loss/expiry cancels permanently for this episode. Keep lost/ambiguous/deadlines, GAP and square-wave policy. Replay raw fixture approximates missing2ms frames; do not claim physical dynamics or lap improvement. Gains/profile55/60/85 unchanged. Flagbit4 marks continuation; record48/header108/CSVv1/Flash layout unchanged. 86667 ARM assertions +8 parser/layout +18 receiver tests; Keil0errors/warnings, not hardware-verified. Backup B22. Keep emulator300M/60s and complete test_done.

B24 current: owner explicitly requests removing brake-before-turn and earlier steering; supersedes B06/B23 policies preserving recovery brake/valid-shape cap. Read docs/speed-b24.md and seq3 analysis. BeginRecovery always TURN, no BRAKE pulse; BRAKE/ENTRY enums only legacy log compatibility. Pivot ramps common->0 at600 and steering->signed min(Max,36) at1800, bounds safe even when Max changes. Valid confirmed directional shape adds0.8*steering_limit floor only in TRACK with e*shape_dir>=1 and e*planner_rate>=0; release on improving/cleared cue. Remove shape-only target32; keep CruiseTarget edge/outward risk, PD/gains/default55/60/85, bounded ambiguity/GAP, post-turnmemory/deadlines/watchdogs. Motor reversal zero is per-channel; both channels may zero if both reverse. ExplicitStop/fault/maintenance brake stays. CORNER triggers directional valid/pending/TURN or fault, not directionlessPROBE; LOSS unchanged. Owner's seq3 CRC3ED4E91F shows B23 startup31.1% PROBE withoutBRAKE, not evidence of high-speed corner braking. 92268ARMassertions+26Pythontests; Keil0errors/warnings, B24notphysicalvalidated. BackupB23; export logs beforeflash. No geometryadvance/minspin/forcedpivot onvalid or invented direction onFF.

B25 current: owner reports B24 hardware regression and explicitly requests fixed high Base without straight boost. Read docs/fixed-base-b25.md. Supersedes B15-B24 boost/curve-risk target scheduling and B24 shape floor/common-ramped pivot: TRACK target=Base; ignore legacy straight_percent, menu5 rows with compatibility field synced toBase. Keep12/0/0.12, Base55/Max85; startup ramp remains. Confirmed pending loss entersTURN immediately unless a new opposite split needs its6ms confirmation; direct symmetric min(Max,36) request, per-wheel motor reversal guard unchanged. Weak central +/-0.5 must not overwrite pending/ALIGN/post-turn direction; strong>=1.2 or confirmed split can retarget. Preserve ALIGN30/postturn35, GAP/ambiguity bounds, deadlines, calibration/watchdogs/noautoStart. BaselineB24 repro90048, B25 97520ARM+26Python tests, Keil0errors/warnings; nohardwarevalidation. No geometryADVANCE/minspin. BackupB24; update error log for new issues.
