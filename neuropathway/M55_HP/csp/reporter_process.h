#ifndef REPORTER_PROCESS_H
#define REPORTER_PROCESS_H

#include <csp/csp4cmsis.h>
#include "common_types.h"

/* The only process that prints while the network runs: it reads reports from
 * one rendezvous channel (writers: Sensor, Inference, MainApp) and prints each
 * one as one line. It has no timer: the stack report is MainApp's, so it does
 * not depend on report traffic.
 *
 * Stack size: 1024 words, as the Console it replaces -- newlib's printf with
 * %f (logits; the build links -u _printf_float, M55_HP.cproject.yml) is the
 * deepest call. */
class Reporter : public csp::CSProcessStatic<1024> {
public:
    explicit Reporter(csp::Chanin<report_t> in);
    void run() override;
    const char* name() const override { return "Reporter"; }

private:
    void print(const report_t& r);

    csp::Chanin<report_t> m_in;
    uint32_t m_et_log_count = 0;   /* from the latest Result, printed with the stack report */
};

#endif /* REPORTER_PROCESS_H */
