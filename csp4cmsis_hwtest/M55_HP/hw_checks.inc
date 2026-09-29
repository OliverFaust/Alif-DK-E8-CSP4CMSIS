// =============================================================================
// Hardware-only checks (DK-E8, docs/hardware_test_plan.md "Hardware-only checks").
// Included at the end of bc_tests.cpp when HWTEST_CHECKS is defined (build type
// RTX5-HwCheck); replaces the suite runner.
//   v1.0.0: T16a-fine -- T16a with a sub-iteration (cycle) sweep; every trial
//           records where the victim was preempted inside registerWaitingTask().
//   2.0:    LAT -- putFromISR() cost and interrupt latency around its masked copy.
// Output: RESULT/SUMMARY lines as in the suite.
// =============================================================================
#include "rtx_os.h"

namespace {

// Saved PC of a preempted RTX5 thread: RTX stores S16-S31 (extended frame only)
// and R4-R11 below the hardware frame R0-R3, R12, LR, PC, xPSR (irq_armv8mml.S).
uint32_t saved_pc(osThreadId_t id) {
    const osRtxThread_t* t = static_cast<const osRtxThread_t*>(id);
    const uint32_t* sp = reinterpret_cast<const uint32_t*>(t->sp);
    const uint32_t n = (((t->stack_frame & 0x10u) == 0u) ? 16u : 0u) + 8u;
    return sp[n + 6u];
}

#if !defined(CSP4CMSIS_ISR_WRITER_API)
// --------------------------------------------------------------------------
// T16a-fine (v1.x). The coarse sweep moves the victim in steps of one spin
// iteration (~13 cycles at 400 MHz); the race window in registerWaitingTask()
// (between its two stores) is 1-2 instructions. Here every k is run with
// 0..HWT_PADS-1 extra NOPs before the operation, and the ISR records the
// victim's preempted PC, so the log shows which instructions were hit.
// --------------------------------------------------------------------------
} // namespace
extern "C" void _ZN3csp8internal15AltChanSyncBase19registerWaitingTaskEPvb();  // address only
namespace {
constexpr uint32_t HWT_PADS = 32;
constexpr uint32_t HWT_SPAN = 128;           // bytes of registerWaitingTask() covered by the histogram
volatile uint32_t g_pad = 0, g_vpc = 0;
uint32_t hwt_hits[HWT_SPAN / 2], hwt_bugs[HWT_SPAN / 2];

__attribute__((noinline)) void pad_nops(uint32_t p) {
    switch (p) {
    case 31: __NOP(); [[fallthrough]];
    case 30: __NOP(); [[fallthrough]];
    case 29: __NOP(); [[fallthrough]];
    case 28: __NOP(); [[fallthrough]];
    case 27: __NOP(); [[fallthrough]];
    case 26: __NOP(); [[fallthrough]];
    case 25: __NOP(); [[fallthrough]];
    case 24: __NOP(); [[fallthrough]];
    case 23: __NOP(); [[fallthrough]];
    case 22: __NOP(); [[fallthrough]];
    case 21: __NOP(); [[fallthrough]];
    case 20: __NOP(); [[fallthrough]];
    case 19: __NOP(); [[fallthrough]];
    case 18: __NOP(); [[fallthrough]];
    case 17: __NOP(); [[fallthrough]];
    case 16: __NOP(); [[fallthrough]];
    case 15: __NOP(); [[fallthrough]];
    case 14: __NOP(); [[fallthrough]];
    case 13: __NOP(); [[fallthrough]];
    case 12: __NOP(); [[fallthrough]];
    case 11: __NOP(); [[fallthrough]];
    case 10: __NOP(); [[fallthrough]];
    case 9: __NOP(); [[fallthrough]];
    case 8: __NOP(); [[fallthrough]];
    case 7: __NOP(); [[fallthrough]];
    case 6: __NOP(); [[fallthrough]];
    case 5: __NOP(); [[fallthrough]];
    case 4: __NOP(); [[fallthrough]];
    case 3: __NOP(); [[fallthrough]];
    case 2: __NOP(); [[fallthrough]];
    case 1: __NOP(); [[fallthrough]];
    default: break;
    }
}

void fine_victim_main(void* arg) {
    Case* c = static_cast<Case*>(arg);
    for (;;) {
        osThreadFlagsWait(F_START, osFlagsWaitAny, osWaitForever);
        g_stage = 0; spin(g_k); pad_nops(g_pad);
        g_stage = 1; g_pre = c->probe();
        g_stage = 2; c->victim_op();
        g_stage = 3; osThreadFlagsSet(c->runner, F_ACK);
        g_stage = 4;
    }
}

// After a BUG the channel is left inconsistent (the second store lands after the
// ISR cleared the registration) and the next rendezvous blocks for good, so every
// BUG moves the test to a fresh channel from this pool.
constexpr uint32_t HWT_POOL = 8;
csp::Channel<uint32_t> hwt_pool[HWT_POOL];
uint32_t hwt_next = 0;
void t16af_victim() { In in = t16a_ch->reader(); uint32_t v = T15_SENT; in >> v; t16a_got = v; }
void t16af_isr() { g_vpc = saved_pc(t16a.victim); t16a_isr(); }
void t16af_aggr() {
    g_isr_op = t16af_isr; isr_fire(); g_isr_op = nullptr;
    if (!t16a_isr_ok) { Out out = t16a_ch->writer(); out << T16A_KICK; }
}

void test_T16a_fine() {
    t16a_ch = &hwt_pool[hwt_next++]; g_current_test = "T16a-fine";
    t16a = {"T16a-fine", t16a_reset, t16a_probe, t16af_victim, t16af_aggr, nullptr, nullptr, t16a_check, nullptr, osThreadGetId()};
    t16a.victim = spawn(fine_victim_main, &t16a, osPriorityLow, t16a_s, "T16af");
    const uint32_t f0 = reinterpret_cast<uint32_t>(&_ZN3csp8internal15AltChanSyncBase19registerWaitingTaskEPvb) & ~1u;

    uint32_t lo = 0, hi = BC_SEARCH_HI; g_pad = 0;
    while (hi - lo > 1) { uint32_t mid = lo + (hi - lo) / 2; if (trial(t16a, mid) == LATE) hi = mid; else lo = mid; }
    const uint32_t kb = hi, k0 = kb > BC_SWEEP_BELOW ? kb - BC_SWEEP_BELOW : 0, k1 = kb + BC_SWEEP_ABOVE;
    printf("   [T16a-fine] registerWaitingTask @0x%08lx; kb=%lu; k=%lu..%lu x pad 0..%lu NOPs\r\n",
           (unsigned long)f0, (unsigned long)kb, (unsigned long)k0, (unsigned long)k1, (unsigned long)(HWT_PADS - 1));
    uint32_t n[4] = {0, 0, 0, 0}, inside = 0;
    for (uint32_t k = k0; k <= k1; ++k) {
        for (uint32_t p = 0; p < HWT_PADS; ++p) {
            g_pad = p; g_vpc = 0;
            Outcome o = trial(t16a, k);
            n[o]++;
            uint32_t off = g_vpc - f0;
            if (off < HWT_SPAN) { inside++; hwt_hits[off / 2]++; if (o == BUG) hwt_bugs[off / 2]++; }
            if (o == BUG) {
                printf("   [T16a-fine] BUG at k=%lu pad=%lu: victim PC=0x%08lx (+0x%02lx)\r\n", (unsigned long)k,
                       (unsigned long)p, (unsigned long)g_vpc, (unsigned long)off);
                if (hwt_next == HWT_POOL) {
                    printf("   [T16a-fine] channel pool exhausted after %lu BUGs -- sweep stopped\r\n", (unsigned long)n[BUG]);
                    k = k1; break;
                }
                t16a_ch = &hwt_pool[hwt_next++];
            }
            if (o == ANOMALY) { printf("   [T16a-fine] ANOMALY at k=%lu pad=%lu (stage %lu) -- aborted\r\n",
                                       (unsigned long)k, (unsigned long)p, (unsigned long)g_seen); k = k1; break; }
        }
        if (((k - k0) % 250u) == 0u)
            printf("   [T16a-fine] k=%lu: EARLY=%lu LATE=%lu BUG=%lu, preempted inside registerWaitingTask=%lu\r\n",
                   (unsigned long)k, (unsigned long)n[EARLY], (unsigned long)n[LATE], (unsigned long)n[BUG], (unsigned long)inside);
    }
    printf("   [T16a-fine] trials=%lu EARLY=%lu LATE=%lu BUG=%lu ANOMALY=%lu; preempted inside registerWaitingTask=%lu\r\n",
           (unsigned long)(n[0] + n[1] + n[2] + n[3]), (unsigned long)n[EARLY], (unsigned long)n[LATE],
           (unsigned long)n[BUG], (unsigned long)n[ANOMALY], (unsigned long)inside);
    for (uint32_t i = 0; i < HWT_SPAN / 2; ++i)
        if (hwt_hits[i]) printf("   [T16a-fine] victim PC +0x%02lx: %lu trials, %lu BUG\r\n", (unsigned long)(2 * i),
                                (unsigned long)hwt_hits[i], (unsigned long)hwt_bugs[i]);
    result("T16a-fine", (n[BUG] == 0 && n[ANOMALY] == 0 && n[EARLY] > 0 && n[LATE] > 0) ? 1 : 0,
           "rendezvous putFromISR() vs a reader entering input(), cycle-step sweep");
}

void hw_runner(void*) {
    printf("\r\n=== CSP4CMSIS hardware-only checks ===\r\n");
    printf("backend: %s; library: %s; %s\r\n", BACKEND_NAME, LIB_NAME, HEAP_MODE);
    isr_init();
    test_T16a_fine();
    printf("SUMMARY: PASS=%lu FAIL=%lu SKIP=%lu REPLACED=%lu\r\n", (unsigned long)g_pass, (unsigned long)g_fail,
           (unsigned long)g_skip, (unsigned long)g_replaced);
    printf("\x04");
    park();
}

#else  // 2.0
// --------------------------------------------------------------------------
// LAT (2.0). (a) Cycles of IsrChanout::putFromISR() called from the SWI
// handler: KeepNewest into a full 1-slot channel (only the masked copy, no RTOS
// call) and Block into an empty channel with a reader blocked in input() (copy
// + wake). (b) Interrupt latency: the runner (kernel locked) performs
// back-to-back putFromISR() copies into a full KeepNewest channel (masked copy,
// no RTOS call, so thread mode is fine); a DWT cycle-count match raises
// DebugMonitor (priority 64, never masked) at a chosen cycle, which pends the
// probe interrupt BC_SWI_IRQn; the probe handler records pend -> entry.
// Probe at priority 64 (above CSP4CMSIS_MAX_SYSCALL_INTERRUPT_PRIORITY) and
// 160 (below it: held off by the masked copy). Baseline: plain copies, no
// masking. (DebugMonitor itself cannot be the probe: a debug event that arrives
// while DebugMonitor is masked is dropped, not held pending.)
// --------------------------------------------------------------------------
template <size_t N> struct Blob { uint8_t b[N]; };
constexpr uint32_t LAT_N = 256;               // measurements per case

struct Stat { uint32_t n = 0, min = UINT32_MAX, max = 0; uint64_t sum = 0;
              void add(uint32_t v) { n++; sum += v; if (v < min) min = v; if (v > max) max = v; } };
void print_stat(const char* what, const Stat& s) {
    printf("   [LAT] %-44s n=%lu min=%lu avg=%lu max=%lu cycles\r\n", what, (unsigned long)s.n,
           (unsigned long)(s.n ? s.min : 0), (unsigned long)(s.n ? s.sum / s.n : 0), (unsigned long)s.max);
}

// ---- (a) cost of putFromISR() ---------------------------------------------
Stat g_cost;
template <size_t N> struct CostCase {
    static inline csp::SamplingBufferedChannel<Blob<N>, 1, csp::BufferPolicy::KeepNewest>* keep = nullptr;
    static inline csp::BufferedChannel<Blob<N>, 4>* block = nullptr;
    static inline Blob<N> src{};
    static void isr_keep()  { auto w = keep->isrWriter();  uint32_t t0 = DWT->CYCCNT; (void)w.putFromISR(src); g_cost.add(DWT->CYCCNT - t0); }
    static void isr_block() { auto w = block->isrWriter(); uint32_t t0 = DWT->CYCCNT; (void)w.putFromISR(src); g_cost.add(DWT->CYCCNT - t0); }
    static inline ThreadSlot rslot;
    static void reader(void*) { auto in = block->reader(); static Blob<N> d; for (;;) in >> d; }
    static void run() {
        static csp::SamplingBufferedChannel<Blob<N>, 1, csp::BufferPolicy::KeepNewest> k; keep = &k;
        static csp::BufferedChannel<Blob<N>, 4> b; block = &b;
        char what[64];
        { auto w = k.isrWriter(); (void)w.putFromISR(src); }   // full from now on: every put overwrites
        g_cost = Stat{}; g_isr_op = isr_keep;
        for (uint32_t i = 0; i < LAT_N; ++i) { isr_fire(); }
        g_isr_op = nullptr;
        snprintf(what, sizeof what, "putFromISR %u B, KeepNewest full (copy only)", (unsigned)N); print_stat(what, g_cost);
        spawn(reader, nullptr, osPriorityNormal, rslot, "LATr");
        g_cost = Stat{};
        for (uint32_t i = 0; i < LAT_N; ++i) { osDelay(1); g_isr_op = isr_block; isr_fire(); g_isr_op = nullptr; }
        snprintf(what, sizeof what, "putFromISR %u B, Block, reader waiting", (unsigned)N); print_stat(what, g_cost);
    }
};

// ---- (b) latency with a cycle-exact trigger (DWT match -> DebugMonitor) ---
volatile uint32_t g_dm_target = 0, g_dm_entry = 0, g_dm_hits = 0, g_pb_pend = 0, g_pb_entry = 0, g_pb_hits = 0;
volatile bool g_dm_armed = false;
} // namespace
extern "C" void DebugMon_Handler(void) {
    const uint32_t now = DWT->CYCCNT;
    (void)DWT->FUNCTION0;                       // reading clears MATCHED
    DWT->FUNCTION0 = 0;                         // disarm
    if (g_dm_armed) {
        g_dm_entry = now; g_dm_hits = g_dm_hits + 1; g_dm_armed = false;
        g_pb_pend = DWT->CYCCNT;
        NVIC_SetPendingIRQ(BC_SWI_IRQn);        // the probe
    }
}
namespace {
void probe_isr() { g_pb_entry = DWT->CYCCNT; g_pb_hits = g_pb_hits + 1; }
bool dm_available = false;
bool dm_setup() {
    CoreDebug->DEMCR |= CoreDebug_DEMCR_MON_EN_Msk | CoreDebug_DEMCR_TRCENA_Msk;
    DWT->FUNCTION0 = 0;
    DWT->COMP0 = 0;
    DWT->FUNCTION0 = (1u << DWT_FUNCTION_ACTION_Pos) | (1u << DWT_FUNCTION_MATCH_Pos);   // cycle counter match -> debug event
    const uint32_t f = DWT->FUNCTION0;
    DWT->FUNCTION0 = 0;
    printf("   [LAT] DWT: NUMCOMP=%lu FUNCTION0 readback=0x%08lx; DHCSR.C_DEBUGEN=%lu; DEMCR=0x%08lx (MON_EN=%lu SDME=%lu)\r\n",
           (unsigned long)((DWT->CTRL & DWT_CTRL_NUMCOMP_Msk) >> DWT_CTRL_NUMCOMP_Pos), (unsigned long)f,
           (unsigned long)(CoreDebug->DHCSR & 1u), (unsigned long)CoreDebug->DEMCR,
           (unsigned long)((CoreDebug->DEMCR & CoreDebug_DEMCR_MON_EN_Msk) ? 1 : 0),
           (unsigned long)((CoreDebug->DEMCR & (1u << 20)) ? 1 : 0));
    return ((f & DWT_FUNCTION_MATCH_Msk) >> DWT_FUNCTION_MATCH_Pos) == 1u;
}
void dm_arm(uint32_t at) {
    g_dm_target = at; g_dm_armed = true;
    DWT->COMP0 = at;
    DWT->FUNCTION0 = (1u << DWT_FUNCTION_ACTION_Pos) | (1u << DWT_FUNCTION_MATCH_Pos);
}

constexpr uint32_t LAT_WORK = 200;            // copies (or plain passes) per SWI activation
Stat g_lat, g_dml;
uint32_t g_lat_miss = 0;
volatile uint32_t g_offset = 0;
template <size_t N> struct LatCase {
    static inline csp::SamplingBufferedChannel<Blob<N>, 1, csp::BufferPolicy::KeepNewest>* ch = nullptr;
    static inline Blob<N> src{}, sink{};
    static void load_copies() {
        auto w = ch->isrWriter();
        dm_arm(DWT->CYCCNT + g_offset);
        for (uint32_t i = 0; i < LAT_WORK; ++i) (void)w.putFromISR(src);
    }
    static void load_plain() {                // same kind of work, no masking
        dm_arm(DWT->CYCCNT + g_offset);
        for (uint32_t i = 0; i < LAT_WORK; ++i) { std::memcpy(&sink, &src, N); __DSB(); }
    }
    static void measure(void (*op)(), uint32_t span, const char* label, uint32_t prio) {
        char what[64];
        NVIC_SetPriority(BC_SWI_IRQn, prio);
        g_lat = Stat{}; g_dml = Stat{}; g_lat_miss = 0;
        uint32_t seed = 12345u;
        for (uint32_t i = 0; i < LAT_N; ++i) {
            seed = seed * 1664525u + 1013904223u;
            g_offset = 200u + (seed >> 8) % span;      // somewhere inside the handler's work
            g_dm_hits = 0; g_pb_hits = 0;
            int32_t lk = osKernelLock(); op(); osKernelRestoreLock(lk);
            for (volatile int w = 0; w < 100; ++w) { }   // let a late probe arrive
            if (g_dm_hits && g_pb_hits) { g_lat.add(g_pb_entry - g_pb_pend); g_dml.add(g_dm_entry - g_dm_target); }
            else { g_lat_miss++; DWT->FUNCTION0 = 0; g_dm_armed = false; }
        }
        snprintf(what, sizeof what, "%s %u B, probe prio %lu", label, (unsigned)N, (unsigned long)prio);
        print_stat(what, g_lat);
        print_stat("  (trigger: DWT match -> DebugMonitor entry)", g_dml);
        if (g_lat_miss) printf("   [LAT]   (%lu of %lu triggers missed)\r\n", (unsigned long)g_lat_miss, (unsigned long)LAT_N);
    }
    static void run() {
        static csp::SamplingBufferedChannel<Blob<N>, 1, csp::BufferPolicy::KeepNewest> c; ch = &c;
        { auto w = c.isrWriter(); (void)w.putFromISR(src); }
        // Span: how long the handler's work takes (measured once).
        g_offset = 0x7fffffffu;
        uint32_t t0 = DWT->CYCCNT; load_plain();
        uint32_t span = (DWT->CYCCNT - t0) / 2u; DWT->FUNCTION0 = 0; g_dm_armed = false;
        measure(load_plain,  span, "latency, plain copies", 64);
        measure(load_copies, span, "latency, putFromISR copies", 64);
        measure(load_plain,  span, "latency, plain copies", 160);
        measure(load_copies, span, "latency, putFromISR copies", 160);
    }
};

void test_LAT() {
    g_current_test = "LAT";
    printf("   [LAT] SystemCoreClock=%lu Hz; SWI priority %d; threshold %d\r\n", (unsigned long)SystemCoreClock,
           (int)BC_SWI_PRIO, (int)CSP4CMSIS_MAX_SYSCALL_INTERRUPT_PRIORITY);
    CostCase<4>::run(); CostCase<64>::run(); CostCase<1024>::run();
    dm_available = dm_setup();
    if (!dm_available) {
        printf("   [LAT] DWT cycle-count match not available: latency part skipped\r\n");
        result("LAT", -1, "interrupt latency around putFromISR()'s masked copy (needs DWT cycle match)");
        return;
    }
    NVIC_SetPriority(DebugMonitor_IRQn, 64);
    g_isr_op = probe_isr;                     // BC_SWI_IRQn is the probe from here on
    LatCase<4>::run(); LatCase<64>::run(); LatCase<1024>::run();
    result("LAT", 1, "putFromISR() cost and interrupt latency measured (values above; judged in the report)");
}

void hw_runner(void*) {
    printf("\r\n=== CSP4CMSIS hardware-only checks ===\r\n");
    printf("backend: %s; library: %s; %s\r\n", BACKEND_NAME, LIB_NAME, HEAP_MODE);
    isr_init();
    test_LAT();
    printf("SUMMARY: PASS=%lu FAIL=%lu SKIP=%lu REPLACED=%lu\r\n", (unsigned long)g_pass, (unsigned long)g_fail,
           (unsigned long)g_skip, (unsigned long)g_replaced);
    printf("\x04");
    park();
}
#endif

} // namespace

extern "C" void csp_app_main_init(void) {
    spawn(hw_runner, nullptr, osPriorityHigh, runner_slot, "hwrunner");
}
