#ifndef MIDICONTROLLER_H
#define MIDICONTROLLER_H

#include <alsa/asoundlib.h>
#include "Channel.h"

#define CC_MAX 127
#define CC_VOL 7
#define CC_PAN 10
#define CC_DOWN_ROW 11
#define CC_UP_ROW 14
#define CC_PAN_CENTER 16

#define LOWER_DB 65.0

class MidiController
{
public:
    MidiController();
    ~MidiController();
    void restore_midi( Channels *ch);
    void send_midi_CC(int chn, int param,int value );
    void connect_ports();
    void parse_ports_in( const char *arg);
    void parse_ports_out( const char *arg);
    void vegas();
    snd_seq_t *seq;
private:
    void parse_ports(snd_seq_addr_t *port, const char *arg);
    snd_seq_addr_t ports_out;
    snd_seq_addr_t ports_in;
    int receiver;
    int sender;
};

#endif // MIDICONTROLLER_H
