//
//  GaussianElimination.hpp
//  verifier
//
//  Created by Florian on 21.11.23.
//

#ifndef GaussianElimination_hpp
#define GaussianElimination_hpp

#include <Metal/Metal.hpp>
#include <stdio.h>
#include "../Matrix.hpp"
#include "MetalFactory.hpp"
#include "../PITypes.hpp"

using namespace Matrix;

namespace Gauss {
    class GaussianElimination {
    public:
        GaussianElimination(BMatrixType *M, std::vector<PI::XOR_TYPE> v, bool debugPrints = false);
        
        MetalFactory *_factory;
        
        ~GaussianElimination();
        /*
         * Function: solve
         * solves the provided SLE, using metal.
         */
        std::vector<PI::XOR_TYPE> solve();
        void gaussForwardReduction();
        
    private:
        // Matrixes
        MTL::Buffer *_mBuffer; // Matrix m
        MTL::Buffer *_iBuffer; // identity Matrix for bookkeeping
        // MTL::Buffer *_resultBufer; // Result Matrix
        MTL::Buffer *_currentRowBuffer;
        MTL::Buffer *_currentColBuffer;
        MTL::Buffer *_currentBucketBuffer;
        MTL::Buffer *_rowsBuffer; // number of all rows of the matrix m
        MTL::Buffer *_bucketsBuffer; // number of all columns of  the matrix m
        MTL::Buffer *_bucketsIBuffer; // number of buckets of the identity matrix for bookkeeping
        MTL::Buffer *_indexResultBuffer; // the pivot index result
        MTL::Buffer *_cBuffer; // holding the vector c after the system is solved.
        MTL::Buffer *_xorsBuffer; // holding the result of the Bookkeeping
        MTL::Buffer *_vBuffer;
        
        int sendPartitialPivotComputeCommand(int currentRow, int currentCol);
        void sendXORComputeCommand(int &currentRow/*, int currentCol*/);
        std::vector<PI::XOR_TYPE> sendBookkeepingCommand();
        void prepareData();
        //void swapRows(MatrixType &m, int l, int k, BMatrixType &I);
        void printMatrixes();
        void println(std::string s);
        BMatrixType _m;
        BMatrixType *I;
        std::vector<PI::XOR_TYPE> _v;
        bool _debug;
        /*
         This method works only if the matrix m is in upper triangle form.
         */
        int RankOf(Matrix::BMatrixType &m);
    };
}
#endif /* GaussianElimination_hpp */
