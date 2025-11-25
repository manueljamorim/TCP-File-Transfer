// Link layer protocol implementation

#include "link_layer.h"

// MISC
#define _POSIX_SOURCE 1 // POSIX compliant source

struct termios oldtio;
struct termios newtio;

struct timeval begin, end;
double elapsed = 0;

uint alarmEnabled = FALSE;
uint Ns_counter = 0;
int debug_rand = 0;
int fd, NRetries, baudRate, timeout;
int packets_received = 0;

volatile int STOP_FLAG = FALSE;
unsigned char buf[BUF_SIZE + 1] = {0}; // +1: Save space for the final '\0' unsigned char
unsigned char data_buffer[INFORMATION_FIELD_SIZE + 1] = {0};
uint byte_counter_per_packet = 0;
uint s; // data packet length
RETURN_CODE state_machine_code;

////////////////////////////////////////////////
// LLOPEN
////////////////////////////////////////////////

int llopen(LinkLayer connectionParameters)
{
    (void)signal(SIGALRM, alarmHandler);

    NRetries = connectionParameters.nRetransmissions;
    baudRate = connectionParameters.baudRate;
    timeout = connectionParameters.timeout;

    // Open serial port device for reading and writing, and not as controlling tty
    // because we don't want to get killed if linenoise sends CTRL-C.
    if (connectionParameters.role == 0) fd = open(connectionParameters.serialPort, O_RDWR | O_NOCTTY);
    else fd = open(connectionParameters.serialPort, O_RDWR | O_NOCTTY);

    if (fd < 0)
    {
        perror(connectionParameters.serialPort);
        exit(-1);
    }


    // Save current port settings
    if (tcgetattr(fd, &oldtio) == -1)
    {
        perror("tcgetattr");
        exit(-1);
    }

    // Clear struct for new port settings
    memset(&newtio, 0, sizeof(newtio));

    newtio.c_cflag =  baudRate | CS8 | CLOCAL | CREAD;
    newtio.c_iflag = IGNPAR;
    newtio.c_oflag = 0;

    // Set input mode (non-canonical, no echo,...)
    newtio.c_lflag = 0;
    newtio.c_cc[VTIME] = 1; // Inter-unsigned character timer unused
    newtio.c_cc[VMIN] = 0;  // Blocking read until 5 unsigned chars received

    // Set new port settings
    if (tcsetattr(fd, TCSANOW, &newtio) == -1)
    {
        perror("tcsetattr");
        return -1;
    }

    tcflush(fd, TCIOFLUSH);

    /*------------------------------------------  TRANSMITTER  ----------------------------------------------*/
    if (connectionParameters.role == 0){         //Establishing logical conection

        //Assembling SET packet
        //For establishing the connection we'll be using FLAG, C_SET, A_EMITTER_RECEIVER and BCC
        unsigned char BCC = C_SET & A_EMMITER_RECEIVER; //BCC is defined as the bitwise & between A and C
        unsigned char SET_Packet[] =  {FLAG, A_EMMITER_RECEIVER, C_SET, BCC, FLAG, '\0'};
        
        uint try_counter = 0;
        unsigned char buf_rcv[BUF_SIZE + 1] = {0};
        int bytes;
        alarmEnabled = FALSE;

        while (try_counter < NRetries){
            printf("alarm: %d\n", alarmEnabled);
            
            if (alarmEnabled == FALSE)
            {
                alarm(timeout); // Set alarm to be triggered in 3s
                alarmEnabled = TRUE;

                //sending SET Packet
                printf("\n> Sending SET Packet\n");
                write(fd, SET_Packet, 5);
                sleep(1);

                bytes = read(fd, buf_rcv, BUF_SIZE); //returns length of string

                buf_rcv[bytes] = '\0'; // Set end of string to '\0', so we can printf

                for (int i = 0; i<bytes && bytes > 0; i++){
                    if (state_machine(buf_rcv[i]) == UA_CODE){
                        printf("\nUA Packet Received, Beginning Data Exchange\n\n");
                        return fd;
                    }  
                }
                printf("Failed to receive response, retrying\n");
                try_counter++;
            }
        }

        printf("Not able to connect with receiver\n");
        return -1;
    }
    /*------------------------------------------  RECEIVER  ----------------------------------------------*/
    else if (connectionParameters.role == 1){
        // Loop for input
        unsigned char buf[BUF_SIZE + 1] = {0};

        RETURN_CODE state_machine_code;
        while (TRUE)
        {
            // Returns after BUF_SIZE unsigned chars have been input
            int bytes = read(fd, buf, BUF_SIZE); // returns length of string
            buf[bytes] = '\0';                   // Set end of string to '\0', so we can printf


            for (int i = 0; i < bytes && bytes > 0; i++)
            {
                state_machine_code = state_machine(buf[i]);

                if (state_machine_code == SET_CODE)
                {
                    printf("\nSET Packet Received, sending UA Packet\n");
                    // Send UA packet
                    unsigned char BCC = (A_EMMITER_RECEIVER & C_UA);
                    unsigned char UA_Packet[] = {FLAG, A_EMMITER_RECEIVER, C_UA, BCC, FLAG, '\0'};
                    write(fd, UA_Packet, 5);
                    gettimeofday(&begin, 0);
                    return fd;
                }

            }
        }
        return 0;
    }


    return fd;
}

