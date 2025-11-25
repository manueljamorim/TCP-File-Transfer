
#ifndef _STATE_MACHINE_H
#define _STATE_MACHINE_H

#include <stdio.h>
#include "macros.h"
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <termios.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>

typedef enum {
    START,
    FLAG_RCV,
    A_RCV,
    C_RCV_SET,
    C_RCV_UA,
    C_RCV_RR,
    C_RCV_REJ,
    C_RCV_DISC,
    BCC_OK_SET,
    BCC_OK_UA,
    BCC_OK_RR,
    BCC_OK_REJ,
    BCC_OK_DISC,
    C_RCV_I,
    BCC1_I,
    S_I,
    BCC2_I
} STATES;

extern STATES m_state;

// Main processing function that redirects to all the other subfunctions dedicated 
// to recognizing and validating specific purpose packets
RETURN_CODE state_machine(unsigned char byte);

//Validates SET packets
RETURN_CODE state_machine_SET(unsigned char byte);
//Validates UA packets
RETURN_CODE state_machine_UA(unsigned char byte);
//Validates I packets
RETURN_CODE state_machine_I(unsigned char byte);
//Validates DISC packets
RETURN_CODE state_machine_DISC(unsigned char byte);
//Validates RR packets
RETURN_CODE state_machine_RR(unsigned char byte);
//Validates REJ packets
RETURN_CODE state_machine_REJ(unsigned char byte);


#endif