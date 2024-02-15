# Implementation and GPU-Based Optimizations for Secure Logs with Forward Integrity and Crash Recovery

This repository contains an implementation of Blass and Noubir's paper: [Forward Security with Crash Recovery for Secure Logs](https://eprint.iacr.org/2019/506)

The implementation is part of my master thesis. It is currently only tested on my MacBook, and parts of it are written for Metal. The current state is unlikely to run on any other hardware other than Apple. This is likely to change in the future.

## Information

I tried to add `// line x` comments to link the source code to the pseudo code from Blass and Noubirs paper. For reference check the aforementioned paper.

The XCode project can be structured into four distinct parts:

- logger: holds the code for adding log entries into the secure log file.
- verifier: holds the code to verify the secure logging file.
- gauss-benchmark: holds code that benchmarks different gauss approaches, which is in detail explained in my Master Thesis.
- shared: holding code that will be exchanged between all this projects.

## logger

The logger has several arguments which can be passed on start up:

- **-o | --output**: the directory in which the log file should be stored, also the keys will be stored here.
- **-l | --logs**: provide the path to a file holding logs in text format, which should be logged using secure logging.
- **-f | --filename**: overrides the default filename of the log file, which will be stored in the provided output directory.
- **-m | --maxlogs**: (n) the maximum number of logs the log file should hold.

## verifier

The verifier has several arguments which could be passed on start up:

- **-k | --key**: the file path to the master key.
- **-l | --logs**: the directory path within the secure logging file is located.
- **-o | --out**: the file path to the wished location, in which the resulting clear log file should be created.
- **-n**: the maximum number of log files, the given secure logging file could hold.
- **--no-metal**: flag indicating that the CPU should be used instead of the GPU. Should be used if n is less than 2^15.

## gauss-benchmark

The benchmarking utility has several arguments, which could be passed on the start up:

- **-min | --min**: the starting from number of the matrix size, for which should be tested. the provided number x will be the power to two: 2^x.
- **-max | --max**: the ending to which should be tested. The provided number x will be the power to two: 2^x.
- **-bs | --bucket-size**: A comma separated list of bucket sizes on which should be tested. Allowed values are: 1, 32, 64, 256.
- **-d | --debug-prints**: flag indicating to print debug statements.
