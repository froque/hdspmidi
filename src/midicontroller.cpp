#include "midicontroller.h"
#include <iostream>
#include "HDSPMixerCard.h"
#include <cmath>

using namespace std;

MidiController::MidiController(){
    seq = NULL;

    /* open sequencer */
    snd_seq_open(&seq, "default", SND_SEQ_OPEN_DUPLEX, 0);

    /* set our client's name */
    snd_seq_set_client_name(seq, "hdspmidi");

    receiver = snd_seq_create_simple_port(seq, "receiver",
                                     SND_SEQ_PORT_CAP_WRITE |
                                     SND_SEQ_PORT_CAP_SUBS_WRITE,
                                     SND_SEQ_PORT_TYPE_MIDI_GENERIC |
                                     SND_SEQ_PORT_TYPE_APPLICATION);

    sender = snd_seq_create_simple_port(seq, "sender",
                                     SND_SEQ_PORT_CAP_READ |
                                     SND_SEQ_PORT_CAP_SUBS_READ,
                                     SND_SEQ_PORT_TYPE_APPLICATION);

    snd_seq_nonblock(seq, 1);

}

MidiController::~MidiController(){
    snd_seq_close(seq);
}

void MidiController::restore_midi( Channels *ch){
    for(int k=0; k<ch->getNum(); k++){
        double linear;
        if(ch->channels_data[k].volume == 0){
            linear = 0;
        } else {
            double dB = 20*log10(ch->channels_data[k].volume/(ZERO_DB*1.0));
            linear = (dB + LOWER_DB)*CC_MAX/LOWER_DB;
        }

        send_midi_CC(k, CC_VOL,linear);
        send_midi_CC(k, CC_PAN,ch->channels_data[k].balance *CC_MAX);
        send_midi_CC(k, CC_DOWN_ROW,ch->channels_data[k].mute? CC_MAX:0);
        send_midi_CC(k, CC_UP_ROW,ch->channels_data[k].solo? CC_MAX:0);
    }
}

void MidiController::send_midi_CC(int chn, int param, int value )  {
    snd_seq_event_t ev;
    snd_seq_ev_clear(&ev);
    snd_seq_ev_set_subs(&ev);
    snd_seq_ev_set_controller(&ev, chn, param, value);
    snd_seq_ev_set_source(&ev,sender);
    snd_seq_ev_set_direct(&ev);
    int err = snd_seq_event_output_direct(seq, &ev);
    if (err < 0){
        cerr << "Could not send MIDI control change: " << snd_strerror(err) << endl;
    }
}

bool MidiController::connect_ports(){
    int err;

    err = snd_seq_connect_from(seq, receiver, ports_out.client, ports_out.port);
    if (err < 0){
        return false;
    }

    err = snd_seq_connect_to(seq, sender, ports_in.client, ports_in.port);
    if (err < 0){
        return false;
    }
    return true;
}

void MidiController::parse_ports_in( const char *arg){
    parse_ports(&ports_in,arg);
}

void MidiController::parse_ports_out( const char *arg){
    parse_ports(&ports_out,arg);
}

void MidiController::parse_ports(snd_seq_addr_t *port, const char *arg){
    int err =  snd_seq_parse_address(seq, port, arg);
    if (err < 0){
        throw std::exception();
    }
}

void MidiController::vegas(){
    bool b = false;
    for (double phase = 0; phase < 5*2*3.14 ; phase +=0.2 ){
        for (int k=0;k<8;k++){
            send_midi_CC(k,CC_VOL,CC_MAX/2.0 * sin(2*3.14*k/8 + phase) + CC_MAX/2.0);
            send_midi_CC(k,CC_PAN,CC_MAX/2.0 * sin(2*3.14*k/8 + phase) + CC_MAX/2.0);
            send_midi_CC(k,CC_DOWN_ROW,b?CC_MAX:0);
            send_midi_CC(k,CC_UP_ROW,b?0:CC_MAX);
            b = !b;
            usleep(5000);
        }
        b = !b;
    }
}
