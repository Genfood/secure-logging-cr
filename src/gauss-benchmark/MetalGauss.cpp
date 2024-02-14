//
//  MetalGauss.cpp
//  gauss-benchmark
//
//  Created by Florian on 29.09.23.
//

#include "MetalGauss.hpp"
//#define IS_ONE(var,pos) ((var) & (1ul<<(pos)))

//template<typename T>
//Matrix<T> *_m; // this matrix holds the pointer of the shared buffer



template<typename T>
MetalGauss<T>::MetalGauss(MTL::Device *device, Matrix<T> *m, bool debugPrints)
    : _m(m), _factory(new MetalFactory(device)), _debug(debugPrints)
{
    // Create all necessary buffers, fill them with data
    prepareData();
    
    // set pointer of the shared matrix buffer:
    _m->SetCustomDataPointer((T*)_mBuffer->contents());
    
    
    switch (_m->bits) {
        case 1:
            pso_xor = _factory->PSO_xor_1;
            pso_pivot = _factory->PSO_pivot_1;
            break;
            
        case 32:
            pso_xor = _factory->PSO_xor_32;
            pso_pivot = _factory->PSO_pivot_32;
            break;
        case 64:
            pso_xor = _factory->PSO_xor_64;
            pso_pivot = _factory->PSO_pivot_64;
            break;
        case 256:
            pso_xor = _factory->PSO_xor_256;
            pso_pivot = _factory->PSO_pivot_256;
            break;
        default:
            std::cerr << "invalid BITs" << std::endl;
            exit(EXIT_FAILURE);
            break;
    }
    
}



template<typename T>
void MetalGauss<T>::prepareData()
{
    size_t mSize = _m->rows * _m->buckets * sizeof(T);
    
    // Buffer that can be pre filled:
    _mBuffer            = _factory->newBuffer((T*)_m->data, mSize);
    _rowsBuffer         = _factory->newBuffer(&_m->rows, sizeof(int32_t));
    _bucketsBuffer      = _factory->newBuffer(&_m->buckets, sizeof(int32_t));
    //_resultBufer        = _factory->newBuffer(mSize);
    
    // Buffer that cant be pre filled, because they change for every call:
    _currentColBuffer   = _factory->newBuffer(sizeof(int32_t));
    _currentBucketBuffer= _factory->newBuffer(sizeof(int32_t));
    _currentRowBuffer   = _factory->newBuffer(sizeof(int32_t));
    _indexResultBuffer  = _factory->newBuffer(sizeof(int32_t));
}

template<typename T>
void MetalGauss<T>::swapRows(Matrix<T> &m, int &l, int &k)
{
    if (l >= m.rows || k >= m.rows || l == k)
    {
        throw std::out_of_range("Invalid row indices for swapping");
    }
    
    
    // Calculate the starting indices for the two rows
    
    size_t start_l = l * m.buckets;
    size_t start_k = k * m.buckets;
    // Bounds checking for start_l and start_k
    if (start_l >= m.rows * m.buckets || start_k >= m.rows * m.buckets) {
        throw std::out_of_range("Invalid row start indices for swapping");
    }
    
    
    // Swap the rows by swapping the corresponding elements in the data vector
    for (size_t i = 0; i < m.buckets; ++i) {
        
        
        std::swap(m.data[start_l + i], m.data[start_k + i]);
    }
}

template<typename T>
int MetalGauss<T>::sendPartitialPivotComputeCommand(int &currentRow) {
    return sendPartitialPivotComputeCommand(pso_pivot, currentRow);
}

template<typename T>
void MetalGauss<T>::sendXORComputeCommand(int &currentRow) {
    sendXORComputeCommand(pso_xor, currentRow);
}

template<typename T>
void MetalGauss<T>::sendXORComputeCommand(MTL::ComputePipelineState *pso, int &currentRow)
{
    // +1 because we dont need to do something with the current row, no xor here.
    int size = _m->rows - (currentRow +1);
    
    // TODO: at a specific threshold a diffrent solution might be better.
    if (1 > size) {
        return;
    }
    
    MTL::Buffer *buffers [] = {
        _mBuffer,
        _currentRowBuffer,
        _currentColBuffer,
        _currentBucketBuffer,
        _bucketsBuffer
    };
    
    
    _factory->sendCommand(pso, buffers, 5, size);
}

template<typename T>
int MetalGauss<T>::sendPartitialPivotComputeCommand(MTL::ComputePipelineState *pso, int &currentRow) {
    int size = _m->rows - (currentRow +1);
    
    // TODO: at a specific threshold a diffrent solution might be better.
    if (1 > size) {
        return currentRow;
    }
    
    //MetalFactory::setBufferWithUInt32(_currentRowBuffer, currentRow);
    //MetalFactory::setBufferWithUInt32(_currentColBuffer, currentCol);
    MetalFactory::setBufferWithUInt32(_indexResultBuffer, currentRow); // set as default the current row
    
    MTL::Buffer *buffers [] = {
        _mBuffer,
        _currentRowBuffer,
        _currentColBuffer,
        _bucketsBuffer,
        _indexResultBuffer
    };
    
    
    _factory->sendCommand(pso, buffers, 5, size);
    
    return *(int*)_indexResultBuffer->contents();
}

