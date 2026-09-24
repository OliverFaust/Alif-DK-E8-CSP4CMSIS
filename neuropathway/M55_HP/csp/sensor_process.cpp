/*
 * Sensor process -- ICM-42670 IMU init/decimate/convert/normalize/window,
 * exactly as proven in icm42670_windowing_test/M55_HP/main.c (ran cleanly,
 * indefinitely, 28+ windows/71+ seconds, physically-plausible values --
 * see that project's session report), now split so the one-time IMU
 * init runs once at the top of run() and the per-sample decimation/
 * windowing runs in a continuous loop, writing one window_t per
 * completed window -- same one-time-setup / per-item-loop shape as
 * csp4cmsis_neuropathway's Camera::run().
 *
 * hardware_init() below is single_shot_test_no_ioflex's (Variant A)
 * hardware_init() verbatim: se_services_port_init() +
 * SERVICES_get_run_cfg() + board_pins_config() -- NO
 * SERVICES_set_run_cfg() call anywhere. This is the one load-bearing
 * fact this entire project depends on (see Neuropathway.csolution.yml's
 * header comment for the investigation that established it).
 *
 * PING-PONG BUFFERING (post-diagnosis fix): the original single-buffer
 * version of this file assumed the channel's rendezvous write() blocked
 * until Inference's read() had FULLY completed, including Inference's
 * own use of the data the window_t pointer refers to. That assumption
 * was wrong, confirmed by direct inspection of csp4cmsis/src/
 * alt_channel_sync.cpp's tryHandshake(): the writer's output() copies
 * only the window_t struct itself (index/pointer, ~12 bytes) into the
 * reader's stack variable and returns immediately -- it does not wait
 * for the reader task to even resume, let alone finish dereferencing
 * the pointer. Combined with configUSE_TIME_SLICING=0 and all three
 * CSProcesses sharing one priority, Sensor was free to keep running and
 * start overwriting g_window_buffer before Inference had been
 * scheduled. Live diagnostic capture (paired Sensor/Inference
 * signatures during physical shaking) confirmed this produced both
 * stale (off-by-one-window) and genuinely torn (mid-overwrite) reads.
 *
 * Fix: NUM_WINDOW_BUFFERS (2) ping-pong buffers, each gated by its own
 * binary semaphore that Sensor must take before it's allowed to start
 * decimating a NEW window into that slot, and that Inference gives
 * (via sensor_release_buffer()) immediately after finishing its own
 * copy out of the buffer. This is deliberately NOT "2 buffers should be
 * enough because the window period is long" -- that's the same class
 * of timing assumption that caused the original bug. With the
 * semaphore gate, correctness holds regardless of how slow Inference
 * ever gets: if Inference falls behind by more than one window, Sensor
 * blocks safely on the semaphore instead of corrupting data. */
#include "sensor_process.h"

extern "C" {
#include "Driver_IMU.h"
#include "sensor_utils.h"
#include "se_services_port.h"
}

#include "board_config.h"
#include "FreeRTOS.h"
#include "semphr.h"

#include <cstdio>
#include <cinttypes>
#include <cstring>

using namespace csp;

extern ARM_DRIVER_IMU  ICM42670P;
static ARM_DRIVER_IMU *Drv_IMU = &ICM42670P;

#define DECIMATE_BLOCK 16 /* 800Hz -> 50Hz */
#define DEG_TO_RAD     0.017453292519943295

#define CH_ACC_X  0
#define CH_ACC_Y  1
#define CH_ACC_Z  2
#define CH_GYRO_X 3
#define CH_GYRO_Y 4
#define CH_GYRO_Z 5

/* har_training/data/norm_stats.npz -- exact per-channel mean/std, same
 * values used by every prior stage of this project. */
static const double kChannelMean[NUM_CHANNELS] = {
    0.5018503070, 0.2625706494, 0.2671442628,
    0.0042963331, -0.0035427392, 0.0092124026,
};
static const double kChannelStd[NUM_CHANNELS] = {
    0.4976975024, 0.4924422801, 0.4490955174,
    0.3519084454, 0.3812576532, 0.2384556085,
};

#define NUM_WINDOW_BUFFERS 2
static float g_window_buffer[NUM_WINDOW_BUFFERS][NUM_CHANNELS * WINDOW_SAMPLES];

