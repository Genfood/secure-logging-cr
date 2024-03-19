//
//  main.cpp
//  gauss-benchmark
//
//  Copyright © 2023 Airbus Commercial Aircraft
//  Created by Florian on 29.09.23.
//

#define NS_PRIVATE_IMPLEMENTATION
#define CA_PRIVATE_IMPLEMENTATION
#define MTL_PRIVATE_IMPLEMENTATION

#include <sstream>
#include <iostream>
#include <algorithm>
#include <chrono>
#include <math.h>
#include "MetalGauss.hpp"
#include "RandomStuff.hpp"
#include "Matrix.hpp"
#include "GaussBenchmark.hpp"

#define K 5
#define C 1.1244

using std::chrono::high_resolution_clock;
using std::chrono::duration_cast;
using std::chrono::duration;
using std::chrono::milliseconds;
using std::string;

BenchmarkContext parseArguments(int argc, const char* argv[]);
std::vector<int> parseBucketSizes(const std::string& bucketSizeStr);

string printTime(high_resolution_clock::time_point t1, high_resolution_clock::time_point t2, int fac, bool debug);
string runTests(bool isVec, MTL::Device *device, int bits, int testStart, int testEnd, bool debug);
template<typename T>
string test(bool isVec, MTL::Device *device, int bits, std::string testName, std::function<void(Matrix<T>, MetalGauss<T> *)> solve, int testStart, int testEnd, bool debug);



