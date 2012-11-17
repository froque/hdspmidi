#ifndef RELAY_H
#define RELAY_H
#include <SerialStream.h>

#define BUFFER_SIZE 3

class Relay{
private:
    LibSerial::SerialStream stream;
    void send(char command[BUFFER_SIZE]);

public:
    Relay();
    Relay(std::string port);
    ~Relay();
    void open(std::string port);
    bool isOpen();
    void on();
    void off();
};
#endif // RELAY_H
