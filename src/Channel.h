#ifndef CHANNEL_H
#define CHANNEL_H

#include <libconfig.h++>

#define CHARMAX 50
#define CH_MAX 10

struct channel {
public:
    int idx;
    char name[CHARMAX];
    bool mute;
    bool solo;
    bool stereo;
    bool input;
    int left_map;
    int right_map;
    double volume;
    double balance;
};



class Channels {
public:
    bool read(libconfig::Config *cfg);
    bool save(libconfig::Config *cfg);
    void print();
    Channels(){num=0;}
    int getNum(){return num;}
    struct channel channels_data[CH_MAX];
private:
    int num;
};

#endif // CHANNEL_H