string printTime(high_resolution_clock::time_point t1, high_resolution_clock::time_point t2, int fac, bool debug) {
    duration<long, std::nano> ns_double = t2 - t1;
    auto s = ceil(ns_double.count()/1'000'000'000.0);
    if (debug)
        std::cout << "Matrix size of 2^" << fac << ": " << s << "s ("<< ns_double.count() << "ns)"  << std::endl;
    std::ostringstream ss;
    
    ss << "\"2^" << fac << "\": " << ns_double.count();
    return ss.str();
}

template<typename T>
string test(bool isVec, MTL::Device *device, int bits, std::string testName, std::function<void(Matrix<T>, MetalGauss<T> *)> solve, int testStart, int testEnd, bool debug) {
    if (debug) {
        std::cout << testName << std::endl;
        std::cout << "----------------------------------------------------------" << std::endl;
    }
    std::ostringstream ss;
    
    ss << "\"" << testName << "\": {";
    for (int t = testStart; t <= testEnd; ++t) {
        unsigned int size = 1<<t;
        
        Matrix<T> m = CreateRandomMatrix<T>(ceil(size * C), size, K, bits);
        auto *gauss = new MetalGauss<T>(device, &m, false);
        
        
        auto t1 = high_resolution_clock::now();
        // solve matrix
        solve(m, gauss);
        
        auto t2 = high_resolution_clock::now();
        
        string timePart = printTime(t1,t2, t, debug);
        ss << timePart;
        if (debug)
            std::cout << timePart<<std::endl;
        if (t != testEnd) {
            ss << ", ";
        }
        delete gauss;
    }
    ss << "}";
    
    if (debug)
        std::cout << "----------------------------------------------------------" << std::endl;
    
    return ss.str();
}

template<typename T>
string runTests(bool isVec, MTL::Device *device, int bits, int testStart, int testEnd, bool debug) {
    std::ostringstream ss;
    
    ss << "{";
    ss << test(isVec, device, bits, "metal", (std::function<void(Matrix<T>, MetalGauss<T>*)>)([](Matrix<T> m, MetalGauss<T> *gauss) -> void {
        gauss->solve();
    }), testStart, testEnd, debug);
    
    ss << ", ";// \"no metal\": ";
    ss << test(isVec, device, bits, "no Metal", (std::function<void(Matrix<T>, MetalGauss<T>*)>)([](Matrix<T> m, MetalGauss<T> *gauss) -> void {
        MetalGauss<T>::solve_withoutMetal(&m);
    }), testStart, testEnd, debug);
    
    ss << ", ";//\"no pivot\": ";
    ss << test(isVec, device, bits, "metal no pivot", (std::function<void(Matrix<T>, MetalGauss<T>*)>)([](Matrix<T> m, MetalGauss<T> *gauss) -> void {
        gauss->solve_noParallelPivoting();
    }), testStart, testEnd, debug);
    
    ss << "}";
    return ss.str();
}

void printNewTest(std::string s, bool debug) {
    if (!debug) return;
    
    std::cout << std::endl << "----------------------------------------------------------" << std::endl;
    std::cout << "----------------------------------------------------------" << std::endl;
    std::cout << s << std::endl;
    std::cout << "----------------------------------------------------------" << std::endl;
    std::cout << "----------------------------------------------------------" << std::endl << std::endl;
}

std::vector<int> parseBucketSizes(const std::string& bucketSizeStr) {
    std::vector<int> bucketSizes;
    std::stringstream ss(bucketSizeStr);
    std::string size;
    
    while (std::getline(ss, size, ',')) {
        try {
            // Convert string to integer
            int bucketSize = std::stoi(size);
            bucketSizes.push_back(bucketSize);
        } catch (const std::exception& e) {
            // Handle any conversion errors
            std::cerr << "Error parsing bucket size: " << e.what() << "\n";
            exit(EXIT_FAILURE);
        }
    }
    
    return bucketSizes;
}


BenchmarkContext parseArguments(int argc, const char* argv[]) {
    BenchmarkContext ctx = {{}, -1, -1, false};
    bool minSet = false, maxSet = false, bucketSizeSet = false;

    for (int i = 1; i < argc; ++i) {
        std::string arg(argv[i]);

        if (((arg == "-min") || (arg == "--min")) && i + 1 < argc) {
            ctx.min = std::atoi(argv[++i]);
            minSet = true;
        } else if (((arg == "-max") || (arg == "--max")) && i + 1 < argc) {
            ctx.max = std::atoi(argv[++i]);
            maxSet = true;
        } else if (((arg == "-bs") || (arg == "--bucket-size")) && i + 1 < argc) {
            ctx.bucketSizes = parseBucketSizes(argv[++i]);
            bucketSizeSet = true;
        } else if ((arg == "-d") || (arg == "--debug-prints")) {
            ctx.debugPrints = true;
        }
    }
    
    if (!minSet || !maxSet || !bucketSizeSet) {
        std::cerr << "Missing required arguments." << std::endl;
        std::cerr << "Usage: " << argv[0] << " [-min|--min] <min_value> "
                          << "[-max|--max] <max_value> "
                          << "[-bs|--bucket-size] <bucket_size1,bucket_size2,...> "
                          << "[-d|--debug-prints]" << std::endl;
        exit(EXIT_FAILURE); // Terminate the program
    }

    std::vector<int> validBucketSizes = {1, 32, 64, 256};
    for (int size : ctx.bucketSizes) {
        if (std::find(validBucketSizes.begin(), validBucketSizes.end(), size) == validBucketSizes.end()) {
            std::cerr << "Invalid bucket size " << size << ". Allowed values are 1, 32, 64, 256.\n";
            exit(EXIT_FAILURE);
        }
    }
    
    return ctx;
}

int main(int argc, const char * argv[])
{
    std::ostringstream ss;
    BenchmarkContext ctx = parseArguments(argc, argv);
    std::cout << "INFO: Start running benchmarks, in the range from 2^"<< ctx.min;
    std::cout << " - 2^" << ctx.max << ", this could take a while!";
    if (!ctx.debugPrints)
        std::cout << " No debug statements will be printed.";
    std::cout << std::endl;
    
    // get GPU
    MTL::Device *device = MTL::CreateSystemDefaultDevice();
    ss << "{";
    
    for (int i = 0; i < ctx.bucketSizes.size(); ++i) {
        auto bucketSize = ctx.bucketSizes[i];
        
        switch (bucketSize) {
            case 1:
                printNewTest("Running tests with one int holding, exactly 1 value:", ctx.debugPrints);
                ss << "\"1bit\": ";
                ss << runTests<B1>(false, device, 1, ctx.min, ctx.max, ctx.debugPrints);
                break;
            case 32:
                printNewTest("Running tests with one integer holding, exactly 32 values:", ctx.debugPrints);
                ss << "\"32bit\": ";
                ss << runTests<B32>(false, device, 32, ctx.min, ctx.max, ctx.debugPrints);
                
                break;
            case 64:
                printNewTest("Running tests with one vector of 1 long holding, exactly 64 values:", ctx.debugPrints);
                ss << "\"64bit\": ";
                ss << runTests<B64>(false, device, 64, ctx.min, ctx.max, ctx.debugPrints);
                
                break;
            case 256:
                printNewTest("Running tests with one vector of 4 longs holding, exactly 256 values:", ctx.debugPrints);
                ss << "\"256bit\": ";
                ss << runTests<B256>(true, device, 256, ctx.min, ctx.max, ctx.debugPrints);
                
                break;
            default:
                std::cerr << "Invalid bucket size " << bucketSize << ". Allowed values are 1, 32, 64, 256.\n";
                exit(EXIT_FAILURE);
                break;
        }
        
        if (i < (ctx.bucketSizes.size() -1))
            ss << ", ";
    }
    
    ss << "}";
    
    std::cout << ss.str() << std::endl;
}


