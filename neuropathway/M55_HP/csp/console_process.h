#ifndef CONSOLE_PROCESS_H
#define CONSOLE_PROCESS_H

#include <csp/csp4cmsis.h>
#include "common_types.h"

/* Stack size: see sensor_process.h/csp4cmsis_neuropathway's console for
 * the 1024-word rationale (newlib printf stack cost) -- applies here
 * identically, since Console::run() is nothing but a printf loop. This
 * build's printf calls include %f (logits), unlike
 * csp4cmsis_neuropathway's Console -- confirmed the linked build
 * pulls in -u _printf_float (M55_HP.cproject.yml), same as every prior
 * ExecuTorch stage in this project. */
class Console : public csp::CSProcessStatic<1024> {
public:
    explicit Console(csp::Chanin<result_t> in);
    void run() override;
    const char* name() const override { return "Console"; }

private:
    csp::Chanin<result_t> m_result_in;
};

#endif /* CONSOLE_PROCESS_H */