////////////////////////////////////////////////
// LLWRITE
////////////////////////////////////////////////
int llwrite(const unsigned char *buf, int bufSize)
{
    unsigned char BCC1;
    unsigned char BCC2;
    unsigned char I_Packet[INFORMATION_FIELD_SIZE+7+1]; 
    uint RR_RCV = FALSE;
    uint try_counter = 0;

    BCC1 =  C_I(Ns_counter%2) & A_EMMITER_RECEIVER;

    I_Packet[0] = FLAG;
    I_Packet[1] = A_EMMITER_RECEIVER;
    I_Packet[2] = C_I(Ns_counter%2);
    I_Packet[3] = BCC1;
    I_Packet[4] = bufSize;

    memcpy(I_Packet + 5, buf, bufSize);

    BCC2 = I_Packet[5];
    for (int j = 0; j < bufSize; j++){
        BCC2 = BCC2 & I_Packet[5+j];
    }

    I_Packet[bufSize+5] = BCC2;   
    I_Packet[bufSize+6] = FLAG;       
    I_Packet[bufSize+7] = '\0';       // 6 control bits

    //printf("\ndata as string: !%s!\n", I_Packet + 5);
    alarmEnabled = FALSE;
    while (!RR_RCV && try_counter < NRetries)
    {
        if (alarmEnabled == FALSE){

            printf("\nNs_counter: %d\n", Ns_counter);
            write(fd, I_Packet, bufSize+7); //data + control bits
        
            RETURN_CODE check_rr_result = check_RR(fd);

            alarmEnabled = TRUE;
            alarm(timeout);

            if (check_rr_result == RR_0_CODE){
                if (Ns_counter == 1) {
                    RR_RCV = TRUE;
                    printf("Received RR_0 packet\n");
                }
                else printf("Received RR_0 packet, wrong return packet\n");
            }
            else if (check_rr_result == RR_1_CODE){
                if (Ns_counter == 0){
                    RR_RCV = TRUE;
                    printf("Received RR_1 packet\n");
                }
                else printf("Received RR_1 packet, wrong return packet\n");
            }
            else printf("RR packet not received\n");

            try_counter++;
        }
    }
    if (try_counter == NRetries) return -1;

    Ns_counter = (Ns_counter+1)%2;

    return 0;
}

