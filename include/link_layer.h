// Link layer header.
// NOTE: This file must not be changed.

#ifndef _LINK_LAYER_H_
#define _LINK_LAYER_H_

#include <termios.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
#include <sys/time.h>
#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include "state_machine.h"

typedef enum
{
    LlTx,
    LlRx,
} LinkLayerRole;

typedef struct
{
    char serialPort[50];
    LinkLayerRole role;
    int baudRate;
    int nRetransmissions;
    int timeout;
} LinkLayer;

// SIZE of maximum acceptable payload.
// Maximum number of bytes that application layer should send to link layer
#define MAX_PAYLOAD_SIZE 1000

// MISC
#define FALSE 0
#define TRUE 1

// Open a connection using the "port" parameters defined in struct linkLayer.
// Return "1" on success or "-1" on error.
int llopen(LinkLayer connectionParameters);

// Send data in buf with size bufSize.
// Return number of chars written, or "-1" on error.
int llwrite(const unsigned char *buf, int bufSize);

// Receive data in packet.
// Return number of chars read, or "-1" on error, or 0 if it's an irrelevant packet which carries no data.
int llread(unsigned char *packet);

// Close previously opened connection.
// if showStatistics == TRUE, link layer should print statistics in the console on close.
// Return "1" on success or "-1" on error.
int llclose(int fd, int showStatistics);

//Exchange DISC and UA packets to signal the intent of closing the connection
int llend(LinkLayerRole role);

//Alarm handler that sets AlarmEnabled to false when called by a set alarm
void alarmHandler(int signal);

//Check the serial port for a correct RR packet for "timeout" seconds
RETURN_CODE check_RR(int fd);

//Send a RR packet that signals the transmitter that a packet was correctly read
void RR_send(int fd, uint Nr);

//Send a REJ packet that signals the transmitter that a packet was missread
void REJ_send(int fd, uint Nr);

#endif // _LINK_LAYER_H_
