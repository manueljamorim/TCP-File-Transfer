// Application layer protocol implementation

#include "application_layer.h"

void applicationLayer(const char *serialPort, const char *role, int baudRate,
                      int nTries, int timeout, const char *filename)
{   
    LinkLayerRole ll_role;
    LinkLayer linkLayer_control;
    linkLayer_control.baudRate = baudRate;
    linkLayer_control.nRetransmissions = nTries;
    linkLayer_control.timeout = timeout;
    strcpy(linkLayer_control.serialPort, serialPort);

    int fd;

    /*--------------------------------------   RECEIVER  -----------------------------------------------*/
    if (strcmp(role,"rx") == 0){
        printf("\n> Initializing Receiver\n");

        ll_role = LlRx;
        linkLayer_control.role = ll_role;

        FILE *fp;
        fd = llopen(linkLayer_control);
        if (fd < 0){
            printf("\n> Error opening port\n");
        }
        unsigned char data[INFORMATION_FIELD_SIZE];

        int output;
        int counter = 0; //count sequence number
        output = llread(data);
        int writable_bytes, readable_bytes;
        char received_file_name[200] = "received.gif";
        int file_size_counter = 0;
        int CONTROL_RECEIVED = 0;
        
        union
        {
            unsigned int integer;
            unsigned char byte[4];
        } file_size;

        //output will be -1 when the receiver detects the DISC packet
        while (output >= 0)
        {
            file_size_counter += output - INFORMATION_FIELD_CONTROL_BYTES;
            
            if (output != 0) {
                if(data[0]==1){//Data Application Packet
                
                    if(!CONTROL_RECEIVED){
                        printf("> Control Packet not received yet\n");
                        return;
                    }

                    ///Remove Application Packet Flags before writting to file
                    writable_bytes = data[2] * 256 + data[3];

                    if(data[0] != DATA_CONTROL_FIELD || data[1] != counter || writable_bytes + INFORMATION_FIELD_CONTROL_BYTES != output){
                        printf("> Application Packet is corrupted, exiting\n");
                        return;
                    }

                    counter = (counter+1)%255;
                    fwrite(data+4, 1, output-4, fp);
                }
                
                else{ //Control Application Packet
                    if(CONTROL_RECEIVED){
                        printf("Control Packet already received before\n");
                        return;
                    } 
                    
                    CONTROL_RECEIVED = 1;
                    printf("> Control packet received \n");

                    /*
                    printf("Application initial Control Packet Received: ");
                    for(int i=0; i<output; i++)
                        printf("%02X ", data[i]);
                    
                    printf("\n");
                    */

                    int i = 1;
                    while (i<output)
                    {
                        if (data[i] == 0){
                            //File size
                            readable_bytes = data[i+1];
                            
                            for(int j=0; j<readable_bytes; j++){
                                file_size.byte[j] = data[i+1+j];
                                printf("%02X \n", data[i+1+j]);
                            }
                            printf("\nfile size: %d\n", file_size.integer);
                        }
                        else if (data[i] == 1){
                            //file name
                            readable_bytes = data[i+1];
                            memcpy(received_file_name, data+i+2, readable_bytes);
                        }
                        else {
                            printf("> Application Control Packet is corrupted, exiting\n");  
                            return; 
                        }
                        
                        i += 2 + readable_bytes;                    
                    }

                    fp = fopen(received_file_name, "w");

                    if (fp == NULL)
                    {
                        printf("> Couldn't open file\n");
                        llclose(fd, 0);
                        return;
                    }
                    
                }
            }
            output = llread(data);
        }
        printf("\n> Received file size: %d\n", file_size_counter);
        fclose(fp);
    }



    /*--------------------------------------   TRANSMITER   -----------------------------------------------*/
    else if (strcmp(role,"tx") == 0){
        printf("\n> Initializing Transmitter\n");

        ll_role = LlTx;
        linkLayer_control.role = ll_role;

        fd = llopen(linkLayer_control);
        if (fd >= 0){

            FILE *fp = fopen(filename, "r");

            if (fp == NULL){
                printf("> couldn't open file\n");
                exit(1);
            }

            //count file size 
            fseek(fp, 0, SEEK_END);           //set STREAM position to end of file
            int file_len = ftell(fp);         //get STREAM pointer position
            rewind(fp);                       //reset STREAM pointer
            uint n_of_packets = file_len/DATA_FIELD_SIZE;  //total bytes / number of bytes per packet
            uint last_packet_size = file_len%DATA_FIELD_SIZE; 

            int readable_bytes = DATA_FIELD_SIZE;
            unsigned char data[INFORMATION_FIELD_SIZE] = {0};

            //Send Application Control Packet
            char transmitted_file_name[100] = {0};
            strcpy(transmitted_file_name, filename);
            transmitted_file_name[strlen(transmitted_file_name)-strlen(".gif")]='\0';
            strcat(transmitted_file_name, "_received.gif");
            
            //Determine how many bytes are needed to transmit the file size and file name
            int len_field_size = (int) ceil( (log2( (double) file_len) +1)/8);
            int name_field_size = strlen(transmitted_file_name);
            
            //Assemble packet
            unsigned char ControlPacket[200];
            ControlPacket[0] =  START_CONTROL_FIELD;
            ControlPacket[1] =  0;
            ControlPacket[2] =  len_field_size;

            //Union makes it possible to use memcpy and turn the binary data into an int
            union
            {
                unsigned int integer;
                unsigned char byte[4];
            } size_obj;
            size_obj.integer = file_len;

            int j= 3;
            for(int i=(len_field_size-1); i>=0; i--){
                printf("%02X \n", size_obj.byte[i]);
                memcpy(ControlPacket + j, &(size_obj.byte[i]), 1);
                j++;
            }
            
            //Assemble the 2nd part of the control packet's data that refers to the file_name
            ControlPacket[3+len_field_size] =  1;
            ControlPacket[4+len_field_size] =  name_field_size;
            memcpy(ControlPacket+5+len_field_size, transmitted_file_name, name_field_size);

            int ControlPacket_size = 5 + len_field_size + name_field_size;
            printf("Application initial Control Packet: ");
            for(int i=0; i<ControlPacket_size; i++)
                printf("%02X ", ControlPacket[i]);
            
            //Send the control packet
            llwrite(ControlPacket, ControlPacket_size);

            //Send the data packets
            for (uint i = 0; i < n_of_packets; i++)
            {
                //if it's the last packet update "readable bytes", to transmit it to the receiver
                if (i == n_of_packets - 1) {
                    printf("\n> last packet being sent\n");
                    readable_bytes = last_packet_size;
                }
                if (fread(data+INFORMATION_FIELD_CONTROL_BYTES, sizeof(unsigned char), readable_bytes, fp) <= 0 ) break; //4
                
                // Create Application Packet
                data[0] = DATA_CONTROL_FIELD;
                data[1] = i%255;
                data[2] = readable_bytes/256;
                data[3] = readable_bytes%256;

                printf("> Packet Sequence Nº: %d\n", data[1]);

                if (llwrite(data, INFORMATION_FIELD_CONTROL_BYTES+readable_bytes) == -1){
                    printf("> error, exiting\n");
                    fclose(fp);
                    llclose(fd, 1);
                    exit(-1);
                    break;
                }
            }
            sleep(1);
            fclose(fp);
        }
        else{
            printf("\n> Error opening port\n");
        }
    }
    else{
        printf("> wrong role argument check usage\n");
    }


    /*------------------------------------------  BOTH TRANSMITTER AND RECEIVER RUN THIS PART OF THE CODE   ---------------------------------------------*/
    if(linkLayer_control.role == LlTx) llend(linkLayer_control.role);
    llclose(fd, 1);
    printf("\n> Exiting...\n");
    return;
}


