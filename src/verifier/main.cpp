//
//  main.cpp
//  verifier
//  Entry point of the log verifier. Holding also a helper functions to parse the provided arguments...
//
//  Copyright © 2023 Airbus Commercial Aircraft
//  Created by Florian on 15.11.23.
//

#define NS_PRIVATE_IMPLEMENTATION
#define CA_PRIVATE_IMPLEMENTATION
#define MTL_PRIVATE_IMPLEMENTATION
#include <Metal/Metal.hpp>

#include <iostream>
#include "PI.hpp"
#include "Matrix.hpp"
#include "gaussian-elimination/GaussianElimination.hpp"
#include <iomanip>
#include <sstream>
#include <chrono>

using namespace Gauss;
using namespace std;
using std::chrono::high_resolution_clock;
using std::chrono::duration;

static PI::VerifierContext parseCommandLineArguments(int argc, const char * argv[]);


int main(int argc, const char * argv[]) {
    PI::VerifierContext ctx;
    
    ctx = parseCommandLineArguments(argc, argv);
    ctx.m = ceil(ctx.n * C);
    
    std::cout << "Start verifiying logs.\n";
    
    auto t1 = high_resolution_clock::now();
    // create instance of the verifier, based on the context holding all important information.
    PI::Verifier verifier(&ctx);
    
    // start the verification of the provided log file.
    auto result = verifier.Verify();
    auto t2 = high_resolution_clock::now();
    
    duration<long, std::nano> ns_double = t2 - t1;
    cout << "Verification process end to end needed: " << ns_double.count() << " ns" << endl;
    return 0;
}

static PI::VerifierContext parseCommandLineArguments(int argc, const char * argv[])
{
    PI::VerifierContext ctx;
    ctx.useMetal = true; // default
    
    // Check if the minimum number of arguments is met
    if (argc < 9 || argc > 10) {
        std::cerr << "Error: Insufficient number of arguments." << std::endl;
        exit(EXIT_FAILURE);
    }
    
    for (int i = 1; i < argc; ++i) {
        std::cout << "Argument " << i << ": " << argv[i] << std::endl;
        if (std::string(argv[i]) == "-k" || std::string(argv[i]) == "--key") {
            // Check if there is a value following the -k option
            if (i + 1 < argc) {
                i++;
                ctx.masterKeyPath = argv[i];
            } else {
                std::cerr << "Error: Missing value for -k option." << std::endl;
                exit(EXIT_FAILURE);
            }
        } else if (std::string(argv[i]) == "-l" || std::string(argv[i]) == "--logs") {
            // Check if there is a value following the -l option
            if (i + 1 < argc) {
                i++;
                ctx.logFileDirectory = argv[i];
            } else {
                std::cerr << "Error: Missing value for -l option." << std::endl;
                exit(EXIT_FAILURE);
            }
        } else if (std::string(argv[i]) == "-o" || std::string(argv[i]) == "--out") {
            // Check if there is a value following the -l option
            if (i + 1 < argc) {
                i++;
                ctx.outFile = argv[i];
            } else {
                std::cerr << "Error: Missing value for -o option." << std::endl;
                exit(EXIT_FAILURE);
            }
        } else if (std::string(argv[i]) == "-n") {
            // Check if there is a value following the -l option
            if (i + 1 < argc) {
                i++;
                try {
                    ctx.n = std::stoi(argv[i]);
                } catch (const std::invalid_argument& e) {
                    std::cerr << "Invalid argument for -n: " << e.what() << std::endl;
                } catch (const std::out_of_range& e) {
                    std::cerr << "Out of range: " << e.what() << std::endl;
                }
            } else {
                std::cerr << "Error: Missing value for -n option." << std::endl;
                exit(EXIT_FAILURE);
            }
        } else if (std::string(argv[i]) == "--no-metal") {
            ctx.useMetal = false;
        } else {
            std::cerr << "Error: Unknown option or invalid argument: " << argv[i] << std::endl;
            exit(EXIT_FAILURE);
        }
    }
    
    return ctx;
}
