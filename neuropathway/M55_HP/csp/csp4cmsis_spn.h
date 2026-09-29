#ifndef CSP4CMSIS_SPN_H
#define CSP4CMSIS_SPN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Builds and runs the Sensor/Inference/Console network (see
 * csp4cmsis_spn.cpp), then returns -- called once from main.cpp between
 * osKernelInitialize() and osKernelStart(), matching csp4cmsis_alt_test/
 * csp4cmsis_neuropathway's own RunProcessingChainTest() entry point. */
void RunProcessingChainTest(void);

#ifdef __cplusplus
}
#endif

#endif /* CSP4CMSIS_SPN_H */
