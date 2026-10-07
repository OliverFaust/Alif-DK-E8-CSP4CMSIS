/*
 * Channel payload types for the Sensor/Inference/Console network.
 *
 * Shaped after csp4cmsis_neuropathway's frame_t/result_t (common_types.h)
 * with the equivalent substitution: a completed IMU window in place of a
 * captured camera frame, a WALKING/LAYING classification in place of a
 * detection-box list.
 */
#ifndef COMMON_TYPES_H
#define COMMON_TYPES_H

#include <cstdint>
#include <type_traits>

#define NUM_CHANNELS   6
#define WINDOW_SAMPLES 128

/* One completed, normalized 128x6 window, ready for inference.
 *
 * Carries a POINTER into one of Sensor's ping-pong window buffers, not
 * the 3072-byte payload itself. NOTE, corrected after live diagnosis:
 * the original single-buffer version of this comment claimed "Sensor::
 * run()'s rendezvous write() blocks until Inference::run()'s read()
 * completes" -- confirmed WRONG by direct inspection of
 * csp4cmsis/src/alt_channel_sync.cpp's tryHandshake(): the writer's
 * output() copies only the window_t struct itself into the reader's
 * stack variable and returns immediately, without waiting for the
 * reader task to resume or dereference the pointer. Live diagnostic
 * capture (paired Sensor/Inference signatures during physical shaking)
 * confirmed this produced both stale (off-by-one-window) and genuinely
 * torn (mid-overwrite) reads on a single shared buffer. buffer_index
 * (below) plus sensor_process.h's sensor_release_buffer() close this
 * gap with an explicit per-buffer acknowledgment, not just alternation
 * -- see sensor_process.cpp's own header comment for the full design
 * rationale. */
struct window_t {
    uint32_t index;        /* Monotonic window counter, for correlating
                             * Console's output back to a specific window. */
    uint32_t buffer_index;  /* Which of Sensor's ping-pong buffers data
                             * points into -- Inference must pass this to
                             * sensor_release_buffer() once it's done
                             * reading, so Sensor knows it's safe to
                             * reuse that specific buffer slot. */
    const float* data;     /* NUM_CHANNELS*WINDOW_SAMPLES floats,
                             * channel-major then time (matches training's
                             * (N,6,128) layout) -- owned by Sensor. */
    float sensor_sum;      /* Sum of the window's samples as Sensor wrote
                             * them: Inference compares it with its own sum
                             * of what it received (handoff check), and the
                             * Reporter prints both. */
};

/* One report to the Reporter, the only process that prints while the
 * network runs: the console is a shared resource (printf from two threads
 * interleaves or drops characters), so Sensor and Inference send what they
 * have to say as a report_t over one rendezvous channel with several
 * writers, and the Reporter owns every format string. */
enum class ReportSource : uint8_t { Sensor, Inference };
enum class ReportKind : uint8_t {
    Started,     /* initialised and running; aux: Inference's method pool bytes */
    Failed,      /* stopped; code: the failing step, aux: the driver/runtime error */
    Result,      /* one window (Inference); code >= 0: the predicted label,
                  * code < 0: the failing step, aux: the runtime error */
    RuntimeLog,  /* ExecuTorch's first log message; code: its source line,
                  * aux: its level character (later ones are only counted) */
};

struct report_t {
    ReportKind   kind;
    ReportSource source;
    int16_t      code;
    uint32_t     index;     /* Result, RuntimeLog: the window index */
    uint32_t     buffer;    /* Result: the ping-pong buffer the window came in */
    uint32_t     aux;       /* see ReportKind */
    uint32_t     count;     /* Result: ExecuTorch log messages so far */
    float        value[4];  /* Result: sensor sum, inference sum, logit 0, logit 1 */
};
static_assert(std::is_trivially_copyable<report_t>::value,
              "channel element types must be trivially copyable");

#endif /* COMMON_TYPES_H */