template<typename T>
void MetalGauss<T>::gaussForwardReduction()
{
    int null_col_counter = 0;
    int *current_row;
    int *current_col;
    int *current_bucket;
    
    // set pointer to buffer, to work on the same pointer than the GPU.
    current_row     = (int*)_currentRowBuffer->contents();
    current_col     = (int*)_currentColBuffer->contents();
    current_bucket  = (int*)_currentBucketBuffer->contents();

    
    while ((*current_row + null_col_counter) < _m->colsInBits)
    {
        *current_col = *current_row + null_col_counter;
        
        //int current_bucket = current_col/B_BITS;
        //int posInBucket = B_BITS - (current_col % B_BITS + 1);
        
        // is pivoting needed for the current iterration?
        // TODO: without this if, just always pivot?
        if (!(*_m)(*current_row, *current_col)) //.data[current_row * _m.buckets + current_bucket].getBit(current_col%B_BITS))
        {
            int new_pivot_index = sendPartitialPivotComputeCommand(*current_row);
            
            if (new_pivot_index == *current_row) {
                null_col_counter++;
                continue; // stop current iteration and check the next col for 1s
            }
            
            
            println("Now swap rows!");
            swapRows(*_m, *current_row, new_pivot_index);
            println("Matrix after rows has been swapped:");
            
            printMatrix(*_m);
        }
        
        *current_bucket = (*current_col)/_m->bits;
        sendXORComputeCommand(*current_row);
        
        println("Matrix after the xor:");
        printMatrix(*_m);
        (*current_row)++;
    }
}

template<typename T>
Matrix<T> *MetalGauss<T>::solve()
{
    gaussForwardReduction();
    
    return _m;
}

template<typename T>
Matrix<T> *MetalGauss<T>::solve_noParallelPivoting() {
    int null_col_counter = 0;
    int *current_row;
    int *current_col;
    int *current_bucket;
    
    // set pointer to buffer, to work on the same pointer than the GPU.
    current_row     = (int*)_currentRowBuffer->contents();
    current_col     = (int*)_currentColBuffer->contents();
    current_bucket     = (int*)_currentBucketBuffer->contents();
    
    while ((*current_row + null_col_counter) < _m->colsInBits)
    {
        *current_col = *current_row + null_col_counter;
        
        // partitial pivot
        int new_pivot_index = -1;
        
        for (unsigned int i = *current_row; i < _m->rows; ++i) {
            if ((*_m)(i, *current_col)) {
                new_pivot_index = i;
                break;
            }
        }
        
        // no 1 has been found below the current row
        if (-1 == new_pivot_index) {
            null_col_counter++;
            continue; // stop current iteration and check the next col for 1s
        }
        
        // swap the rows
        if (new_pivot_index != *current_row) {
            //println("Now swap rows!");
            swapRows(*_m, *current_row, new_pivot_index);
            //println("Matrix after rows has been swapped:");
            //printMatrix(*m);
        }
        
        *current_bucket = (*current_col)/_m->bits;
        sendXORComputeCommand(*current_row);
        
        println("Matrix after the xor:");
        printMatrix(*_m);
        (*current_row)++;
    }
    
    return _m;
}

template<typename T>
void MetalGauss<T>::solve_withoutMetal(Matrix<T> *m) {
    int null_col_counter = 0;
    int current_row = 0;
    
    while ((current_row + null_col_counter) < m->colsInBits)
    {
        int current_col = current_row + null_col_counter; // it is always smaller the col count, because we only iterate until $< cols$.
        int new_pivot_index = -1;
        
        for (int i = current_row; i < m->rows; ++i) {
            if ((*m)(i, current_col)) {
                new_pivot_index = i;
                break;
            }
        }
        
        if (-1 == new_pivot_index) {
            null_col_counter++;
            continue; // stop current iteration and check the next col for 1s
        }
        
        if (new_pivot_index != current_row) {
            swapRows(*m, current_row, new_pivot_index);
        }
        
        
        for (int row = current_row + 1; row < m->rows; ++row) {
            
            // if 1
            if (!(*m)(row,current_col)) {
                continue;
            }
            
            // XOR
            int current_bucket = current_row / m->bits;
            
            for (int c = current_bucket; c < m->buckets; ++c) {
                int index = row * m->buckets + c;
                m->data[index] = m->data[index] ^ m->data[current_row * m->buckets + c];
            }
        }
        
        current_row++;
    }
}

template<typename T>
void MetalGauss<T>::printMatrix(Matrix<T> &m)
{
    if (!_debug) {
        return;;
    }
    m.print();
}
template<typename T>
void MetalGauss<T>::println(std::string s)
{
    if (!_debug) {
        return;
    }
    
    std::cout << s <<std::endl;
}
template<typename T>
MetalGauss<T>::~MetalGauss()
{
    // TDOO: release all pointers!!
    // others:
    delete _factory;
    
    // buffers:
    _mBuffer->setPurgeableState(MTL::PurgeableStateEmpty);
    _mBuffer->release();
    _currentRowBuffer->setPurgeableState(MTL::PurgeableStateEmpty);
    _currentRowBuffer->release();
    _currentColBuffer->setPurgeableState(MTL::PurgeableStateEmpty);
    _currentColBuffer->release();
    _rowsBuffer->setPurgeableState(MTL::PurgeableStateEmpty);
    _rowsBuffer->release();
    _bucketsBuffer->setPurgeableState(MTL::PurgeableStateEmpty);
    _bucketsBuffer->release();
    _indexResultBuffer->release();
}

template class MetalGauss<B1>;
template class MetalGauss<B16>;
template class MetalGauss<B32>;
template class MetalGauss<B64>;
template class MetalGauss<B256>;
