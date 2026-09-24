#include "console_process.h"

#include <cstdio>
#include <cinttypes>

static const char *kLabels[2] = {"WALKING", "LAYING"};

Console::Console(csp::Chanin<result_t> in) : m_result_in(in) {}

void Console::run()
{
    while (true) {
        result_t res;
        m_result_in.read(res);

        const char* pred_name =
            (res.predicted >= 0 && res.predicted < 2) ? kLabels[res.predicted] : "?";

        printf("[window %4" PRIu32 "] logits = [%.6f, %.6f] -> %s\r\n",
               res.index, (double)res.logits[0], (double)res.logits[1], pred_name);
    }
}
