//
//  PlainGaussHelper.hpp
//  verifier
// 	solve gaussian elimination using the CPU, some parts will be reused during the GPU process.
//
//  Copyright © 2023 Airbus Commercial Aircraft
//  Created by Florian on 15.01.24.
//
#ifndef PlainGaussHelper_hpp
#define PlainGaussHelper_hpp

#include <stdio.h>
#include "../Matrix.hpp"
#include "../PITypes.hpp"

using namespace Matrix;

namespace PlainGaussHelper {
    std::vector<PI::XOR_TYPE> Solve(BMatrixType *M, std::vector<PI::XOR_TYPE> &v, bool debug = false);
    inline int Pivot(BMatrixType *M, int &currentRow, int &currentCol) {
        for (int i = currentRow; i < M->rows; ++i) {
            if ((*M)(i, currentCol)) {
                return i;
            }
        }
        return -1;
    }
    void ForwardReduction(BMatrixType *M, BMatrixType *I);
    std::vector<PI::XOR_TYPE> ApplyBookkeeping(BMatrixType &I, std::vector<PI::XOR_TYPE> &v);
    std::vector<PI::XOR_TYPE> BackSubstitution(BMatrixType &M, std::vector<PI::XOR_TYPE> &v);
    int RankOf(BMatrixType &m);
}

#endif /* PlainGaussHelper_hpp */
