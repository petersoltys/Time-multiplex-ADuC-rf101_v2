#include "uwbpacketclass.hpp"

/*****************************************************************************************************************/

uwbPacketTx::uwbPacketTx(int radarId) : endingChar('$'), rounder(100.0)
{
    radarID = radarId;
    packetCount = 0;
    radarTime = 0;

    packet = NULL;
    packetLength = 0;

    crc_tab16_init = FALSE;
}

void uwbPacketTx::generatePacket(float *data, int data_count)
{
    // allocate enough array space
    deleteLastPacket();
    packet = new unsigned char[data_count*2*3+10+1]; // plus one is because of ending character

    std::bitset<8> b_radarID(radarID);
    std::bitset<8> b_radarTime(radarTime);
    std::bitset<8> b_packetNumber(packetCount);

    std::bitset<8> b_constant(15);

    unsigned char ch; // temp char

    // convert radarID
    ch = makeCorrection(((b_radarID>>4)&b_constant).to_ulong());
    packet[packetLength++] = ch;
    ch = makeCorrection((b_radarID&b_constant).to_ulong());
    packet[packetLength++] = ch;

    // convert radarTime
    ch = makeCorrection(((b_radarTime>>4)&b_constant).to_ulong());
    packet[packetLength++] = ch;
    ch = makeCorrection((b_radarTime&b_constant).to_ulong());
    packet[packetLength++] = ch;

    // convert packetCount
    ch = makeCorrection(((b_packetNumber>>4)&b_constant).to_ulong());
    packet[packetLength++] = ch;
    ch = makeCorrection((b_packetNumber&b_constant).to_ulong());
    packet[packetLength++] = ch;

    std::bitset<12> b_val_constant(15);
    for(int i=0; i<data_count; i++)
    {
        // convert float to int
        int num = data[i]*rounder;
        std::bitset<12> b_temp(num);

        // convert x and y to 3 chars since values can be passed as 3*4 bit long = 12 bits
        for(int j=8; j>=0; j=j-4)
        {
            ch = makeCorrection(((b_temp>>j)&b_val_constant).to_ulong());

            packet[packetLength++] = ch;
        }
    }

    // calculate CRC
    unsigned short crc = 0;
    for(int k = 0; k<packetLength; k++)
        crc = update_crc_16(crc, packet[k]);

    std::bitset<16> b_crc(crc);

    std::bitset<16> crc_constant(15); // constant for extraction 4 bits from 16 bit long bitstream
    for(int j=12; j>=0; j=j-4)
    {
        ch = makeCorrection(((b_crc&crc_constant)>>j).to_ulong());
        packet[packetLength++] = ch;
    }

    packet[packetLength++] = endingChar;

    incrementPacketCount();
}

unsigned char uwbPacketTx::makeCorrection(unsigned char ch)
{
    int ascii = (int)(ch)+48;

    if(ascii>57)
    {
        ascii += 7;
    }

    return (unsigned char)(ascii);
}

unsigned short uwbPacketTx::update_crc_16( unsigned short crc, char c )
{

    unsigned short tmp, short_c;

    short_c = 0x00ff & (unsigned short) c;

    if ( ! crc_tab16_init ) this->init_crc16_tab();

    tmp =  crc       ^ short_c;
    crc = (crc >> 8) ^ crc_tab16[ tmp & 0xff ];

    return crc;

}

void uwbPacketTx::init_crc16_tab()
{
        int i, j;
        unsigned short crc, c;

        for (i=0; i<256; i++) {

            crc = 0;
            c   = (unsigned short) i;

            for (j=0; j<8; j++) {

                if ( (crc ^ c) & 0x0001 ) crc = ( crc >> 1 ) ^ P_16;
                else                      crc =   crc >> 1;

                c = c >> 1;
            }

            crc_tab16[i] = crc;
        }

        crc_tab16_init = TRUE;
}

void uwbPacketTx::deleteLastPacket()
{
    if(packet!=NULL)
    {
        delete[] packet;
    }
    packetLength = 0;
    packet = NULL;
}

void uwbPacketTx::incrementPacketCount()
{
    ++packetCount;
    // packet count sending in packet must be 8 bit long, maximum is therefore 255
    if(packetCount>255) packetCount = 0;
}

