#include "midicontroller.h"
#include <iostream>
#include "HDSPMixerCard.h"

using namespace std;

// fixme: is this function really needed?
/* prints an error message to stderr, and dies */
static void fatal(const char *msg, ...){
    va_list ap;

    va_start(ap, msg);
    vfprintf(stderr, msg, ap);
    va_end(ap);
    fputc('\n', stderr);
    exit(EXIT_FAILURE);
}

/* memory allocation error handling */
static void check_mem(void *p){
    if (!p)
        fatal("Out of memory");
}

/* error handling for ALSA functions */
static void check_snd(const char *operation, int err){
    if (err < 0)
        fatal("Cannot %s - %s", operation, snd_strerror(err));
}

MidiController::MidiController()
{
    seq = NULL;
    ports_out = NULL;
    ports_in = NULL;
    port_count_out = 0;
    port_count_in = 0;
    int err;

    /* open sequencer */
    err = snd_seq_open(&seq, "default", SND_SEQ_OPEN_DUPLEX, 0);
    check_snd("open sequencer", err);

    /* set our client's name */
    err = snd_seq_set_client_name(seq, "hdspmidi");
    check_snd("set client name", err);

    err = snd_seq_create_simple_port(seq, "receiver",
                                     SND_SEQ_PORT_CAP_WRITE |
                                     SND_SEQ_PORT_CAP_SUBS_WRITE,
                                     SND_SEQ_PORT_TYPE_MIDI_GENERIC |
                                     SND_SEQ_PORT_TYPE_APPLICATION);
    check_snd("create port", err);
    receiver = err;


    err = snd_seq_create_simple_port(seq, "sender",
                                     SND_SEQ_PORT_CAP_READ |
                                     SND_SEQ_PORT_CAP_SUBS_READ,
                                     SND_SEQ_PORT_TYPE_APPLICATION);
    check_snd("create port", err);
    sender = err;

    err = snd_seq_nonblock(seq, 1);
    check_snd("set nonblock mode", err);
}

MidiController::~MidiController()
{
    snd_seq_close(seq);
}

void MidiController::restore_midi( Channels *ch){
    for(int k=0; k<ch->getNum(); k++){
        send_midi_CC(k, CC_VOL,ch->channels_data[k].volume /RME_MAX * CC_MAX);
        send_midi_CC(k, CC_PAN,ch->channels_data[k].balance *CC_MAX);
        send_midi_CC(k, CC_DOWN_ROW,ch->channels_data[k].mute);
        send_midi_CC(k, CC_UP_ROW,ch->channels_data[k].solo);
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
        cerr<<"Could not send MIDI control change: "<<snd_strerror(err)<<endl;
    }
}

void MidiController::connect_ports(){
    int i, err;

    for (i = 0; i < port_count_out; ++i) {
        err = snd_seq_connect_from(seq, receiver, ports_out[i].client, ports_out[i].port);
        if (err < 0)
            fatal("Cannot connect from port %d:%d - %s",
                  ports_out[i].client, ports_out[i].port, snd_strerror(err));
    }
    for (i = 0; i < port_count_in; ++i) {
        err = snd_seq_connect_to(seq, sender, ports_in[i].client, ports_in[i].port);
        if (err < 0)
            fatal("Cannot connect from port %d:%d - %s",
                  ports_in[i].client, ports_in[i].port, snd_strerror(err));
    }
}

void MidiController::parse_ports_in( const char *arg){
    port_count_in =  parse_ports(&ports_in,arg);
}

void MidiController::parse_ports_out( const char *arg){
    port_count_out = parse_ports(&ports_out,arg);
}

/* parses one or more port addresses from the string */
int MidiController::parse_ports(snd_seq_addr_t **ports, const char *arg){
    char *buf, *s, *port_name;
    int err;
    int port_count =0;

    cout << "parse_ports "<< arg << endl;

    /* make a copy of the string because we're going to modify it */
    buf = strdup(arg);
    check_mem(buf);

    for (port_name = s = buf; s; port_name = s + 1) {
        /* Assume that ports are separated by commas.  We don't use
         * spaces because those are valid in client names. */
        s = strchr(port_name, ',');
        if (s){
            *s = '\0';
        }

        ++port_count;
        *ports = (snd_seq_addr_t*) realloc( *ports, port_count * sizeof(snd_seq_addr_t));

        check_mem(ports);

        err = snd_seq_parse_address(seq, ports[port_count - 1], port_name);
        if (err < 0)
            fatal("Invalid port %s - %s", port_name, snd_strerror(err));
    }

    free(buf);
    return port_count;
}
