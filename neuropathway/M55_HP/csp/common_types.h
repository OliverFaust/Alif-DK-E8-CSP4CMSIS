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
};

/* One window's classification result. */
struct result_t {
    uint32_t index;   /* Matches the window_t::index this result came from. */
    int32_t  predicted; /* 0 = WALKING, 1 = LAYING. */
    float    logits[2];
};

#endif /* COMMON_TYPES_H */
