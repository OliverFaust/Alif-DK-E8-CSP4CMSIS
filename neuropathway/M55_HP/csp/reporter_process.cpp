#include "reporter_process.h"

#include "cmsis_os2.h"

#include <cstdio>
#include <cinttypes>

using namespace csp;

/* The processes run forever, so there is no natural "network finished"
 * point to report stack usage at -- report periodically instead. This is a
 * live worst-observed-so-far reading, trustworthy only once each process's
 * deepest call path has been exercised. */
#ifndef CSP_STACK_REPORT_INTERVAL_MS
#define CSP_STACK_REPORT_INTERVAL_MS (10000)
#endif

static const char *kLabels[2] = {"WALKING", "LAYING"};

static const char* sourceName(ReportSource s)
{
    switch (s) {
    case ReportSource::Sensor:    return "Sensor";
    case ReportSource::Inference: return "Inference";
    }
    return "?";
}

Reporter::Reporter(Chanin<report_t> in, CSProcess* const* others, size_t other_count)
    : m_in(in), m_others(others), m_other_count(other_count)
{
}

void Reporter::run()
{
    const uint32_t interval = (uint32_t)((uint64_t)CSP_STACK_REPORT_INTERVAL_MS * osKernelGetTickFreq() / 1000U);
    uint32_t last_stack_report = osKernelGetTickCount();

    while (true) {
        report_t r;
        m_in.read(r);
        print(r);

        /* Printed here, between two reports, so that it cannot interleave
         * with anything: no other process prints. */
        uint32_t now = osKernelGetTickCount();
        if (now - last_stack_report >= interval) {
            last_stack_report = now;
            printStackReport();
        }
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
    }
}

static void printStack(CSProcess& p)
{
    size_t allocated_bytes = p.stackWords() * sizeof(uint32_t);
    uint32_t hwm = p.stackHighWaterMarkWords();
    if (hwm == CSP_STACK_HWM_UNAVAILABLE) {
        printf("%s: allocated = %u bytes, HWM unavailable\r\n", p.name(), (unsigned)allocated_bytes);
        return;
    }
    size_t unused_bytes = hwm * sizeof(uint32_t);
    size_t used_bytes = (unused_bytes <= allocated_bytes) ? allocated_bytes - unused_bytes : 0;
    printf("%s: %u/%u bytes used (%u bytes unused headroom, %u words HWM)\r\n",
           p.name(), (unsigned)used_bytes, (unsigned)allocated_bytes,
           (unsigned)unused_bytes, (unsigned)hwm);
}

void Reporter::printStackReport()
{
    for (size_t i = 0; i < m_other_count; i++) {
        printStack(*m_others[i]);
    }
    printStack(*this);
    if (m_et_log_count > 0) {
        printf("[executorch] log messages so far: %" PRIu32 "\r\n", m_et_log_count);
    }
}
