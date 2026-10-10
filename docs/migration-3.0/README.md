# CSP4CMSIS 2.0.0 -> 3.0.0

All three projects pin the published pack `OliverFaust::CSP4CMSIS@3.0.0` (release v3.0.0, pack SHA-256
`824284b9…9d00ca`); the lock files were regenerated. Board runs: `results-3.0.0/`, one per project, from
reset; reference: `../migration-2.0/results-2.0.0/`. Built with CMSIS-Toolbox 2.14.0 (Alif SDK) and
Ubuntu's GCC 14.2.1, as for 2.0.0; captured 2026-10-07 on the DevKit-E8 (AE822FA0E5597LS0 rev A1).

## Changes

| Commit | Change |
|---|---|
| Pin | `OliverFaust::CSP4CMSIS@3.0.0` (was 2.0.0) in all three projects; lock files regenerated |
| `main.cpp` (alt_test, pack_test) | the FreeRTOS kernel hooks (idle and timer task memory, stack-overflow and malloc-failed hooks, the free-heap report) are conditional on `RTE_CMSIS_RTOS2_FreeRTOS`, the project's RTOS component, instead of CSP4CMSIS's backend define (which `main.cpp` no longer sees: it does not include CSP4CMSIS) |
| Defines | `CSP4CMSIS_STATIC_ALLOCATION` and `CSP4CMSIS_RTOS2_BACKEND_*` removed; only `CSP4CMSIS_MAX_SYSCALL_INTERRUPT_PRIORITY: 128` remains. Static allocation is the 3.0 default, and the backend is detected from `RTE_Components.h` (RTX5 for alt_test, Arm's FreeRTOS adapter for pack_test and neuropathway) |
| Receiver (alt_test, pack_test) | starts with `SleepFor(Milliseconds(10))` (was `osDelay(10)`, 10 ticks at 1 kHz) |
| Comments | `CSP_DEFAULT_NETWORK_PRIORITY` (was `CSP_LEGACY_PARALLEL_PRIORITY`), `Channel` only (`One2OneChannel` is removed in 3.0) |

No other source change was needed: the projects already used `Channel<T>`, `Alternative(in | v, ...)`
and `Run(InParallel(...), ExecutionMode::StaticNetwork)`.

**Removing the defines changes no code.** Built with and without them (same tree, same paths): every
function's instructions and every initialised data section are identical; only the addresses of the
linker's long-branch veneers differ (alt_test, pack_test; each build is reproducible). `neuropathway`'s
image is byte-identical.

## Builds

All six configurations (Debug and Release of each project) build with no warnings, except ExecuTorch's
deprecated `Method::get_inputs()` in `neuropathway/M55_HP/csp/inference_process.cpp` (unchanged code);
each compiles the seven CSP4CMSIS sources from the 3.0.0 pack.

## Board runs

| | 2.0.0 | 3.0.0 |
|---|---|---|
| `csp4cmsis_alt_test` (RTX5, Debug) | `SUCCESS: 20000 messages verified heap-free.`, no `DATA ERROR` | **same** |
| stacks MainApp free / Snd1 / Snd2 / Receiver | 15 552 / 488 / 488 / 640 B | 15 552 / 488 / 488 / **648** B |
| `csp4cmsis_pack_test` (FreeRTOS, Debug) | 19 progress lines, then `SUCCESS: 20000 …` partly overwritten (`[Receer 2] Finished.`, `000 messages...`) | 19 progress lines, then `[Sender 1] SUCCESS:.` (two runs, identical) |
| `Free heap before MainApp_Task creation` | 0 bytes | 0 bytes |
| stacks MainApp free / Snd1 / Snd2 / Receiver | 15 560 / 520 / 420 / 640 B | 15 560 / 520 / 420 / **656** B |
| `neuropathway` (FreeRTOS, Release) | windows in sequence, each handed from Sensor to Inference unchanged, ping-pong buffers alternating, LAYING while the board lies still | **same** (windows 0–32) |
| stacks CSP_Main free / Sensor / Inference / Console | 3 504 / 848 / 2 168 / 600–608 B | 3 504 / 848 / 2 168 / 600 B |

- **pack_test's last lines:** three threads print to one UART without a lock, so lines that are printed
  at the same moment interleave; this happened in 2.0.0 as well, at a slightly different place.
  `[Sender 1] Finished.` and `[Receiver] SUCCESS: 20000 messages verified heap-free.` merged; the
  `SUCCESS:` fragment shows the Receiver reached that `printf`, which it does only with no data error.
  In both versions `[Sender 1] Starting sequence.` is lost the same way.
- **Receiver stacks:** +8 B (alt_test) and +16 B (pack_test) of 4 KB; not examined (library code changed
  in 3.0, `SleepFor` instead of `osDelay`).
- The alt_test log starts with five stack reports of the run that was already going when the capture
  started; the run from reset follows.

## Not in the stability promise

`application.cpp` and `csp4cmsis_spn.cpp` give MainApp_Task (a plain thread, not a `CSProcess`) a static
stack and control block with `csp::internal::csp_stack_word_t` and `csp::internal::csp_static_thread_storage_t`.
They compile with 3.0.0, but `csp::internal` is not covered by 3.x's source-compatibility promise.

## neuropathway: one console owner (branch `reporter`, 2026-10-07)

Before, Sensor, Inference, Console, MainApp and ExecuTorch's log hook all printed with `printf`, and their
lines collided (`Senerence: model ready ...` in `results-3.0.0/neuropathway.txt`). Now the **Reporter** is
the only process that prints while the network runs. Sensor and Inference send fixed-size `report_t`
messages (`common_types.h`; trivially copyable) over one rendezvous `Channel<report_t>` with two writers:

- **Sensor** reports only `Started`/`Failed`; its window signature (the handoff check) travels in
  `window_t::sensor_sum`, so it never waits for the console while it samples the IMU.
- **Inference** reports `Started`/`Failed` and one `Result` per window (label, logits, Sensor's and its own
  sum; a negative code for a failed step). ExecuTorch's log hook no longer prints: it counts the messages
  and keeps the first one's level and source line (`RuntimeLog`, reported once); the count goes with each
  `Result`, and the Reporter prints it with the stack report.
- **MainApp** printed its line and ended before the network ran, and the Reporter printed the stack report
  (changed since: see the next section).

Board run (Release, from reset, `results-3.0.0/neuropathway_reporter.txt`; the first lines are the tail of
the run before the reset):

| | 3.0.0 before (`neuropathway.txt`) | Reporter |
|---|---|---|
| Lines per window | 4 (`[Sensor sig]`, `[Infer sig]`, the ExecuTorch log line, `[window]`), some collided | 1: `[window n] logits = [...] -> LABEL \| buf=b sum=s` |
| Windows | in sequence, one every 2.56 s | 0..25 in sequence, 4 per 10 s report, every handoff sum equal (no `MISMATCH`), LAYING (board lying still) |
| ExecuTorch log messages | one printed line per window | counted: first `level I, line 122` (model load), then one per window (`log messages so far: 5, 9, 13, ...`) |
| Stacks Sensor / Inference / Reporter (Console) | 848 / 2168 / (600) B | 648–696 / 2224 / 672 B |

Builds: Release and Debug, no warnings except ExecuTorch's (unchanged); ITCM 80.68 % (Release).

## neuropathway: stack report by MainApp (branch `mainapp-stack-monitor`, 2026-10-09)

As in the Himax neurochannel: the Reporter's own stack report depended on report traffic (no reports, no
stack report -- exactly when a process stalls). Now `MainApp_Task` keeps the network (`auto network =
InParallel(...)`), runs it as a StaticNetwork and stays alive: every 3000 ms (was 10 s in the Reporter) it
sends one `ReportKind::Stack` report per process (`network.forEachProcess`) and one for itself
(`osThreadGetStackSpace()`), over its own writer end of `report_chan` (now three writers). MainApp never
prints (its "network started" line is dropped); the Reporter only receives and prints, and adds the
ExecuTorch log count after MainApp's own line. A plain CMSIS-RTOS2 thread may use a rendezvous in CSP4CMSIS
3.0.0 (it blocks on thread flag 0 of the calling thread; MainApp uses no thread flags otherwise). MainApp
stays above the network (`osPriorityLow1`); time slicing is on.