/* One binary semaphore per buffer slot: "given" means the slot is free
 * for Sensor to write into; "taken" means either Sensor is currently
 * writing into it or Inference is currently reading out of it. Both
 * start "given" (free) at task startup. */
static SemaphoreHandle_t g_buffer_free_sem[NUM_WINDOW_BUFFERS];

void sensor_release_buffer(uint32_t buffer_index)
{
    if (buffer_index < NUM_WINDOW_BUFFERS && g_buffer_free_sem[buffer_index] != nullptr) {
        xSemaphoreGive(g_buffer_free_sem[buffer_index]);
    }
}

/* VARIANT A (single_shot_test_no_ioflex) verbatim: se_services_port_init()
 * + SERVICES_get_run_cfg() + board_pins_config(). NO
 * SERVICES_set_run_cfg() call. board_pins_config() (libs/board_config/
 * board_config.c) already sets 1.8V flex-IO itself via a direct
 * VBAT->GPIO_CTRL register write (FLEX_IO_VOLTAGE_1V8==1, confirmed in
 * this board's board_defs.h) -- the SE IOFLEX call this project omits
 * was always redundant with that. */
static int32_t hardware_init(void)
{
    int32_t       ret        = 0;
    uint32_t      error_code = SERVICES_REQ_SUCCESS;
    uint32_t      service_error_code;
    run_profile_t runp;

    se_services_port_init();

    error_code = SERVICES_get_run_cfg(se_services_s_handle, &runp, &service_error_code);
    if (error_code) {
        printf("Sensor: Get current run config failed\r\n");
        return -1;
    }

    ret = board_pins_config();
    if (ret != 0) {
        printf("Sensor: ERROR board pin mux configuration failed: %" PRId32 "\r\n", ret);
        return ret;
    }

    return ARM_DRIVER_OK;
}

Sensor::Sensor(Chanout<window_t> out) : m_window_out(out), m_window_counter(0) {}

