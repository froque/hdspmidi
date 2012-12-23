#include "relay.h"
#include <stdlib.h>
#include <cstring>

using namespace LibSerial;

Relay::Relay(void){
    port_name = "";
    start_monitoring();
}

Relay::Relay(std::string port)
{
    port_name = "";
    open(port);
    start_monitoring();
}

void Relay::start_monitoring(void){
    /* Create the udev object */
    udev_s = udev_new();
    if (!udev_s) {
        printf("Can't create udev\n");
        exit(EXIT_FAILURE);
    }

    mon_s = udev_monitor_new_from_netlink(udev_s, "udev");
    if (!mon_s){
        printf("Can't create mon\n");
        exit(EXIT_FAILURE);
    }
    if ( udev_monitor_filter_add_match_subsystem_devtype(mon_s, "tty", NULL) < 0){
        printf("Can't create filter to udev_monitor\n");
        exit(EXIT_FAILURE);
    }
    if (udev_monitor_enable_receiving(mon_s) <0 ){
        printf("Can't start monitoring udev\n");
        exit(EXIT_FAILURE);
    }
}

void Relay::open(std::string port){
    if (stream.IsOpen()){
        return;
    }
    port_name = port;
    stream.Open(port);
    stream.SetBaudRate( SerialStreamBuf::BAUD_9600 ) ;
    stream.SetCharSize( SerialStreamBuf::CHAR_SIZE_8 ) ;
    stream.SetNumOfStopBits(1);
    stream.SetParity(SerialStreamBuf::PARITY_NONE);
}

void Relay::open(){
    // test if port_name was defined
    if (port_name.compare("") != 0){
        this->open(port_name);
    }
}

void Relay::close(){
    stream.Close();
}

Relay::~Relay(){
    off(); // turn the lights off when leaving the room :P
    close();
    udev_monitor_unref(mon_s);
    udev_unref(udev_s);
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
    if (isOpen()){
        stream.write(command, BUFFER_SIZE) ;
    }
}

int Relay::get_fd(void){
    return udev_monitor_get_fd(mon_s);
}


bool Relay::compare_names(struct udev_device *dev_s){
    struct udev_list_entry *list;
    struct udev_list_entry *dev_list_entry;
    list = udev_device_get_devlinks_list_entry(dev_s);

    // compare real path in /dev
    // commented, because another device can get this name
//    if (strcmp(udev_device_get_devnode(dev_s),name) == 0 ){
//        return true;
//    }

    // compare symlinks in /dev
    udev_list_entry_foreach(dev_list_entry,list){
        if (port_name.compare(udev_list_entry_get_name(dev_list_entry)) == 0){
            return true;
        }
    }
    return false;
}

// exit boolean to indicate bridge class to restore()
bool Relay::reconnect(){
    struct udev_device *dev_s = udev_monitor_receive_device(mon_s);
    if (dev_s != NULL && compare_names(dev_s)){
        if (strcmp(udev_device_get_action(dev_s),"remove") == 0){
            close();
            return false;
        } else {
            if (strcmp(udev_device_get_action(dev_s),"add") == 0){
                open();
                return true;
            }
        }
    }
    return false;
}
