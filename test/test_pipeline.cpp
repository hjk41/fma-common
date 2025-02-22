﻿/* Copyright (c) 2022 AntGroup. All Rights Reserved. */

#include <math.h>
#include <future>
#include <thread>

#include "fma-common/configuration.h"
#include "fma-common/logging.h"
#include "fma-common/pipeline.h"
#include "fma-common/utils.h"
#include "fma-common/unit_test_utils.h"

using namespace fma_common;

static double sleep_time = 0.1;

std::pair<int, double> AddOne(int d) {
    FMA_DBG() << "1i: " << d;
    SleepS(sleep_time);
    return std::make_pair(d, (double)d + 1);
}

std::pair<int, double> Square(const std::pair<int, double> &d) {
    FMA_DBG() << "2i: " << d.first;
    static unsigned int seed = 0;
    SleepS((double)((rand_r(&seed) % 10) + 1) / 5 * sleep_time);
    return std::make_pair(d.first, d.second * d.second);
}

std::pair<int, int> SqrtMinusOne(const std::pair<int, double> &d) {
    FMA_DBG() << "3i: " << d.first;
    SleepS(sleep_time);
    return std::make_pair(d.first, (int)(sqrt(d.second) - 1));
}

FMA_SET_TEST_PARAMS(Pipeline, "", "--nTasks 100 --sleepTime 0.05");

FMA_UNIT_TEST(Pipeline) {
    Logger::Get().SetFormatter(std::make_shared<TimedLogFormatter>());
    Logger::Get().SetLevel(LogLevel::LL_DEBUG);

    int n_tasks = 10;
    Configuration config;
    config.Add(sleep_time, "sleepTime", true).Comment("Time to sleep for each task");
    config.Add(n_tasks, "nTasks", true).Comment("Number of tasks");
    config.Parse(argc, argv);
    config.Finalize();

    ThreadPool tp(13);

    PipelineStage<int, std::pair<int, double>> p1(AddOne, &tp, 2, 8, 8);
    PipelineStage<std::pair<int, double>, std::pair<int, double>> p2(Square, &tp, 1, 3, 3);
    PipelineStage<std::pair<int, double>, std::pair<int, int>> p3(SqrtMinusOne, &tp, 0, 8, 8);
    BoundedQueue<std::pair<int, int>> results;
    p1.SetNextStage(&p2);
    p2.SetNextStage(&p3);
    p3.SetNextStage(&results);

    double t1 = GetTime();

    for (int i = 0; i < n_tasks; i++) {
        p1.Push(i);
    }

    for (int i = 0; i < n_tasks; i++) {
        std::pair<int, int> r;
        bool s = results.Pop(r);
        FMA_ASSERT(s && r.second == i)
            << "Error poping the " << i << "th result: returned" << s << " and popped " << r.second;
    }

    p1.WaitTillClear();
    p2.WaitTillClear();
    p3.WaitTillClear();
    double t2 = GetTime();
    double et = t2 - t1;
    FMA_LOG() << "cost: " << et;
    // FMA_ASSERT(et >= sleep_time * ((double)((n_tasks + 4) / 5) * 2 +
    //    (double)0.2 * (n_tasks + 2)/ 3))
    //    << "Total time is too small: " << et;
    // FMA_ASSERT(et <= sleep_time * ((double)((n_tasks + 4) / 5) * 2 +
    //    (double)(n_tasks + 2) / 3) + sleep_time)
    //    << "Total time is too small: " << et;
    return 0;
}
