#include "reporter_process.h"

#include <cstdio>
#include <cinttypes>

using namespace csp;

static const char *kLabels[2] = {"WALKING", "LAYING"};

static const char* sourceName(ReportSource s)
{
    switch (s) {
    case ReportSource::Sensor:    return "Sensor";
    case ReportSource::Inference: return "Inference";
    case ReportSource::Reporter:  return "Reporter";
    case ReportSource::Main:      return "CSP_Main";
    }
    return "?";
}

Reporter::Reporter(Chanin<report_t> in) : m_in(in)
{
}

void Reporter::run()
{
    while (true) {
        report_t r;
        m_in.read(r);
        print(r);
    }
}

void Reporter::print(const report_t& r)
{
    switch (r.kind) {
    case ReportKind::Started:
        if (r.source == ReportSource::Inference) {
            printf("Inference: model ready (method_allocation_pool[SRAM0]=%" PRIu32 " bytes)\r\n", r.aux);
        } else {
            printf("Sensor: IMU streaming (decimate 16x -> convert -> normalize -> window)\r\n");
        }
        break;
    case ReportKind::Failed:
        printf("%s: FAILED at step %d (error 0x%" PRIx32 "), stopped\r\n",
               sourceName(r.source), (int)r.code, r.aux);
        break;
    case ReportKind::RuntimeLog:
        printf("[executorch] log message (level %c, line %d); further ones are counted in the stack report\r\n",
               (char)r.aux, (int)r.code);
        break;
    case ReportKind::Result:
        m_et_log_count = r.count;
        if (r.code < 0) {
            printf("[window %4" PRIu32 "] inference failed at step %d (error 0x%" PRIx32 ")\r\n",
                   r.index, (int)r.code, r.aux);
        } else if (r.value[0] == r.value[1]) {
            /* Sensor's and Inference's sums of the window agree: the handoff
             * delivered exactly what Sensor wrote. */
            printf("[window %4" PRIu32 "] logits = [%.6f, %.6f] -> %s | buf=%" PRIu32 " sum=%.6f\r\n",
                   r.index, (double)r.value[2], (double)r.value[3],
                   (r.code < 2) ? kLabels[r.code] : "?", r.buffer, (double)r.value[1]);
        } else {
            printf("[window %4" PRIu32 "] logits = [%.6f, %.6f] -> %s | buf=%" PRIu32
                   " HANDOFF MISMATCH: sensor sum=%.6f, inference sum=%.6f\r\n",
                   r.index, (double)r.value[2], (double)r.value[3],
                   (r.code < 2) ? kLabels[r.code] : "?", r.buffer,
                   (double)r.value[0], (double)r.value[1]);
        }
        break;
    case ReportKind::Stack:
        if (r.index == UINT32_MAX) {
            printf("%s: allocated = %" PRIu32 " bytes, HWM unavailable\r\n", sourceName(r.source), r.count);
        } else {
            printf("%s: %" PRIu32 "/%" PRIu32 " bytes used (%" PRIu32 " bytes unused headroom, %" PRIu32 " words HWM)\r\n",
                   sourceName(r.source), r.aux, r.count, r.count - r.aux, r.index);
        }
        /* MainApp's own line ends each stack report: add the ExecuTorch log
         * message count seen so far. */
        if (r.source == ReportSource::Main && m_et_log_count > 0) {
            printf("[executorch] log messages so far: %" PRIu32 "\r\n", m_et_log_count);
        }
        break;
    }
}
