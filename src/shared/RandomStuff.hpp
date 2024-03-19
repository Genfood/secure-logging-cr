//
//  RandomStuff.hpp
//  shared
//
//  Copyright © 2023 Airbus Commercial Aircraft
//  Created by Florian on 18.10.23.
//

#ifndef RandomStuff_hpp
#define RandomStuff_hpp

#include <stdio.h>
#include <random>


size_t *distinctRandomEz(size_t range, int k);
size_t *distinctRandomEz(size_t range, int k, int seed);

#endif /* RandomStuff_hpp */