void uwbPacketTx::decrementPacketCount()
{
    --packetCount;
    // packet count sending in packet must be 8 bit long, maximum is therefore 255
    if(packetCount<0) packetCount = 255;
}

/**********************************************************************************************************************/


uwbPacketRx::uwbPacketRx() : endingChar('$'), rounder(100.0), buffer_size(128), c_buffer_size(10)
{
    radarID = radarTime = packetCount = dataCount = 0;
    data = NULL;

    buffer_read_pointer = buffer_stack_pointer = buffer_rs232_read_size = buffer_packet_size = 0;

    buffer = new unsigned char[buffer_size];
    c_buffer = new unsigned char[c_buffer_size];
}

void uwbPacketRx::readPacket()
{
    int ch;
    int stack_pointer = 0;

    std::bitset<8> b_temp(0);
    std::bitset<12> val_temp(0);

    // read radar ID
    ch = (int)(removeCorrection(packet[stack_pointer++]));
    b_temp |= ch;
    b_temp <<= 4;
    ch = (int)(removeCorrection(packet[stack_pointer++]));
    b_temp |= ch;
    radarID = b_temp.to_ulong();
    b_temp &= 0; // zero all bits

    // read radar Time
    ch = (int)(removeCorrection(packet[stack_pointer++]));
    b_temp |= ch;
    b_temp <<= 4;
    ch = (int)(removeCorrection(packet[stack_pointer++]));
    b_temp |= ch;
    radarTime = b_temp.to_ulong();
    b_temp &= 0; // zero all bits

    // read packet Count
    ch = (int)(removeCorrection(packet[stack_pointer++]));
    b_temp |= ch;
    b_temp <<= 4;
    ch = (int)(removeCorrection(packet[stack_pointer++]));
    b_temp |= ch;
    packetCount = b_temp.to_ulong();
    b_temp &= 0; // zero all bits

    // go throught all availible data
    int internal_counter = 0; // if reaches 3, we have all 12 bits, therefore we have one value
    int value_counter = 0; // index of buffer

    // create data array as long as needed
    dataCount = packetLength-8-1; // packet length contains CRC, radarID, radarTime, packetCount and endingChar bytes
    data = new float[dataCount];
    dataCount /= 3; // since real data are represented by 3 chars, real data count is 3 times smaller as character count

    while(stack_pointer<(packetLength-3)) // last three bytes are not values, but CRC and ending char. StackPointer holds index position!
    {
        // read 4 bits
        ch = (int)(removeCorrection(packet[stack_pointer++]));

        val_temp |= ch;

        if(++internal_counter<3)
            val_temp <<= 4; // if counter is less than 3 we still need some bits to complete value
        else
        {
            // all bits are read
            internal_counter = 0;
            data[value_counter++] = ((float)(val_temp.to_ulong()))/rounder;

            val_temp &= 0; // reset temporary bit stream
        }
    }
}

void uwbPacketRx::deleteLastPacket()
{
    if(packet!=NULL) delete packet;
    packet = NULL;
}

int uwbPacketRx::removeCorrection(unsigned char ch)
{
    int ascii = (int)(ch);
    if(ascii>64) ascii-=7;

    ascii -= 48;

    return ascii;
}

unsigned short uwbPacketRx::update_crc_16( unsigned short crc, char c )
{

    unsigned short tmp, short_c;

    short_c = 0x00ff & (unsigned short) c;

    if ( ! crc_tab16_init ) this->init_crc16_tab();

    tmp =  crc       ^ short_c;
    crc = (crc >> 8) ^ crc_tab16[ tmp & 0xff ];

    return crc;

}

void uwbPacketRx::init_crc16_tab()
{
        int i, j;
        unsigned short crc, c;

        for (i=0; i<256; i++) {

            crc = 0;
            c   = (unsigned short) i;

            for (j=0; j<8; j++) {

                if ( (crc ^ c) & 0x0001 ) crc = ( crc >> 1 ) ^ P_16;
                else                      crc =   crc >> 1;

                c = c >> 1;
            }

            crc_tab16[i] = crc;
        }

        crc_tab16_init = TRUE;
}
