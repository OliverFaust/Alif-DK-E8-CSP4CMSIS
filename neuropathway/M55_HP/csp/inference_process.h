#ifndef INFERENCE_PROCESS_H
#define INFERENCE_PROCESS_H

#include <csp/csp4cmsis.h>
#include "common_types.h"

/*
 * Stack size: csp4cmsis_neuropathway's Inference (TFLM MicroInterpreter +
 * NMS postprocessing) needed CSProcessStatic<4096>. Ours calls straight
 * into ExecuTorch's Program::load()/load_method()/Method::execute() --
 * a real, if differently-shaped, C++ call chain of comparable depth
 * (proven on hardware in executorch_batch_test, itself run from plain
 * main(), no CSP stack constraint there to compare against). Starting
 * at the same 4096 words as the TFLM precedent; Step 3's stack
 * high-water-mark report will show whether this is over- or under-
 * sized for a real run.
 */
class Inference : public csp::CSProcessStatic<4096> {
public:
    Inference(csp::Chanin<window_t> in, csp::Chanout<report_t> report);
    void run() override;
    const char* name() const override { return "Inference"; }

private:
    void report(report_t r);                  /* also sends the first RuntimeLog once */

    csp::Chanin<window_t>  m_window_in;
    csp::Chanout<report_t> m_report_out;
    bool m_log_reported = false;
};

#endif /* INFERENCE_PROCESS_H */
