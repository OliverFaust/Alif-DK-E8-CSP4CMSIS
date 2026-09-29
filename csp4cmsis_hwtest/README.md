> **Frozen hardware test evidence for CSP4CMSIS 2.0** (library commit `c60665d`, before the 2.0.0 release; the `-O2`/`-Os` runs at `73f46b7`, same library sources). Not an example: the examples, on the released 2.0.0 pack, are on [main](https://github.com/OliverFaust/Alif-DK-E8-CSP4CMSIS/tree/main).

# csp4cmsis_hwtest: CSP4CMSIS regression suite on the DK-E8

Stage 1 of the CSP4CMSIS 2.0 hardware test plan (`docs/hardware_test_plan.md` in the CSP4CMSIS
repository, branch `buffered-channel-v2`): the Corstone-300 FVP regression suite
(`tests/fvp_sse300/bc_tests.cpp`) run on the Alif DevKit-E8, RTSS-HP (Cortex-M55, 400 MHz).
Test branch only.

## What differs from the FVP harness

- `M55_HP/bc_tests.cpp` is a copy of the FVP suite @ `dff8277`. The only change is a block of `BC_*`
  platform macros (defaults = FVP values), set in `M55_HP/M55_HP.cproject.yml`:
  - software interrupt for the ISR tests: `CANFD0_IRQ_IRQn` (104, unused here) at priority 192, below
    `CSP4CMSIS_MAX_SYSCALL_INTERRUPT_PRIORITY` = 128 (unshifted, 8 priority bits). The handler
    `CANFD0_IRQHandler` is resolved into the vector table at link time (the table is in MRAM);
  - sweep ranges unchanged (`kb-1500 .. kb+1000`, search bound 400000).
- RTOS configuration: the DK-E8 files of `csp4cmsis_alt_test`, with the FVP harness's test settings
  (marked `HWTEST`): tick 1 kHz on both backends, no time slicing, timer thread priority 55, 16 KB RTOS
  heap, RTX5 stack watermark and 8 libspace slots; `configSUPPORT_DYNAMIC_ALLOCATION` overridable for
  the heap-free build.
- Code placement: GCC's linker script runs all code from ITCM. The project's copy of the AC6 scatter
  file (`linker_ac6_mram.sct`, marked `HWTEST`) does the same, so both toolchains have the same memory
  timing. I- and D-cache are on.
- `main.cpp` prints a header: clock, caches, library commit, build type, toolchain, RTOS, threshold.

## Build types

`RTX5`, `FreeRTOS` (CMSIS-FreeRTOS 11.3.0 adapter), `RTX5-NoHeap`, `FreeRTOS-NoHeap`, each with
`--toolchain AC6` (6.24) or `GCC` (14.2.1), `-O0`. Output: `out/DevKit-E8/<build type>/<toolchain>/`.

Hardware-only checks (RTX5):
- `RTX5-HwCheck` (`M55_HP/hw_checks.inc`, replaces the suite runner): with v1.0.0, T16a with a
  cycle-step sweep (0..31 NOPs per spin value; the ISR records the victim's preempted PC); with 2.0,
  `putFromISR()` cost and interrupt latency (DWT cycle match -> DebugMonitor -> probe interrupt).
- `RTX5-Soak`: the suite, restarted with `NVIC_SystemReset()` after every SUMMARY.
- `RTX5-HwCheck-O2`: the same checks at `-O2`. For the T16a sweep it adds a tick-driven monitor, a
  DebugMonitor watchdog at priority 0 (DWT cycle match) and prints through `hwtest_raw_puts()`.

Optimised suite builds: `RTX5-O2`, `RTX5-Os`, `FreeRTOS-O2`, `FreeRTOS-Os` (flag passed verbatim).

Fault reports (`HardFault_Handler` in `bc_tests.cpp`) go through `hwtest_raw_puts()` in `main.cpp`,
which polls the UART4 registers: `printf()` cannot be used there, because its stdio lock needs an RTOS
mutex.

## Running

```sh
export HWTEST_SETOOLS=<SETOOLS app-release-exec-linux directory>
./hwtest.sh build v2 RTX5 AC6      # v1 = the v1.0.0 worktree, v2 = the CSP4CMSIS working tree
./hwtest.sh flash v2 RTX5 AC6      # SW4 in the SE position
# SW4 to UART; minicom -D /dev/ttyACM0 -b 115200 -C ~/hwtest_console.log; press RESET
./hwtest.sh collect v2 RTX5 AC6    # last run of the minicom log -> results/
```

Only one program may have `/dev/ttyACM0` open: SETOOLS, minicom or `hwtest.sh capture` (a pyserial
alternative to minicom). With two readers, each gets part of the bytes.

`hwtest.sh build` points the symlink `M55_HP/csp4cmsis_under_test` at the library tree and writes
`M55_HP/lib_version.h` (its commit); both are untracked. `collect` refuses a log whose header names
another library commit, build type or toolchain.

## Results

`results/` holds one log per run. See the CSP4CMSIS repository, `docs/hardware_results_dk_e8.md`, for
the summary.
