#pragma once

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifdef DISABLE_NANOPROFILER

static inline void NanoprofilerBegin(int thread_id, const char * label) {}
static inline void NanoprofilerEnd(int thread_id, const char * label) {}
static inline void NanoprofilerAllocate(size_t samples_count_per_thread) {}
static inline void NanoprofilerOutputAndFree(int thread_id) {}

#else // DISABLE_NANOPROFILER

void NanoprofilerBegin(int thread_id, const char * label);
void NanoprofilerEnd(int thread_id, const char * label);
void NanoprofilerAllocate(size_t samples_count_per_thread);
void NanoprofilerOutputAndFree(int thread_id);

#endif // DISABLE_NANOPROFILER

#ifdef __cplusplus
}
#endif
