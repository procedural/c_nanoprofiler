#include "nanoprofiler.h"

#ifndef DISABLE_NANOPROFILER_HEADER_IMPORT
#ifdef _WIN32
#include <windows.h>
#else
#include <time.h>
#endif
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#endif // DISABLE_NANOPROFILER_HEADER_IMPORT

#ifndef NANOPROFILER_MAX_NUM_OF_THREADS
#define NANOPROFILER_MAX_NUM_OF_THREADS 1
#endif

#ifndef NANOPROFILER_OUTPUT_ASCII_STRING
#if defined(__linux__)
#define NANOPROFILER_OUTPUT_ASCII_STRING(ascii_string) fprintf(stderr, "%s", ascii_string)
#elif defined(_WIN32)
#define NANOPROFILER_OUTPUT_ASCII_STRING(ascii_string) OutputDebugStringA(ascii_string)
#endif
#endif

#if defined(__linux__)
#define __NANOPROFILER_LONG long
#elif defined(_WIN32)
#define __NANOPROFILER_LONG long long
#endif

struct __NANOPROFILER_SAMPLE {
#if defined(__linux__)
  time_t        seconds;
  long          nanoseconds;
#elif defined(_WIN32)
  LARGE_INTEGER counter;
#endif
  const char *  label;
  char          scope;
};

struct __NANOPROFILER_SAMPLES {
  struct __NANOPROFILER_SAMPLE * samples;
  int                            current_sample_index;
};

struct __NANOPROFILER_SAMPLES __g_nanoprofiler_samples[NANOPROFILER_MAX_NUM_OF_THREADS] = {0};
struct __NANOPROFILER_SAMPLE  __g_nanoprofiler_initial_sample                           = {0};
size_t                        __g_nanoprofiler_samples_count_per_thread                 = 0;
#if defined(_WIN32)
LARGE_INTEGER                 __g_nanoprofiler_performance_frequency                    = {0};
#endif

void NanoprofilerBegin(int thread_id, const char * label) {
  int i = __g_nanoprofiler_samples[thread_id].current_sample_index;
  __g_nanoprofiler_samples[thread_id].current_sample_index += 1;
  struct __NANOPROFILER_SAMPLE * s = &__g_nanoprofiler_samples[thread_id].samples[i];
#if defined(__linux__)
  struct timespec timespec;
  clock_gettime(CLOCK_REALTIME, &timespec);
  s->seconds     = timespec.tv_sec;
  s->nanoseconds = timespec.tv_nsec;
#elif defined(_WIN32)
  QueryPerformanceCounter(&s->counter);
#endif
  s->label = label;
  s->scope = 'B';
}

void NanoprofilerEnd(int thread_id, const char * label) {
  int i = __g_nanoprofiler_samples[thread_id].current_sample_index;
  __g_nanoprofiler_samples[thread_id].current_sample_index += 1;
  struct __NANOPROFILER_SAMPLE * s = &__g_nanoprofiler_samples[thread_id].samples[i];
#if defined(__linux__)
  struct timespec timespec;
  clock_gettime(CLOCK_REALTIME, &timespec);
  s->seconds     = timespec.tv_sec;
  s->nanoseconds = timespec.tv_nsec;
#elif defined(_WIN32)
  QueryPerformanceCounter(&s->counter);
#endif
  s->label = label;
  s->scope = 'E';
}

void NanoprofilerAllocate(size_t samples_count_per_thread) {
#if defined(__linux__)
  struct timespec timespec;
  clock_gettime(CLOCK_REALTIME, &timespec);
  __g_nanoprofiler_initial_sample.seconds     = timespec.tv_sec;
  __g_nanoprofiler_initial_sample.nanoseconds = timespec.tv_nsec;
#elif defined(_WIN32)
  BOOL query_returned_error = QueryPerformanceFrequency(&__g_nanoprofiler_performance_frequency);
  assert(query_returned_error != 0);
  QueryPerformanceCounter(&__g_nanoprofiler_initial_sample.counter);
#endif
  __g_nanoprofiler_samples_count_per_thread = samples_count_per_thread;
  for (int t = 0; t < NANOPROFILER_MAX_NUM_OF_THREADS; t += 1) {
    __g_nanoprofiler_samples[t].samples = (struct __NANOPROFILER_SAMPLE *)calloc(samples_count_per_thread, sizeof(struct __NANOPROFILER_SAMPLE));
  }
}

static inline double __NanoprofilerSecondsAndNanosecondsToMicrosecondsWithDecimalNanoseconds(__NANOPROFILER_LONG seconds, __NANOPROFILER_LONG nanoseconds) {
  __NANOPROFILER_LONG seconds_as_milliseconds = seconds                 * 1000;
  __NANOPROFILER_LONG seconds_as_microseconds = seconds_as_milliseconds * 1000;
  __NANOPROFILER_LONG seconds_as_nanoseconds  = seconds_as_microseconds * 1000;

  __NANOPROFILER_LONG summed_seconds_as_nanoseconds_and_nanoseconds = seconds_as_nanoseconds + nanoseconds;
  double microseconds_with_decimal_nanoseconds = (double)summed_seconds_as_nanoseconds_and_nanoseconds / 1000.0;

  return microseconds_with_decimal_nanoseconds;
}

