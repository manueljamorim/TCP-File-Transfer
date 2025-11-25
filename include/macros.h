#ifndef _MACROS_H
#define _MACROS_H_

#define _POSIX_SOURCE 1 // POSIX compliant source

#include <math.h>

typedef unsigned int uint;

#define FALSE 0
#define TRUE 1

#define DEBUG_FER 0
#define DEBUG_T_prop 0
#define DEBUG_ERROR_CHANCE 0.60
#define DEBUG_T_prop_time ((int) 0.5 * pow(10, 6))  //IN MICROSECONDS

#define INFORMATION_FIELD_SIZE 32
#define BUF_SIZE INFORMATION_FIELD_SIZE+7  //32 data bytes + 7 control bytes
//Each information received is: 32bytes --> 4 bytes for flags e 28 bytes for real data
#define INFORMATION_FIELD_CONTROL_BYTES 4
#define DATA_FIELD_SIZE (INFORMATION_FIELD_SIZE - INFORMATION_FIELD_CONTROL_BYTES) //Information field without control bytes (C N L2 L1) 

#define DATA_CONTROL_FIELD 0b00000001 
#define START_CONTROL_FIELD 0b00000010 

//DEFINE FLAG, C, A
    #define FLAG 0b01111110
    #define C_SET 0b00000011
    #define C_DISC 0b00001011
    #define C_UA 0b00000111
    #define C_0_I 0b00000000
    #define C_1_I 0b01000000
    #define C_I(n) (n << 6) 
    #define C_RR(Nr) (0b00000101 | (Nr << 7))
    #define C_0_RR (0b00000101)
    #define C_1_RR (0b10000101)
    #define C_REJ(Nr) (0b00000001 | (Nr << 7))
    #define C_0_REJ (0b00000001)
    #define C_1_REJ (0b10000001)
    #define A_EMMITER_RECEIVER 0b00000011 //the one we'll be using
    #define A_RECEIVER_EMITER 0b00000001

//DEDFINE STATE MACHINE RETURN CODES
    typedef enum RETURN_CODE {UNDEFINED, SET_CODE, UA_CODE, RR_0_CODE, RR_1_CODE,
    REJ_0_CODE, REJ_1_CODE, S_CODE, DISC_CODE, RCV_DATA, I_0_CODE, I_1_CODE} RETURN_CODE;

//auxiliary code for byte analysis and debugging
    #define BYTE_TO_BINARY_PATTERN "%c%c%c%c%c%c%c%c"
    #define BYTE_TO_BINARY(byte)  \
    (byte & 0x80 ? '1' : '0'), \
    (byte & 0x40 ? '1' : '0'), \
    (byte & 0x20 ? '1' : '0'), \
    (byte & 0x10 ? '1' : '0'), \
    (byte & 0x08 ? '1' : '0'), \
    (byte & 0x04 ? '1' : '0'), \
    (byte & 0x02 ? '1' : '0'), \
    (byte & 0x01 ? '1' : '0') 
    

#endif