////////////////////////////////////////////////
// LLREAD
////////////////////////////////////////////////
int llread(unsigned char *packet)
{   
    while (STOP_FLAG == FALSE)
    {
        // Returns after BUF_SIZE unsigned chars have been input
        int bytes = read(fd, buf, BUF_SIZE); // returns length of string
        buf[bytes] = '\0';                   // Set end of string to '\0', so we can printf

        //printf("\n\n Link Layer bytes received: %d", bytes);

        for (int i = 0; i < bytes && bytes > 0; i++)
        {
            if (byte_counter_per_packet == INFORMATION_FIELD_SIZE)
            { // Prevents from reading wrong
                byte_counter_per_packet = 0;
            }

            if (m_state == START || m_state == FLAG){
                s = 0;
                byte_counter_per_packet = 0;
            }

            state_machine_code = state_machine(buf[i]);
            // printf("m_state machine return code: %d\n", state_machine_code);

            if (state_machine_code != RCV_DATA)
            {
                byte_counter_per_packet = 0;
            }

            if (state_machine_code == SET_CODE)
            {
                printf("\nSET Packet Received, sending UA Packet\n");

                // SEND UA
                unsigned char BCC = (A_EMMITER_RECEIVER & C_UA);
                unsigned char UA_Packet[] = {FLAG, A_EMMITER_RECEIVER, C_UA, BCC, FLAG, '\0'};
                write(fd, UA_Packet, 5);
            }
            else if (state_machine_code == DISC_CODE)
            {
                LinkLayerRole role = LlRx;
                llend(role);

                return -1;
            }
            else if (state_machine_code == UA_CODE)
            {
                printf("\nUA Packet Received, Receiver Disconnecting\n");
                STOP_FLAG = TRUE;
                break;
            }
            else if (state_machine_code == S_CODE)
            { // BYTE THAT TELLS RECEIVER HOW MANY BYTES TO READ
                s = buf[i];
                printf("\n> Data Packet Received\n> Read Bytes: %d\n", s);
            }
            else if (state_machine_code == RCV_DATA)
            {
                memcpy((data_buffer + byte_counter_per_packet), buf + i, 1);
                // printf("unsigned char counter: %d\n", byte_counter_per_packet);
                byte_counter_per_packet++;
            }
            else if (state_machine_code == I_0_CODE)
            {
                //Varying FER
                if (DEBUG_FER){
                    srand(time(NULL));
                    debug_rand = (rand() % 100);
                    if (debug_rand < DEBUG_ERROR_CHANCE * 100)
                        return 0;
                }

                if (DEBUG_T_prop)
                    usleep(DEBUG_T_prop_time);

                printf("Ns = 0 Packet Received\n");  //SUCCESSFULY READ PACKET
                // printf("data received: %d", byte_counter_per_packet);
                // printf("\ntemporary data: %s\n",data_buffer);
                if (Ns_counter != 0){
                    printf("> Sending RR 1 Packet again\n");
                    RR_send(fd, 1);
                    return 0;
                }
                Ns_counter = 1;
                memcpy(packet, data_buffer, s);
                printf("> Sending RR 1 Packet\n");
                RR_send(fd, 1);

                if (!bytes) return 0;
                else packets_received++;
                return s;
            }
            else if (state_machine_code == I_1_CODE)  //SUCCESSFULY READ PACKET
            {
                //Varying FER
                if (DEBUG_FER){
                    srand(time(NULL));
                    debug_rand = (rand() % 100);
                    if (debug_rand < DEBUG_ERROR_CHANCE * 100)
                        return 0;
                }

                if (DEBUG_T_prop)
                    usleep(DEBUG_T_prop_time);

                printf("Ns = 1 Packet Received\n");
                // printf("data received: %d\n", byte_counter_per_packet);
                // printf("\ntemporary data: %s\n",data_buffer);
                if (Ns_counter != 1){
                    printf("> Sending RR 0 Packet again\n");
                    RR_send(fd, 0);
                    return 0;
                }
                Ns_counter = 0;
                memcpy(packet, data_buffer, s);
                printf("> Sending RR 0 Packet\n");
                RR_send(fd, 0);

                if (!bytes) return 0;
                else packets_received++;
                return s;
            }
        }

    }
    return 0;
}

////////////////////////////////////////////////
// LLEND
////////////////////////////////////////////////

int llend(LinkLayerRole role){
    /*------------------------------------------  TRANSMITTER  ----------------------------------------------*/
    if(role == 0){
        // -----------------Envio da trama final DISC -----------------------
        unsigned char BCC = C_DISC & A_EMMITER_RECEIVER; //BCC is defined as the bitwise & between A and C
        unsigned char DISC_Packet[] =  {FLAG, A_EMMITER_RECEIVER, C_DISC, BCC, FLAG, '\0'};

        
        //DEFINING UA
        unsigned char BCC_UA = (A_EMMITER_RECEIVER & C_UA);
        unsigned char UA_Packet[] =  {FLAG, A_EMMITER_RECEIVER, C_UA, BCC_UA, FLAG, '\0'};


        uint try_counter = 0;
        uint bytes;
        uint DISC_rcv = FALSE;
        unsigned char buf_rcv[BUF_SIZE + 1] = {0};
        //bytes;

        while (!DISC_rcv && try_counter < NRetries){

            if (alarmEnabled == FALSE)
            {
                alarm(timeout); // Set alarm to be triggered in 3s
                alarmEnabled = TRUE;

                //sending DISC Packet
                write(fd, DISC_Packet, 5);
                printf("\n> DISK Packet Sent\n");
                
                sleep(1);

                buf_rcv[0] = '\0';
                bytes = read(fd, buf_rcv, BUF_SIZE); //returns length of string

                printf("bytes: %d\n",bytes);

                for (int i = 0; i<bytes && bytes>0; i++){
                    if (state_machine(buf_rcv[i]) == DISC_CODE){
                        //We received a DISC, lets check if we dont receive another one in the next 3 seconds
                        //If it all goes well and the Rx receives the UA we're about to send we wont receive another DISC
                        //SENDING UA PACKET
                        try_counter = 0;
                        while (try_counter < NRetries)
                        {
                            printf("\n> DISC Packet Received, sending UA\n");
                            write(fd, UA_Packet, 5);
                            sleep(timeout);
                            bytes = read(fd, buf_rcv, BUF_SIZE); 

                            DISC_rcv = FALSE;
                            for (int i = 0; i<bytes && bytes>0; i++){
                                if (state_machine(buf_rcv[i]) == DISC_CODE){
                                    DISC_rcv = TRUE;
                                }
                            }
                            try_counter++;
                            if (!DISC_rcv) return 0;
                        }
                        printf("\n> Receiver is not receiving UA, force disconnecting\n");
                        return 0;

                    }  
                }
                
                if(!DISC_rcv) printf("> Failed to receive DISC response, retrying\n");

                try_counter++;
            }
        }

        if (!DISC_rcv) printf("> Not able to DISCONECT with receiver\n");
    }
    
    else if (role == 1) { //receiver
        printf("\n> DISC Packet Received, sending DISC Packet\n");

        int UA_rcv = FALSE;
        int try_counter = 0;
        uint bytes;
        unsigned char buf_rcv[BUF_SIZE + 1] = {0};
        while (!UA_rcv && try_counter < NRetries){

            if (alarmEnabled == FALSE)
            {
                alarm(timeout); // Set alarm to be triggered in 3s
                alarmEnabled = TRUE;

                //sending DISC Packet
                unsigned char BCC = (A_EMMITER_RECEIVER & C_DISC);
                unsigned char DISC_Packet[] = {FLAG, A_EMMITER_RECEIVER, C_DISC, BCC, FLAG, '\0'};
                write(fd, DISC_Packet, 5);
                printf("\n> DISK Packet Sent\n");
                
                sleep(1);

                buf_rcv[0] = '\0';
                bytes = read(fd, buf_rcv, BUF_SIZE); //returns length of string
                buf_rcv[bytes] = '\0'; // Set end of string to '\0', so we can printf

                for (int i = 0; i<bytes && bytes>0; i++){
                    if (state_machine(buf_rcv[i]) == UA_CODE){
                        printf("\n> UA Packet Received, disconnecting\n");

                        UA_rcv = TRUE;
                        return 0;
                    }  
                }
                
                if(!UA_rcv) printf("> Failed to receive UA response, retrying\n");

                try_counter++;
            }

        }
        printf("> Failed to receive UA, force disconnecting\n");
        return -1;
    }
    
    return 0;
}

