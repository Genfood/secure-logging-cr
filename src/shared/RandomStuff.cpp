//
//  RandomStuff.cpp
//  shared
//
//  Created by Florian on 18.10.23.
//

#include "array"
#include "RandomStuff.hpp"

bool exists(size_t element, const size_t arr[], size_t size)
{
    for (size_t i = 0; i < size; ++i) {
        if (arr[i] == element) {
            return true;
        }
    }
    return false;
}
size_t *distinctRandomEz(size_t range, int k) {
    std::random_device rd;  // Use hardware entropy source if available
    // init random stuf
    std::mt19937 gen(rd());
    size_t *k_random = new size_t[k];
    
    std::fill(k_random, k_random + k, -1);
    
    std::uniform_int_distribution<size_t> distribution(0, range);
    
    int i = 0;
    while (i < k) {
        size_t r = distribution(gen);
        
        if (exists(r, k_random, k))
        {
            continue;;
        }
        
        k_random[i] = r;
        i++;
    }
    
    return k_random;
}

size_t *distinctRandomEz(size_t range, int k, int seed)
{
    // init random stuf
    std::mt19937 gen(seed);
    size_t *k_random = new size_t[k];
    
    std::fill(k_random, k_random + k, -1);
    
    std::uniform_int_distribution<size_t> distribution(0, range);
    
    int i = 0;
    while (i < k) {
        size_t r = distribution(gen);
        
        if (exists(r, k_random, k))
        {
            continue;;
        }
        
        k_random[i] = r;
        i++;
    }
    
    return k_random;
}
