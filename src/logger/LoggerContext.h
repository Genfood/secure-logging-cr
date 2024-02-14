//
//  LoggerContext.h
//  logger
//
//  Created by Gollum on 17.01.24.
//

#ifndef LoggerContext_h
#define LoggerContext_h

// A struct to hold the context of the logger.
typedef struct {
    const char *outputPath;
    const char *logPath; // input path
    const char *logFileName;
    int maxLogs;
} LoggerContext;

#endif /* LoggerContext_h */
