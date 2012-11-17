#include "relay.h"

using namespace LibSerial;

Relay::Relay(void){
}

Relay::Relay(std::string port)
{
    open(port);
}

void Relay::open(std::string port){
    stream.Open(port);
    stream.SetBaudRate( SerialStreamBuf::BAUD_9600 ) ;
    stream.SetCharSize( SerialStreamBuf::CHAR_SIZE_8 ) ;
    stream.SetNumOfStopBits(1);
    stream.SetParity(SerialStreamBuf::PARITY_NONE);
}

Relay::~Relay(){
    off(); // turn the lights off when leaving the room :P
    stream.Close();
}

void Relay::on(){
    char command[BUFFER_SIZE] = {0xFF,0x01,0x01};
    send(command);
}

void Relay::off(){
    char command[BUFFER_SIZE] = {0xFF,0x01,0x00};
    send(command);
}

bool Relay::isOpen(){
    return stream.IsOpen();
}

void Relay::send(char command[BUFFER_SIZE]){
    stream.write(command, BUFFER_SIZE) ;
}
