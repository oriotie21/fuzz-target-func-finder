#pragma once
#ifndef LOG_H
#define LOG_H

extern int VERBOSE;
extern int DEBUGGING;
extern int WARN;

#define MSG(...) printf(__VA_ARGS__);
#define DEBUG(...) do { \
    if(DEBUGGING){\
    MSG("[?] "  __VA_ARGS__); \
    MSG("\n"); \
    }\
  } while (0)
#define VERB(...) do { \
    if(VERBOSE){\
    MSG("[>] "  __VA_ARGS__); \
    MSG("\n"); \
    } else{\
    break;\
    }\
  } while (0)
#define INFO(...) do { \
    MSG("[+] "  __VA_ARGS__); \
    MSG("\n"); \
  } while (0)
#define WARN(...) do { \
    if(WARN){ \
    MSG("[!] "  __VA_ARGS__); \
    MSG("\n"); \
       } \
  } while (0)
#define ERROR(...) do { \
    MSG("[-] "  __VA_ARGS__); \
    MSG("\n"); \
    return -1; \
  } while (0)
#define DELAY()do { \
    for(int i=0; i<5000; i++){ \
    printf("");    \
    }\
  } while (0) 

#endif