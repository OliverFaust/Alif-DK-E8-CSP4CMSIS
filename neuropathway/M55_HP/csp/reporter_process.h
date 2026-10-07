#ifndef REPORTER_PROCESS_H
#define REPORTER_PROCESS_H

#include <stddef.h>
#include <csp/csp4cmsis.h>
#include "common_types.h"

/* The only process that prints while the network runs: it reads Sensor's and
 * Inference's reports from one rendezvous channel (two writers) and prints
 * them, one line per window, and it prints the stack report every
 * CSP_STACK_REPORT_INTERVAL_MS.
 *
 * Stack size: 1024 words, as the Console it replaces -- newlib's printf with
 * %f (logits; the build links -u _printf_float, M55_HP.cproject.yml) is the
 * deepest call. */
class Reporter : public csp::CSProcessStatic<1024> {
public:
    /* others: the processes whose stacks the stack report lists, besides
     * the Reporter itself. */
    Reporter(csp::Chanin<report_t> in, csp::CSProcess* const* others, size_t other_count);
    void run() override;
    const char* name() const override { return "Reporter"; }

private:
    void print(const report_t& r);
    void printStackReport();

    csp::Chanin<report_t> m_in;
    csp::CSProcess* const* m_others;
    size_t m_other_count;
    uint32_t m_et_log_count = 0;
};

#endif /* REPORTER_PROCESS_H */
