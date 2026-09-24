#ifndef SENSOR_PROCESS_H
#define SENSOR_PROCESS_H

#include <csp/csp4cmsis.h>
#include "common_types.h"

/*
 * Stack size: mirrors csp4cmsis_neuropathway's Camera process
 * (CSProcessStatic<1024>, "newlib's printf can be stack-hungry" --
 * csp4cmsis_alt_test's own Receiver precedent). Sensor::run()'s own call
 * depth (Driver_IMU's Control()/GetStatus(), sensor_value_to_double())
 * is shallower than Camera's CameraCaptureInit()/Start(), but printf's
 * cost dominates either way -- same 1024 words, not re-derived.
 */
class Sensor : public csp::CSProcessStatic<1024> {
public:
    explicit Sensor(csp::Chanout<window_t> out);
    void run() override;
    const char* name() const override { return "Sensor"; }

private:
    csp::Chanout<window_t> m_window_out;
    uint32_t m_window_counter;
};

/* Called by Inference once it has finished reading a buffer's contents
 * (i.e. right after its own memcpy out of window_t::data, before doing
 * anything else with the copy) -- releases Sensor to reuse that
 * specific ping-pong buffer slot. See sensor_process.cpp's header
 * comment for why this explicit acknowledgment is required rather than
 * just alternating buffers by window count. */
void sensor_release_buffer(uint32_t buffer_index);

#endif /* SENSOR_PROCESS_H */
