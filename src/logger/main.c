//
//  main.cpp
//  logger
//
//  Created by Florian on 27.10.23.
//

#include <stdio.h>
#include "Crypto.h"
#include <string.h>
#include <time.h>
#include "PI.h"
#include "LoggerContext.h"

#define N 10
#define MAX_LINE_LENGTH MESSAGE_LEN // length of the define max log legnth

LoggerContext parseArgs(int argc, const char * argv[]);
char **readLogs(const char *path, int *logCount, int maxlogCount);

int main(int argc, const char * argv[]) {
    struct timespec start, end;
    long seconds, nanoseconds;;
    
    LoggerContext loggerCtx = parseArgs(argc, argv);
    int logCount;
    char **logs = readLogs(loggerCtx.logPath, &logCount, loggerCtx.maxLogs);
    
    PIContext *ctx = CreatePIContext(logCount, loggerCtx.outputPath, loggerCtx.outputPath, loggerCtx.logFileName);
    Init(ctx);
    clock_gettime(CLOCK_MONOTONIC, &start);
    for (int i = 0; i < logCount; ++i) {
        // Pass the modified string to AddLogEntry
        AddLogEntry(ctx, (unsigned char *)logs[i], (int)strlen(logs[i]));
    }
    clock_gettime(CLOCK_MONOTONIC, &end);
    
    
    // print execution time:
    seconds = end.tv_sec - start.tv_sec;
    nanoseconds = end.tv_nsec - start.tv_nsec;
    
    // Adjust the values in case of a negative nanoseconds difference
    if (nanoseconds < 0) {
        seconds--;
        nanoseconds += 1000000000;
    }
    printf("Execution time: %ld nano seconds\n", nanoseconds);
    printf("Execution time: %ld seconds and %ld nanoseconds\n", seconds, nanoseconds);

    printf("Execution time: %ld seconds\n", seconds);
    printf("%d Logs has been written.", logCount);
    
    fclose(ctx->logFile);
    fclose(ctx->keyFile);
    free(ctx);
    return 0;
}


LoggerContext parseArgs(int argc, const char * argv[]) {
    LoggerContext ctx = {NULL, NULL, "test", INT_MAX};

    for (int i = 1; i < argc; i++) {
        if ((strcmp(argv[i], "-o") == 0 || strcmp(argv[i], "--output") == 0) && i + 1 < argc) {
            ctx.outputPath = argv[++i];
        } else if ((strcmp(argv[i], "-l") == 0 || strcmp(argv[i], "--logs") == 0) && i + 1 < argc) {
            ctx.logPath = argv[++i];
        } else if ((strcmp(argv[i], "-f") == 0 || strcmp(argv[i], "--filename") == 0) && i + 1 < argc) {
            ctx.logFileName = argv[++i];
        } else if ((strcmp(argv[i], "-m") == 0 || strcmp(argv[i], "--maxlogs") == 0) && i + 1 < argc) {
            ctx.maxLogs = atoi(argv[++i]);
            if (ctx.maxLogs <= 0) {
                fprintf(stderr, "ERROR: Invalid number of maximum logs\n");
                exit(EXIT_FAILURE);
            }
        } else {
            fprintf(stderr, "Usage: %s [-o|--output] <output_path> [-l|--logs] <log_path> [-f|--filename <log_file_name>] [-m|--maxlogs <max_logs>]\n", argv[0]);
            exit(EXIT_FAILURE);
        }
    }

    if (ctx.outputPath == NULL || ctx.logPath == NULL) {
        fprintf(stderr, "Both output and log paths must be specified\n");
        exit(EXIT_FAILURE);
    }

    return ctx;
}

char **readLogs(const char *path, int *logCount, int maxlogCount) {
    FILE *file = fopen(path, "r");
    
    if (!file) {
        perror("ERROR: opening log file\n");
        return NULL;
    }
    
    int capacity = 1000; // Initial capacity for the array of line pointers
    char **lines = malloc(capacity * sizeof(char *));
    if (!lines) {
        perror("ERROR: Memory allocation failed\n");
        fclose(file);
        return NULL;
    }
    
    char buffer[MAX_LINE_LENGTH];
    int count = 0;
    while (count < maxlogCount && fgets(buffer, MAX_LINE_LENGTH, file)) {
        // Strip newline character if it exists
        size_t len = strlen(buffer);
        if (len > 0 && buffer[len - 1] == '\n') {
            buffer[len - 1] = '\0';
        }
        
        if (count >= capacity) {
            // Increase the capacity
            capacity *= 2;
            char **newLines = realloc(lines, capacity * sizeof(char *));
            if (!newLines) {
                perror("ERROR: Memory reallocation failed\n");
                for (int i = 0; i < count; ++i) {
                    free(lines[i]);
                }
                free(lines);
                fclose(file);
                return NULL;
            }
            lines = newLines;
        }

        // Allocate space for the line and store it
        lines[count] = strdup(buffer);
        if (!lines[count]) {
            perror("ERROR: Memory allocation failed\n");
            for (int i = 0; i < count; ++i) {
                free(lines[i]);
            }
            free(lines);
            fclose(file);
            return NULL;
        }
        count++;
    }

    fclose(file);
    *logCount = count;
    return lines;

}
