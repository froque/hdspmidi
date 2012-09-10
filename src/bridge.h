#ifndef BRIDGE_H
#define BRIDGE_H

#include "HDSPMixerCard.h"
#include "Channel.h"
#include "midicontroller.h"

class Bridge
{
public:
    Bridge();
    ~Bridge();
    void restore();
    void send_control(int dst, struct channel ch, int left_value, int right_value);
    void control_normal(int dst, struct channel ch);
    void control_solos();
    void dump_event(const snd_seq_event_t *ev);

    HDSPMixerCard *hdsp_card;
    Channels channels;
    MidiController midicontroller;
    int phones;
    int main;
};

#endif // BRIDGE_H
