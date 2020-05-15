
#ifndef SYSTEM_H
#define SYSTEM_H

#ifdef __cplusplus
extern "C" {
#endif

extern volatile void* __lprs_gpu_base;
#define LPRS2_GPU_BASE __lprs_gpu_base

extern volatile void* __lprs_joypad_base;
#define LPRS2_JOYPAD_BASE __lprs_joypad_base

#ifdef __cplusplus
}
#endif

#endif // SYSTEM_H