////////////////////////////////////////////////
// LLCLOSE
////////////////////////////////////////////////
int llclose(int fd, int showStatistics)
{
    if (showStatistics){
        gettimeofday(&end, 0);
        long seconds = end.tv_sec - begin.tv_sec;
        long microseconds = end.tv_usec - begin.tv_usec;
        elapsed = seconds + microseconds*1e-6;

        printf("\n---------------------- STATISTICS -----------------------\n");
        printf("\nBaudRate: %d\t NRetries: %d\tTimeout: %d\n", baudRate, NRetries, timeout);
        printf("\nPackets Received: \t\t%d\n", packets_received);
        printf("Data Transfer Execution Time: \t%.3f seconds.\n", elapsed);

    }

    tcflush(fd, TCIOFLUSH);
    // Restore the old port settings
    if (tcsetattr(fd, TCSANOW, &oldtio) == -1)
    {
        perror("tcsetattr");
        exit(-1);
    }

    close(fd);

    printf("\n> Restored previous serial port settings\n");

    return fd;
}

RETURN_CODE check_RR(int fd){

    RETURN_CODE state_machine_code;

    uint RR_RCV = FALSE;
    //uint try_counter = 0;

    alarm(timeout);
    alarmEnabled = TRUE;

    while (!RR_RCV && alarmEnabled == TRUE )
    {   
        
        unsigned char buf[BUF_SIZE] = {0};
        // Returns after BUF_SIZE unsigned chars have been input
        int bytes = read(fd, buf, BUF_SIZE); //returns length of string
        buf[bytes] = '\0'; // Set end of string to '\0', so we can printf

        //printf("bytes received: %d\n\n", bytes);

        for (int i = 0; i<bytes; i++){

            state_machine_code = state_machine(buf[i]);
            //printf("m_state machine return code: %d\n", state_machine_code);

            if (state_machine_code == RR_0_CODE){
                return RR_0_CODE;
            }
            else if (state_machine_code == RR_1_CODE){ 
                return RR_1_CODE;
            }
        }
        
    }
    return UNDEFINED;
}


void RR_send(int fd, uint Nr)
{
    unsigned char BCC = (A_EMMITER_RECEIVER & C_RR(Nr));
    unsigned char RR_packet[] = {FLAG, A_EMMITER_RECEIVER, C_RR(Nr), BCC, FLAG, '\0'};
    write(fd, RR_packet, 5);
}

void REJ_send(int fd, uint Nr)
{
    unsigned char BCC = (A_EMMITER_RECEIVER & C_REJ(Nr));
    unsigned char REJ_packet[] = {FLAG, A_EMMITER_RECEIVER, C_REJ(Nr), BCC, FLAG, '\0'};
    write(fd, REJ_packet, 5);
}

// Alarm function handler
void alarmHandler(int signal)
{
    alarmEnabled = FALSE;
}