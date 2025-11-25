
#include "../include/state_machine.h"

unsigned char A;
unsigned char C;

STATES m_state = START;

uint byte_counter = 0;
uint BCC2_accumulator = 0xFF;
uint Ns = 0;  //sequence ID
uint I;

RETURN_CODE state_machine_SET(unsigned char byte);
RETURN_CODE state_machine_UA(unsigned char byte);
RETURN_CODE state_machine_RR(unsigned char byte);
RETURN_CODE state_machine_REJ(unsigned char byte);
RETURN_CODE state_machine_I(unsigned char byte);
RETURN_CODE state_machine_DISC(unsigned char byte);


RETURN_CODE state_machine(unsigned char byte){
    //printf("m_state: %d\n", m_state);
    //printf("byte: "BYTE_TO_BINARY_PATTERN, BYTE_TO_BINARY(byte));
    //printf("\n");
    

    switch (m_state)
    {
    case START:   //FIRST STATE
        if (byte == FLAG) m_state = FLAG_RCV; //else m_state remains START
        break;

    case FLAG_RCV:   //SECOND STATE

        byte_counter = 0;          //resets byte counter
        BCC2_accumulator = 0xFF;             //resets bcc2 acumulator

        if (byte == FLAG){ 
            m_state = FLAG_RCV;
        }
        else if (byte == A_EMMITER_RECEIVER){
            A = A_EMMITER_RECEIVER;
            m_state = A_RCV;
        }
        else m_state = START;
        break;

    case A_RCV:   //THIRD STATE
        if (byte == FLAG) m_state = FLAG_RCV;
        else if (byte == C_SET){
            C = C_SET;
            m_state = C_RCV_SET;
        }
        else if (byte == C_UA){
            C = C_UA;
            m_state = C_RCV_UA;
        }
        else if (byte == C_0_RR){
            C = C_0_RR;
            m_state = C_RCV_RR;
            Ns = 0;
        }
        else if (byte == C_1_RR){
            C = C_1_RR;
            m_state = C_RCV_RR;
            Ns = 1;
        }
        else if (byte == C_DISC){
            C = C_DISC;
            m_state = C_RCV_DISC;
        }
        else if (byte == C_0_I){
            C = C_0_I;
            m_state = C_RCV_I;
            Ns = 0;
        }
        else if (byte == C_1_I){
            C = C_1_I;
            m_state = C_RCV_I;
            Ns = 1;
        }
        else m_state = START;
        break;
    


    //REDIRECTS TO SET STATES
    case C_RCV_SET: 
    case BCC_OK_SET: 
        return state_machine_SET(byte);
        break;

    //REDIRECTS TO UA STATES
    case C_RCV_UA: 
    case BCC_OK_UA: 
        return state_machine_UA(byte);
        break;

     //REDIRECTS TO I STATES
    case C_RCV_I:
    case BCC1_I:
    case BCC2_I:
    case S_I:
        return state_machine_I(byte);
        break;

    //REDIRECTS TO RR STATES
    case C_RCV_RR: 
    case BCC_OK_RR: 
        return state_machine_RR(byte);
        break;

    //REDIRECTS TO REJ STATES
    case C_RCV_REJ: 
    case BCC_OK_REJ: 
        return state_machine_REJ(byte);
        break;


    //REDIRECTS TO DISC STATES
    case C_RCV_DISC:
    case BCC_OK_DISC:
        return state_machine_DISC(byte);
        break;

    default:
        m_state = START;
        break;
    }


    return UNDEFINED;
}

RETURN_CODE state_machine_SET(unsigned char byte){
    switch (m_state)
    {
    case C_RCV_SET: //FOURTH STATE
        if (byte == FLAG) m_state = FLAG_RCV;
        else if ((A & C ) == byte) m_state = BCC_OK_SET; //BCC_OK
        else m_state = START;
        break;

    case BCC_OK_SET: //FIFTH STATE
        if (byte == FLAG){
            //PACKET CORRECTLY READ, RETURNING POSITIVE
            m_state = START;  
            return SET_CODE;
        }
        else {
            m_state = START;
        }
        break;

    default:
        m_state = START;
        break;
    }

    return UNDEFINED;
}

