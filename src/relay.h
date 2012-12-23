#ifndef RELAY_H
#define RELAY_H
#include <SerialStream.h>
#include <libudev.h>

#define BUFFER_SIZE 3

class Relay{
private:
    LibSerial::SerialStream stream;
    void send(char command[BUFFER_SIZE]);
    struct udev_monitor *mon_s;
    struct udev *udev_s;
    void start_monitoring(void);
    std::string port_name;
    bool compare_names(struct udev_device *dev_s);

public:
    Relay();
    Relay(std::string port);
    ~Relay();
    void open(std::string port);
    void open();
    void close();
    bool isOpen();
    void on();
    void off();
    int get_fd(void);
    bool reconnect();
};
#endif // RELAY_H
