//
//  GaussBenchmark.hpp
//  gauss-benchmark
//
//  Copyright © 2023 Airbus Commercial Aircraft
//  Created by Florian on 19.01.24.
//

#ifndef GaussBenchmark_h
#define GaussBenchmark_h

struct BenchmarkContext {
    std::vector<int> bucketSizes;
    int min;
    int max;
    bool debugPrints;
};


#endif /* GaussBenchmark_h */
