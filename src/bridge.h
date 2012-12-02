#ifndef BRIDGE_H
#define BRIDGE_H

#include "HDSPMixerCard.h"
#include "Channel.h"
#include "midicontroller.h"
#include "relay.h"

class Bridge
{
public:
    Bridge();
    ~Bridge();
    void vegas();
    void restore(void);
    void send_control(int dst, struct channel ch, int left_value, int right_value);
    void control_normal(int dst, struct channel ch);
    void control_solos();
    void control_onair();
    void dump_event(const snd_seq_event_t *ev);

    HDSPMixerCard *hdsp_card;
    Channels channels;
    MidiController midicontroller;
    Relay relay;
    int phones;
    int main;
    int monitors;
};

#endif // BRIDGE_H
