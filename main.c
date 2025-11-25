// Main file of the serial port project.
// NOTE: This file must not be changed.

#include <stdio.h>
#include <stdlib.h>

#include "application_layer.h"

#define BAUDRATE B1200
#define N_TRIES 5
#define TIMEOUT 1

//ADICIONAR DEFINES
//USAR N_TRIES E TIMEOUT
//Usar funcoes dadas pelos stores (restruturar codigo)
//dar close às à porta serie sempre que ha um erro

// Arguments:
//   $1: /dev/ttySxx
//   $2: tx | rx
//   $3: filename
int main(int argc, char *argv[])
{
    if (argc < 4)
    {
        printf("Usage: %s /dev/ttySxx tx|rx filename\n", argv[0]);
        exit(1);
    }

    const char *serialPort = argv[1];
    const char *role = argv[2];
    const char *filename = argv[3];

    printf("Starting link-layer protocol application\n"
           "  - Serial port: %s\n"
           "  - Role: %s\n"
           "  - Baudrate: %d\n"
           "  - Number of tries: %d\n"
           "  - Timeout: %d\n"
           "  - Filename: %s\n",
           serialPort,
           role,
           BAUDRATE,
           N_TRIES,
           TIMEOUT,
           filename);

    if (strcmp(role, "rx") == 0)
        applicationLayer(serialPort, "rx", BAUDRATE, N_TRIES, TIMEOUT, filename);
    else if (strcmp(role, "tx") == 0)
        applicationLayer(serialPort, "tx", BAUDRATE, N_TRIES, TIMEOUT, filename);
    else printf("error in main, returning\n");

    return 0;
}