Board run (Release, from reset, `results-3.0.0/neuropathway_mainapp_stack.txt`):

| | Reporter's stack report (before) | MainApp's stack report |
|---|---|---|
| Start-up | `*** MainApp_Task: network started ...`, ExecuTorch's first log message, `model ready`, `IMU streaming` | the same without MainApp's line |
| Windows | 0..25 in sequence, every handoff sum equal | 0..25 in sequence, every handoff sum equal (and 24..119 in sequence in an earlier 300 s capture of the 4 KB build) |
| Stack report | every 10 s, after a report | every 3 s, from MainApp: complete groups (Sensor, Inference, Reporter, CSP_Main, then the ExecuTorch count) |
| Stacks Sensor / Inference / Reporter / CSP_Main | 648–696 / 2224 / 672 B / -- (MainApp ended) | 632–688 / 2224 / 664 / 480 of 1024 B (MainApp stack right-sized from 4096 to 1024 B) |

Builds: Release and Debug, no warnings except ExecuTorch's (unchanged).

Observation, not caused by this change: windows and stack reports keep the designed ratio (1.19 windows per
3 s stack report, i.e. 2.53 s per window in board time), but in a 300 s capture only 81 stack reports
arrived, i.e. 243 s of board time: the board's 1 kHz tick appears to run about 19 % slow against the host
clock. Not investigated.
