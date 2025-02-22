﻿/* Copyright (c) 2022 AntGroup. All Rights Reserved. */

#include <vector>

#include "fma-common/configuration.h"
#include "fma-common/local_file_stream.h"
#include "fma-common/logging.h"
#include "fma-common/snappy_stream.h"
#include "fma-common/unit_test_utils.h"
#include "fma-common/utils.h"

using namespace fma_common;

FMA_UNIT_TEST(SnappyStream) {
    ArgParser parser;
    parser.Add<int>("blockSize,b")
        .Comment("snappy block size to use")
        .SetDefault(1024 * 1024 * 128);
    parser.Add<int>("nBlocks,n").Comment("number of blocks to write").SetDefault(1);
    parser.Add<int>("nBuffers,f").Comment("number of buffers to use").SetDefault(1);
    parser.Add("file").SetDefault("snappy.out").Comment("file to use");
    parser.Parse(argc, argv);
    parser.Finalize();

    int block_size = parser.GetValue<int>("blockSize");
    block_size = block_size / 4 * 4;
    int n_blocks = parser.GetValue<int>("nBlocks");
    // int n_buffers = parser.GetValue<int>("nBuffers");
    std::string path = parser.GetValue("file");

    int64_t sum = 0;
    std::vector<char> block(block_size);
    int *p = (int *)(&block[0]);
    static unsigned int seed = 0;
    for (int i = 0; i < block_size / 4; i++) {
        int d = rand_r(&seed) % 255;
        p[i] = d;
        sum += d;
    }
    sum *= n_blocks;

    double t1 = GetTime();
#if ENABLE_SNAPPY
    SnappyOutputStream out(path, n_buffers, block_size, std::ofstream::trunc);
#else
    OutputLocalFileStream out(path, std::ofstream::trunc);
#endif
    for (int i = 0; i < n_blocks; i++) {
        out.Write(&block[0], block_size);
    }
    out.Close();
    double t2 = GetTime();
    LOG() << "write: " << (double)n_blocks * block_size / 1024 / 1024 / (t2 - t1) << "MB/s";

    t1 = GetTime();
#if ENABLE_SNAPPY
    SnappyInputStream in(path, n_buffers);
#else
    InputLocalFileStream in(path);
#endif
    std::string buf;
    int64_t sum2 = 0;
    buf.resize(block_size);
    while (in.Read(&buf[0], block_size)) {
        size_t n_elements = buf.size() / 4;
        int *p = (int *)buf.data();
        for (size_t i = 0; i < n_elements; i++) {
            sum2 += p[i];
        }
    }
    CHECK_EQ(sum, sum2);
    t2 = GetTime();
    LOG() << "read: " << (double)n_blocks * block_size / 1024 / 1024 / (t2 - t1) << "MB/s";
    return 0;
}
