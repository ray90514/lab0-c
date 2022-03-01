/** dude, is my code constant time?
 *
 * This file measures the execution time of a given function many times with
 * different inputs and performs a Welch's t-test to determine if the function
 * runs in constant time or not. This is essentially leakage detection, and
 * not a timing attack.
 *
 * Notes:
 *
 *  - the execution time distribution tends to be skewed towards large
 *    timings, leading to a fat right tail. Most executions take little time,
 *    some of them take a lot. We try to speed up the test process by
 *    throwing away those measurements with large cycle count. (For example,
 *    those measurements could correspond to the execution being interrupted
 *    by the OS.) Setting a threshold value for this is not obvious; we just
 *    keep the x% percent fastest timings, and repeat for several values of x.
 *
 *  - the previous observation is highly heuristic. We also keep the uncropped
 *    measurement time and do a t-test on that.
 *
 *  - we also test for unequal variances (second order test), but this is
 *    probably redundant since we're doing as well a t-test on cropped
 *    measurements (non-linear transform)
 *
 *  - as long as any of the different test fails, the code will be deemed
 *    variable time.
 */

#include "fixture.h"
#include <assert.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../console.h"
#include "../random.h"
#include "constant.h"
#include "ttest.h"

#define enough_measure 10000
#define test_tries 10

extern const int drop_size;
extern const size_t chunk_size;
extern const size_t n_measure;
static t_ctx *t;

/* threshold values for Welch's t-test */
enum {
    t_threshold_bananas = 500, /* Test failed with overwhelming probability */
    t_threshold_moderate = 10, /* Test failed */
};

static void __attribute__((noreturn)) die(void)
{
    exit(111);
}

static void differentiate(int64_t *exec_times,
                          const int64_t *before_ticks,
                          const int64_t *after_ticks)
{
    for (size_t i = 0; i < n_measure; i++)
        exec_times[i] = after_ticks[i] - before_ticks[i];
}

static void update_statistics(const int64_t *exec_times, uint8_t *classes)
{
    for (size_t i = 0; i < n_measure; i++) {
        int64_t difference = exec_times[i];
        /* CPU cycle counter overflowed or dropped measurement */
        if (difference <= 0)
            continue;

        /* do a t-test on the execution time */
        t_push(t, difference, classes[i]);
    }
}

static bool report(void)
{
    double max_t = fabs(t_compute(t));
    double number_traces_max_t = t->n[0] + t->n[1];
    double max_tau = max_t / sqrt(number_traces_max_t);

    if (number_traces_max_t < enough_measure) {
        printf("meas: %7.2lf M, not enough measurements (%.0f still to go).\n",
               (number_traces_max_t / 1e6),
               enough_measure - number_traces_max_t);
        printf("\033[A\033[2K");
        return false;
    }

    /* max_t: the t statistic value
     * max_tau: a t value normalized by sqrt(number of measurements).
     *          this way we can compare max_tau taken with different
     *          number of measurements. This is sort of "distance
     *          between distributions", independent of number of
     *          measurements.
     * (5/tau)^2: how many measurements we would need to barely
     *            detect the leak, if present. "barely detect the
     *            leak" = have a t value greater than 5.
     */
    printf("\033[A\033[2K");
    printf("meas: %7.2lf M, max t: %+7.2f, max tau: %.2e, (5/tau)^2: %.2e.\n",
           (number_traces_max_t / 1e6), max_t, max_tau,
           (double) (5 * 5) / (double) (max_tau * max_tau));

    /* Definitely not constant time */
    if (max_t > t_threshold_bananas)
        return false;

    /* Probably not constant time. */
    if (max_t > t_threshold_moderate)
        return false;

    /* For the moment, maybe constant time. */
    return true;
}

static bool doit(int mode)
{
    int64_t *before_ticks = calloc(n_measure + 1, sizeof(int64_t));
    int64_t *after_ticks = calloc(n_measure + 1, sizeof(int64_t));
    int64_t *exec_times = calloc(n_measure, sizeof(int64_t));
    uint8_t *classes = calloc(n_measure, sizeof(uint8_t));
    uint8_t *input_data = calloc(n_measure * chunk_size, sizeof(uint8_t));

    if (!before_ticks || !after_ticks || !exec_times || !classes ||
        !input_data) {
        die();
    }

    prepare_inputs(input_data, classes);

    measure(before_ticks, after_ticks, input_data, mode);
    differentiate(exec_times, before_ticks, after_ticks);
    update_statistics(exec_times, classes);
    bool ret = report();

    free(before_ticks);
    free(after_ticks);
    free(exec_times);
    free(classes);
    free(input_data);

    return ret;
}

static void init_once(void)
{
    init_dut();
    t_init(t);
}

static bool TEST_CONST(char *text, int mode)
{
    bool result = false;
    t = malloc(sizeof(t_ctx));

    for (int cnt = 0; cnt < test_tries; ++cnt) {
        printf("Testing %s...(%d/%d)\n", text, cnt, test_tries);
        init_once();
        for (int i = 0; i < enough_measure / (n_measure - drop_size * 2) + 1;
             ++i)
            result = doit(mode);
        if (result == true)
            break;
    }
    free(t);
    return result;
}

bool is_insert_head_const(void)
{
    return TEST_CONST("insert_head", 0);
}

bool is_insert_tail_const(void)
{
    return TEST_CONST("insert_tail", 1);
}

bool is_remove_head_const(void)
{
    return TEST_CONST("remove_head", 2);
}

bool is_remove_tail_const(void)
{
    return TEST_CONST("remove_tail", 3);
}