void NanoprofilerOutputAndFree(int thread_id) {
#if 1
  {
#if defined(__linux__)
    struct timespec self_begin_timespec;
    clock_gettime(CLOCK_REALTIME, &self_begin_timespec);
    long self_begin_seconds     = self_begin_timespec.tv_sec;
    long self_begin_nanoseconds = self_begin_timespec.tv_nsec;
#elif defined(_WIN32)
    LARGE_INTEGER self_begin_counter = {0};
    QueryPerformanceCounter(&self_begin_counter);
#endif

    char line[100000] = {0};

#if defined(__linux__)
    __NANOPROFILER_LONG initial_seconds     = __g_nanoprofiler_initial_sample.seconds;
    __NANOPROFILER_LONG initial_nanoseconds = __g_nanoprofiler_initial_sample.nanoseconds;
#elif defined(_WIN32)
    __NANOPROFILER_LONG initial_seconds     = (__g_nanoprofiler_initial_sample.counter.QuadPart / __g_nanoprofiler_performance_frequency.QuadPart);
    __NANOPROFILER_LONG initial_nanoseconds = (__g_nanoprofiler_initial_sample.counter.QuadPart % __g_nanoprofiler_performance_frequency.QuadPart) * 1000000000 / __g_nanoprofiler_performance_frequency.QuadPart;
#endif

    snprintf(line, 100000, "{\"traceEvents\":[{}\n");
    NANOPROFILER_OUTPUT_ASCII_STRING(line);

    for (int t = 0; t < NANOPROFILER_MAX_NUM_OF_THREADS; t += 1) {
      for (size_t i = 0; i < __g_nanoprofiler_samples_count_per_thread; i += 1) {
        struct __NANOPROFILER_SAMPLE * s = &__g_nanoprofiler_samples[t].samples[i];
        if (s->label == NULL) {
          break;
        }

#if defined(__linux__)
        __NANOPROFILER_LONG seconds     = s->seconds;
        __NANOPROFILER_LONG nanoseconds = s->nanoseconds;
#elif defined (_WIN32)
        __NANOPROFILER_LONG seconds     = (s->counter.QuadPart / __g_nanoprofiler_performance_frequency.QuadPart);
        __NANOPROFILER_LONG nanoseconds = (s->counter.QuadPart % __g_nanoprofiler_performance_frequency.QuadPart) * 1000000000 / __g_nanoprofiler_performance_frequency.QuadPart;
#endif
        double time = __NanoprofilerSecondsAndNanosecondsToMicrosecondsWithDecimalNanoseconds(seconds - initial_seconds, nanoseconds);

        snprintf(line, 100000,
          ",{\"ph\":\"%c\",\"ts\":%.3lf,\"pid\":0,\"tid\":%d,\"name\":\"%s\"}\n",
          s->scope,
          time,
          t, s->label
        );
        NANOPROFILER_OUTPUT_ASCII_STRING(line);
      }
    }

#if defined(__linux__)
    struct timespec self_end_timespec;
    clock_gettime(CLOCK_REALTIME, &self_end_timespec);
    long self_end_seconds     = self_end_timespec.tv_sec;
    long self_end_nanoseconds = self_end_timespec.tv_nsec;
#elif defined(_WIN32)
    LARGE_INTEGER self_end_counter = {0};
    QueryPerformanceCounter(&self_end_counter);
#endif

    // Self time profiling begin.
    {
#if defined(__linux__)
      __NANOPROFILER_LONG seconds     = self_begin_seconds;
      __NANOPROFILER_LONG nanoseconds = self_begin_nanoseconds;
#elif defined (_WIN32)
      __NANOPROFILER_LONG seconds     = (self_begin_counter.QuadPart / __g_nanoprofiler_performance_frequency.QuadPart);
      __NANOPROFILER_LONG nanoseconds = (self_begin_counter.QuadPart % __g_nanoprofiler_performance_frequency.QuadPart) * 1000000000 / __g_nanoprofiler_performance_frequency.QuadPart;
#endif
      double time = __NanoprofilerSecondsAndNanosecondsToMicrosecondsWithDecimalNanoseconds(seconds - initial_seconds, nanoseconds);

      snprintf(line, 100000,
        ",{\"ph\":\"%c\",\"ts\":%.3lf,\"pid\":0,\"tid\":%d,\"name\":\"%s\"}\n",
        'B',
        time,
        thread_id, "NanoprofilerOutputAndFree"
      );
      NANOPROFILER_OUTPUT_ASCII_STRING(line);
    }

    // Self time profiling end.
    {
#if defined(__linux__)
      __NANOPROFILER_LONG seconds     = self_end_seconds;
      __NANOPROFILER_LONG nanoseconds = self_end_nanoseconds;
#elif defined (_WIN32)
      __NANOPROFILER_LONG seconds     = (self_end_counter.QuadPart / __g_nanoprofiler_performance_frequency.QuadPart);
      __NANOPROFILER_LONG nanoseconds = (self_end_counter.QuadPart % __g_nanoprofiler_performance_frequency.QuadPart) * 1000000000 / __g_nanoprofiler_performance_frequency.QuadPart;
#endif
      double time = __NanoprofilerSecondsAndNanosecondsToMicrosecondsWithDecimalNanoseconds(seconds - initial_seconds, nanoseconds);

      snprintf(line, 100000,
        ",{\"ph\":\"%c\",\"ts\":%.3lf,\"pid\":0,\"tid\":%d,\"name\":\"%s\"}\n",
        'E',
        time,
        thread_id, "NanoprofilerOutputAndFree"
      );
      NANOPROFILER_OUTPUT_ASCII_STRING(line);
    }

    snprintf(line, 100000, "],\"displayTimeUnit\":\"ns\"}\n");
    NANOPROFILER_OUTPUT_ASCII_STRING(line);
  }
#endif

  for (int t = 0; t < NANOPROFILER_MAX_NUM_OF_THREADS; t += 1) {
    free(__g_nanoprofiler_samples[t].samples);
  }
}
