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
