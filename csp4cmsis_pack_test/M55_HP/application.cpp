#include "csp/csp4cmsis.h"
#include <cstdio>

// --- Configuration ---
#define TOTAL_MESSAGES_PER_SENDER 10000 // Reduced for clarity in terminal output
#define CHECK_INTERVAL 1000           
#define MAX_TOTAL_MESSAGES (TOTAL_MESSAGES_PER_SENDER * 2)

using namespace csp;

// How often to print a stack-usage report for the whole network. This is a
// *live* reading (CSProcess::stackHighWaterMarkWords()), not a one-shot
// end-of-run report -- Sender/Receiver both park permanently (osThreadSuspend)
// after finishing rather than exiting, so the report loop below keeps
// running for the life of the task. Read the HWM columns to right-size the
// provisional CSProcessStatic<N> values on Sender/Receiver below -- those
// numbers were picked before this test ever ran, not measured.
#ifndef CSP_STACK_REPORT_INTERVAL_MS
#define CSP_STACK_REPORT_INTERVAL_MS (3000)
#endif

// Set in RunProcessingChainTest() right after osThreadNew() succeeds, so the
// report loop can also measure MainApp_Task's own stack headroom (it isn't
// a CSProcess, so it doesn't get a stackHighWaterMarkWords() of its own).
static osThreadId_t s_main_app_task_handle = NULL;
#define MAIN_APP_STACK_WORDS 4096

// MainApp_Task's priority, one step above the CSP network it spawns
// (csp4cmsis's own CSP_LEGACY_PARALLEL_PRIORITY, osPriorityLow) --
// preserving the old native-FreeRTOS relationship this task was
// originally given (tskIDLE_PRIORITY+3 vs. the network's tskIDLE_
// PRIORITY+2 -- exactly one step apart) on evidence-of-intent grounds.
// No functional requirement was found that demands this specific gap:
// MainApp_Task's only post-spawn work is a periodic (3s) diagnostic
// printf report, nothing time-sensitive, and time-slicing is enabled in
// this project's RTOS2 config, so equal priority to the network wouldn't
// starve it either. This preserves the deliberate-looking old numeric
// relationship, not a verified functional need -- noted so a future
// reader doesn't mistake one for the other.
constexpr osPriority_t CSP_MAIN_APP_PRIORITY = osPriorityLow1;

struct Message {
    int source_id; 
    int sequence_num;
};

// --- 1. Define Channels ---
// 'Channel' or 'One2OneChannel' now represents a Rendezvous (capacity 0) sync point.
using AltChannel = Channel<Message>;

// CSProcess is abstract: stackWords()/stackBuffer()/taskBuffer() are pure
// virtual, so each process needs its own static stack + thread-control-
// block storage. CSProcessStatic<N> is the library helper that supplies that
// storage -- inherit from it instead of CSProcess directly, with N as the
// stack size in words. 512 is a provisional starting point for Sender
// (a tight loop with no deep call stack); confirm/right-size it against
// the HWM report added to MainApp_Task below.
class Sender : public CSProcessStatic<512> {
private:
    Chanout<Message> out;
    int id; 
public:
    Sender(Chanout<Message> w, int sender_id) : out(w), id(sender_id) {}
    const char* name() const override {
        static char buf[12];
        snprintf(buf, sizeof(buf), "Snd%d", id);
        return buf;
    }

    void run() override {
        printf("[Sender %d] Starting sequence.\r\n", id);
        for (int i = 0; i < TOTAL_MESSAGES_PER_SENDER; ++i) {
            Message msg = {id, i};
            out << msg; 
        }
        printf("[Sender %d] Finished.\r\n", id);
        // Permanent park -- confirmed nothing ever resumes this thread
        // (no osThreadResume() call anywhere, and MainApp_Task's periodic
        // stack-HWM query is a passive read of kernel bookkeeping, not a
        // signal to this thread). A single self-suspend call is
        // sufficient; a successfully suspended thread doesn't return to
        // continue looping, so no while(true) wrapper is needed.
        osThreadSuspend(osThreadGetId());
    }
};

// 1024 words is provisional: Receiver holds a 2-guard Alternative on the
// stack plus calls printf (newlib's printf can be stack-hungry). Confirm
// against the HWM report below.
class Receiver : public CSProcessStatic<1024> {
private:
    Chanin<Message> inA;
    Chanin<Message> inB;
public:
    Receiver(Chanin<Message> rA, Chanin<Message> rB) : inA(rA), inB(rB) {}
    const char* name() const override { return "Receiver"; }

    void run() override {
        osDelay(10);
        printf("[Receiver] Task running. Using Resident-Guard ALT.\r\n");

        Message msgA, msgB; 
        int count = 0;
        int next_seqA = 0;
        int next_seqB = 0;
        bool error_found = false;

        // The Alternative object is on the stack.
        // It borrows pointers to guards that live inside chan_A and chan_B.
        Alternative alt(inA | msgA, inB | msgB);

        while(count < MAX_TOTAL_MESSAGES) {
            // fairSelect is now heap-free.
            int selected = alt.fairSelect();
            
            if (selected == 0) {
                if (msgA.source_id != 1 || msgA.sequence_num != next_seqA) {
                    printf("!! DATA ERROR Chan A: Expected ID 1 Seq %d, Got ID %d Seq %d\r\n", 
                            next_seqA, msgA.source_id, msgA.sequence_num);
                    error_found = true;
                }
                next_seqA++;
            } 
            else if (selected == 1) {
                if (msgB.source_id != 2 || msgB.sequence_num != next_seqB) {
                    printf("!! DATA ERROR Chan B: Expected ID 2 Seq %d, Got ID %d Seq %d\r\n", 
                            next_seqB, msgB.source_id, msgB.sequence_num);
                    error_found = true;
                }
                next_seqB++;
            }

            count++;
            if (count % CHECK_INTERVAL == 0) {
                printf("[Receiver] Verified %d messages...\r\n", count);
                if (error_found) break;
            }
        }

        if (!error_found) {
            printf("[Receiver] SUCCESS: %d messages verified heap-free.\r\n", count);
        }
        // Permanent park -- see Sender::run()'s matching comment.
        osThreadSuspend(osThreadGetId());
    }
};