void Sensor::run()
{
    for (int i = 0; i < NUM_WINDOW_BUFFERS; i++) {
        g_buffer_free_sem[i] = xSemaphoreCreateBinary();
        if (g_buffer_free_sem[i] == nullptr) {
            printf("Sensor: FATAL: failed to create buffer-free semaphore %d\r\n", i);
            return;
        }
        xSemaphoreGive(g_buffer_free_sem[i]); /* both slots start free */
    }

    printf("Sensor: initialising IMU (zero SERVICES_set_run_cfg() calls)\r\n");

    int32_t ret = hardware_init();
    if (ret != 0) {
        printf("Sensor: hardware_init failed, aborting\r\n");
        return;
    }

    ret = Drv_IMU->Initialize();
    if (ret != ARM_DRIVER_OK) {
        printf("Sensor: IMU Initialize failed (0x%" PRIx32 ")\r\n", (uint32_t)ret);
        return;
    }
    ret = Drv_IMU->PowerControl(ARM_POWER_FULL);
    if (ret != ARM_DRIVER_OK) {
        printf("Sensor: IMU PowerControl failed (0x%" PRIx32 ")\r\n", (uint32_t)ret);
        return;
    }
    printf("Sensor: IMU initialized and powered up, streaming continuously "
           "(decimate 16x -> convert -> normalize -> window, %d ping-pong "
           "buffers)\r\n", NUM_WINDOW_BUFFERS);

    double   block_sum[NUM_CHANNELS] = {0};
    uint32_t block_count             = 0;
    uint32_t window_pos              = 0;
    uint32_t buffer_idx              = 0;

    /* Claim buffer 0 for the first window. Non-blocking in practice
     * (both slots start free), but correct in principle either way. */
    xSemaphoreTake(g_buffer_free_sem[buffer_idx], portMAX_DELAY);

    ARM_IMU_COORDINATES data[2];
    ARM_IMU_STATUS       status;

    for (;;) {
        ret = Drv_IMU->Control(IMU_SET_INTERRUPT, true);
        if (ret != ARM_DRIVER_OK) {
            printf("Sensor: Error enabling interrupt (0x%" PRIx32 ")\r\n", (uint32_t)ret);
            return;
        }
        status = Drv_IMU->GetStatus();
        if (!status.data_rcvd) {
            continue;
        }

        ret = Drv_IMU->Control(IMU_SET_INTERRUPT, false);
        if (ret != ARM_DRIVER_OK) {
            printf("Sensor: Error disabling interrupt (0x%" PRIx32 ")\r\n", (uint32_t)ret);
            return;
        }
        ret = Drv_IMU->Control(IMU_GET_ACCELEROMETER_DATA, (uint32_t)&data[0]);
        if (ret != ARM_DRIVER_OK) {
            printf("Sensor: Error reading accelerometer data (0x%" PRIx32 ")\r\n", (uint32_t)ret);
            return;
        }
        ret = Drv_IMU->Control(IMU_GET_GYROSCOPE_DATA, (uint32_t)&data[1]);
        if (ret != ARM_DRIVER_OK) {
            printf("Sensor: Error reading gyroscope data (0x%" PRIx32 ")\r\n", (uint32_t)ret);
            return;
        }

        block_sum[CH_ACC_X] += sensor_value_to_double((SENSOR_VALUE *)&data[0].x);
        block_sum[CH_ACC_Y] += sensor_value_to_double((SENSOR_VALUE *)&data[0].y);
        block_sum[CH_ACC_Z] += sensor_value_to_double((SENSOR_VALUE *)&data[0].z);
        block_sum[CH_GYRO_X] += sensor_value_to_double((SENSOR_VALUE *)&data[1].x);
        block_sum[CH_GYRO_Y] += sensor_value_to_double((SENSOR_VALUE *)&data[1].y);
        block_sum[CH_GYRO_Z] += sensor_value_to_double((SENSOR_VALUE *)&data[1].z);
        block_count++;

        if (block_count == DECIMATE_BLOCK) {
            double decimated[NUM_CHANNELS];
            for (int c = 0; c < NUM_CHANNELS; c++) {
                decimated[c] = block_sum[c] / (double)DECIMATE_BLOCK;
            }
            decimated[CH_GYRO_X] *= DEG_TO_RAD;
            decimated[CH_GYRO_Y] *= DEG_TO_RAD;
            decimated[CH_GYRO_Z] *= DEG_TO_RAD;

            for (int c = 0; c < NUM_CHANNELS; c++) {
                double normalized = (decimated[c] - kChannelMean[c]) / kChannelStd[c];
                g_window_buffer[buffer_idx][c * WINDOW_SAMPLES + window_pos] = (float)normalized;
            }

            block_count = 0;
            for (int c = 0; c < NUM_CHANNELS; c++) {
                block_sum[c] = 0;
            }
            window_pos++;

            if (window_pos == WINDOW_SAMPLES) {
                window_t w;
                w.index        = m_window_counter++;
                w.buffer_index = buffer_idx;
                w.data         = g_window_buffer[buffer_idx];

                /* DIAGNOSTIC (handoff staleness investigation): signature
                 * of the raw window as Sensor itself produced it, printed
                 * BEFORE the channel write -- compare against Inference's
                 * own signature of what it actually received. */
                {
                    float sum = 0.0f;
                    for (int i = 0; i < NUM_CHANNELS * WINDOW_SAMPLES; i++) {
                        sum += g_window_buffer[buffer_idx][i];
                    }
                    printf("[Sensor  sig w=%4" PRIu32 " buf=%" PRIu32 "] acc_x[0]=%.6f acc_x[127]=%.6f sum=%.6f\r\n",
                           w.index, w.buffer_index, (double)g_window_buffer[buffer_idx][0],
                           (double)g_window_buffer[buffer_idx][WINDOW_SAMPLES - 1], (double)sum);
                }

                /* Rendezvous write -- synchronizes only the window_t
                 * struct handoff itself (confirmed, see header comment),
                 * NOT anything about buffer occupancy. Correctness of
                 * reusing this buffer slot later is guaranteed entirely
                 * by the semaphore take below, not by this call. */
                m_window_out.write(w);

                window_pos = 0; /* next window is fresh/non-overlapping */

                /* Advance to the next ping-pong slot and claim it --
                 * blocks here (stalling acquisition, not corrupting
                 * data) if Inference hasn't finished with that slot's
                 * PREVIOUS occupant yet. */
                buffer_idx = (buffer_idx + 1) % NUM_WINDOW_BUFFERS;
                xSemaphoreTake(g_buffer_free_sem[buffer_idx], portMAX_DELAY);
            }
        }
    }
}