RETURN_CODE state_machine_UA(unsigned char byte){
    switch (m_state)
    {
    case C_RCV_UA: //FOURTH STATE
        if (byte == FLAG) m_state = FLAG_RCV;
        else if ((A & C ) == byte) m_state = BCC_OK_UA; //BCC_OK
        else m_state = START;
        break;

    case BCC_OK_UA: //FIFTH STATE
        if (byte == FLAG){
            //PACKET CORRECTLY READ, RETURNING POSITIVE
            m_state = START;  
            return UA_CODE;
        }
        else {
            m_state = START;
        }
        break;

    default:
        m_state = START;
        break;
    }

    return UNDEFINED;
}

RETURN_CODE state_machine_DISC(unsigned char byte){
    switch (m_state)
    {
    case C_RCV_DISC: //FOURTH STATE
        if (byte == FLAG) m_state = FLAG_RCV;
        else if ((A & C ) == byte) m_state = BCC_OK_DISC; //BCC_OK
        else m_state = START;
        break;

    case BCC_OK_DISC: //FIFTH STATE
        if (byte == FLAG){
            //PACKET CORRECTLY READ, RETURNING POSITIVE
            m_state = START;  
            return DISC_CODE;
        }
        else {
            m_state = START;
        }
        break;

    default:
        m_state = START;
        break;
    }

    return UNDEFINED;
}

RETURN_CODE state_machine_I(unsigned char byte){
    switch (m_state)
    {

    case C_RCV_I: //FIFTH STATE
        if (byte == FLAG) m_state = FLAG_RCV;
        else if ((A & C ) == byte) m_state = BCC1_I; //BCC_OK
        else m_state = START;
        break;
        
    case BCC1_I:
        if (byte <= INFORMATION_FIELD_SIZE && byte > 0){
            m_state = S_I;
            I = byte;
            return S_CODE;
        }
        else if (byte == FLAG) m_state = FLAG_RCV;
        else m_state = START;
        break;

    case S_I: //FIFTH STATE
        if (byte_counter == I) { 

            if (byte == BCC2_accumulator){
                m_state = BCC2_I;
            }
            else if (byte == FLAG) m_state = FLAG_RCV;
            else m_state = START;
            break;  
        }

        BCC2_accumulator = BCC2_accumulator & byte;

        byte_counter++;            //increases byte counter
        return RCV_DATA;
        break;

    case BCC2_I: //SIXTH STATE
        if (byte == FLAG){
            m_state = START;  
            
            if (Ns == 0) return I_0_CODE;
            else return I_1_CODE;
        }
        else {
            m_state = START;
        }
        break;


    default:
        m_state = START;
        break;
    }

    return UNDEFINED;
}

RETURN_CODE state_machine_RR(unsigned char byte){
    switch (m_state)
    {
    case C_RCV_RR: //FOURTH STATE
        if (byte == FLAG) m_state = FLAG_RCV;
        else if ((A & C) == byte) m_state = BCC_OK_RR; //BCC_OK
        else m_state = START;
        break;

    case BCC_OK_RR: //FIFTH STATE
        if (byte == FLAG){
            //PACKET CORRECTLY READ, RETURNING POSITIVE
            m_state = START;  
            if (Ns == 0) return RR_0_CODE;
            else if(Ns == 1) return RR_1_CODE;
        }
        else {
            m_state = START;
        }
        break;

    default:
        m_state = START;
        break;
    }

    return UNDEFINED;
}

RETURN_CODE state_machine_REJ(unsigned char byte){
    switch (m_state)
    {
    case C_RCV_REJ: //FOURTH STATE
        if (byte == FLAG) m_state = FLAG_RCV;
        else if ((A & C) == byte) m_state = BCC_OK_REJ; //BCC_OK
        else m_state = START;
        break;

    case BCC_OK_REJ: //FIFTH STATE
        if (byte == FLAG){
            //PACKET CORRECTLY READ, RETURNING POSITIVE
            m_state = START;  
            if (Ns == 0) return REJ_0_CODE;
            else if(Ns == 1) return REJ_1_CODE;
        }
        else {
            m_state = START;
        }
        break;

    default:
        m_state = START;
        break;
    }

    return UNDEFINED;
}