// --- 3. The Main Application Task ---
void MainApp_Task(void* params) {
    osDelay(500);

    printf("\r\n--- BOli2 Launching CSP Static Network (Zero-Heap) ---\r\n");
    
    // NEW: No constructor arguments needed for Rendezvous.
    // These are placed in static memory (.data segment).
    static AltChannel chan_A; 
    static AltChannel chan_B; 

    static Sender sA(chan_A.writer(), 1);
    static Sender sB(chan_B.writer(), 2);
    static Receiver r1(chan_A.reader(), chan_B.reader());

    // Run parallel processes using static execution
    auto network = InParallel(sA, sB, r1);
    Run(network, ExecutionMode::StaticNetwork);

    printf("*** MainApp_Task: Run() returned, entering stack-report loop ***\r\n");

    // Sender/Receiver never return from run() (both park permanently via
    // osThreadSuspend() once done), so there's no "network finished"
    // point to report stack usage at -- report periodically.
    while (true) {
        osDelay(CSP_STACK_REPORT_INTERVAL_MS);

        if (s_main_app_task_handle != NULL) {
            // Mirrors csp4cmsis's own stackHighWaterMarkWords()
            // (process.h) exactly: osThreadGetStackSpace() returns
            // bytes, divided by the same portable word-size type the
            // library itself uses.
            uint32_t hwm = osThreadGetStackSpace(s_main_app_task_handle)
                            / sizeof(internal::csp_stack_word_t);
            size_t unused_bytes = hwm * sizeof(internal::csp_stack_word_t);
            printf("MainApp: %u bytes unused headroom (%u words HWM, of %u allocated)\r\n",
                    (unsigned)unused_bytes, (unsigned)hwm, (unsigned)MAIN_APP_STACK_WORDS);
        }

        network.forEachProcess([](CSProcess& p) {
            // API 1.3: stack depth is fixed at compile time -- no more
            // resolveStackWords()/fallback constant to consult, just ask
            // the process directly.
            size_t allocated_words = p.stackWords();
            size_t allocated_bytes = allocated_words * sizeof(internal::csp_stack_word_t);

            uint32_t hwm = p.stackHighWaterMarkWords();
            if (hwm == CSP_STACK_HWM_UNAVAILABLE) {
                printf("%s: allocated = %u words (%u bytes), HWM unavailable\r\n",
                        p.name(), (unsigned)allocated_words, (unsigned)allocated_bytes);
            } else {
                size_t unused_bytes = hwm * sizeof(internal::csp_stack_word_t);
                size_t used_bytes = (unused_bytes <= allocated_bytes)
                                        ? allocated_bytes - unused_bytes
                                        : 0; // guard against any inconsistency
                printf("%s: %u/%u bytes used (%u bytes unused headroom, %u words HWM)\r\n",
                        p.name(), (unsigned)used_bytes, (unsigned)allocated_bytes,
                        (unsigned)unused_bytes, (unsigned)hwm);
            }
        });
    }
}

// MainApp_Task's own stack buffer and (when this project opts into
// static allocation) thread control block -- same pattern csp4cmsis's
// own Run()/ParallelHelper use for CSP processes (public_task.h, run.h),
// applied here since MainApp_Task is this app's own task, not a
// CSProcess. static storage, matching this file's other static objects
// (chan_A/chan_B/sA/sB/r1 in MainApp_Task above).
// alignas(8): see process.h's CSProcessStatic<N>::m_stack comment -- same
// requirement applies here, this is the same kind of raw stack buffer.
alignas(8) static internal::csp_stack_word_t s_main_app_stack[MAIN_APP_STACK_WORDS];
#if defined(CSP4CMSIS_STATIC_ALLOCATION)
static internal::csp_static_thread_storage_t s_main_app_tcb;
#endif

void RunProcessingChainTest(void) {
    osThreadAttr_t attr = {};
    attr.name       = "MainApp";
    attr.stack_mem  = s_main_app_stack;
    attr.stack_size = MAIN_APP_STACK_WORDS * sizeof(internal::csp_stack_word_t);
#if defined(CSP4CMSIS_STATIC_ALLOCATION)
    attr.cb_mem     = &s_main_app_tcb;
    attr.cb_size    = sizeof(internal::csp_static_thread_storage_t);
#endif
    attr.priority   = CSP_MAIN_APP_PRIORITY;

    s_main_app_task_handle = osThreadNew(MainApp_Task, NULL, &attr);
    if (s_main_app_task_handle == NULL) {
        printf("ERROR: MainApp_Task creation failed!\r\n");
    }
}
