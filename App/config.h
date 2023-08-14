#ifndef CONFIG_H
#define CONFIG_H


#include <stdint.h>


#define HDR_START 0x0
#define HDR_PING  0x1
#define HDR_REPLY 0x2

#define MAXLINE   256
#define DEBUG     true

typedef unsigned int PID; // process ids
typedef unsigned int CID; // client ids
typedef unsigned int VAL; // payload
typedef unsigned int COUNT;


// ----------------------------------------
// Colors
// ------

#define KNRM  "\x1B[0m"

// default background & different foreground colors
#define KRED  "\x1B[49m\x1B[31m"
#define KGRN  "\x1B[49m\x1B[32m"
#define KYEL  "\x1B[49m\x1B[33m"
#define KBLU  "\x1B[49m\x1B[34m"
#define KMAG  "\x1B[49m\x1B[35m"
#define KCYN  "\x1B[49m\x1B[36m"
#define KWHT  "\x1B[49m\x1B[37m"

// default background & different (light) foreground colors
#define KLRED  "\x1B[49m\x1B[91m"
#define KLGRN  "\x1B[49m\x1B[92m"
#define KLYEL  "\x1B[49m\x1B[93m"
#define KLBLU  "\x1B[49m\x1B[94m"
#define KLMAG  "\x1B[49m\x1B[95m"
#define KLCYN  "\x1B[49m\x1B[96m"
#define KLWHT  "\x1B[49m\x1B[97m"

// diferent background colors & white foreground
#define KBRED  "\x1B[41m\x1B[37m"
#define KBGRN  "\x1B[42m\x1B[37m"
#define KBYEL  "\x1B[43m\x1B[37m"
#define KBBLU  "\x1B[44m\x1B[37m"
#define KBMAG  "\x1B[45m\x1B[37m"
#define KBCYN  "\x1B[46m\x1B[37m"
#define KBWHT  "\x1B[47m\x1B[30m"

#endif
