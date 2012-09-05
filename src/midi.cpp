/*
 *
 * Copyright (c) 2005 Clemens Ladisch <clemens@ladisch.de>
 *
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <signal.h>
#include <getopt.h>
#include <sys/poll.h>
#include <alsa/asoundlib.h>
#include "HDSPMixerCard.h"
#include <iostream>

#define VERSION_STR 0.1
#define CHARMAX 50
//#define PHONES_CHANNEL 6    // this is stereo. See dest_map_h9652_ss
#define OUT_CHANNEL 4       // again see dest_map_h9652_ss

#define CC_MAX 127
#define CC_VOL 7
#define CC_PAN 10
#define CC_DOWN_ROW 11
#define CC_UP_ROW 14

#define CHANNELS_NUM 7      // fixme: this define is temporary

using namespace std;

static volatile sig_atomic_t stop = 0;

struct channel {
    int idx;
    char name[CHARMAX];
    bool mute;
    bool solo;
    bool stereo;
    bool input;
    int left_map;
    int right_map;
    double volume;
    double pan;
};

void search_card(HDSPMixerCard **hdsp_card){
    char *name, *shortname;
    int card = -1;;

    while (snd_card_next(&card) >= 0) {
        if (card < 0) {
            break;
        }
        snd_card_get_longname(card, &name);
        snd_card_get_name(card, &shortname);
//        cout << "Card " << card << ":" << name << endl;
        if (!strncmp(name, "RME Hammerfall DSP + Multiface", 30)) {
//            cout << "Multiface found!" << endl;
            *hdsp_card = new HDSPMixerCard(Multiface, card, shortname);
            break;
        } else if (!strncmp(name, "RME Hammerfall DSP + Digiface", 29)) {
//            cout << "Digiface found!" << endl;
            *hdsp_card = new HDSPMixerCard(Digiface, card, shortname);
            break;
        } else if (!strncmp(name, "RME Hammerfall DSP + RPM", 24)) {
//            cout << "RPM found!" << endl;
            *hdsp_card = new HDSPMixerCard(RPM, card, shortname);
            break;
        } else if (!strncmp(name, "RME Hammerfall HDSP 9652", 24)) {
//            cout << "HDSP 9652 found!" << endl;
            *hdsp_card = new HDSPMixerCard(H9652, card, shortname);
            break;
        } else if (!strncmp(name, "RME Hammerfall HDSP 9632", 24)) {
//            cout << "HDSP 9632 found!" << endl;
            *hdsp_card = new HDSPMixerCard(H9632, card, shortname);
            break;
        } else if (!strncmp(name, "RME MADIface", 12)) {
//            cout << "RME MADIface found!" << endl;
            *hdsp_card = new HDSPMixerCard(HDSPeMADI, card, shortname);
            break;
        } else if (!strncmp(name, "RME MADI", 8)) {
//            cout << "RME MADI found!" << endl;
            *hdsp_card = new HDSPMixerCard(HDSPeMADI, card, shortname);
            break;
        } else if (!strncmp(name, "RME AES32", 8)) {
//            cout << "RME AES32 or HDSPe AES found!" << endl;
            *hdsp_card = new HDSPMixerCard(HDSP_AES, card, shortname);
            break;
        } else if (!strncmp(name, "RME RayDAT", 10)) {
//            cout << "RME RayDAT found!" << endl;
            *hdsp_card = new HDSPMixerCard(HDSPeRayDAT, card, shortname);
            break;
        } else if (!strncmp(name, "RME AIO", 7)) {
//            cout << "RME AIO found!" << endl;
            *hdsp_card = new HDSPMixerCard(HDSPeAIO, card, shortname);
            break;
        } else if (!strncmp(name, "RME Hammerfall DSP", 18)) {
            cout << "Uninitialized HDSP card found.\nUse hdsploader to upload configuration data to the card." << endl;
        }
    }
    free(name);
}

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

static void init_seq(snd_seq_t **seq){
    int err;

    /* open sequencer */
    err = snd_seq_open(seq, "default", SND_SEQ_OPEN_DUPLEX, 0);
    check_snd("open sequencer", err);

    /* set our client's name */
    err = snd_seq_set_client_name(*seq, "hdspmidi");
    check_snd("set client name", err);
}

/* parses one or more port addresses from the string */
static int parse_ports(snd_seq_t *seq,snd_seq_addr_t **ports, const char *arg){
    char *buf, *s, *port_name;
    int err;
    int port_count;

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

static void create_port(snd_seq_t *seq){
    int err;

    err = snd_seq_create_simple_port(seq, "out",
                                     SND_SEQ_PORT_CAP_WRITE |
                                     SND_SEQ_PORT_CAP_SUBS_WRITE,
                                     SND_SEQ_PORT_TYPE_MIDI_GENERIC |
                                     SND_SEQ_PORT_TYPE_APPLICATION);
    check_snd("create port", err);

    err = snd_seq_create_simple_port(seq, "in",
                                     SND_SEQ_PORT_CAP_READ |
                                     SND_SEQ_PORT_CAP_SUBS_READ,
                                     SND_SEQ_PORT_TYPE_APPLICATION);
    check_snd("create port", err);
}

static void connect_ports(snd_seq_t *seq,snd_seq_addr_t *ports_out,int port_count_out,snd_seq_addr_t *ports_in,int port_count_in){
    int i, err;

    for (i = 0; i < port_count_out; ++i) {
        err = snd_seq_connect_from(seq, 0, ports_out[i].client, ports_out[i].port);
        if (err < 0)
            fatal("Cannot connect from port %d:%d - %s",
                  ports_out[i].client, ports_out[i].port, snd_strerror(err));
    }
    for (i = 0; i < port_count_in; ++i) {
        err = snd_seq_connect_to(seq, 0, ports_in[i].client, ports_in[i].port);
        if (err < 0)
            fatal("Cannot connect from port %d:%d - %s",
                  ports_in[i].client, ports_in[i].port, snd_strerror(err));
    }
}

void send_midi_control(snd_seq_t* seq, int chn, int param,int value )  {
    snd_seq_event_t ev;
    snd_seq_ev_clear(&ev);
    snd_seq_ev_set_subs(&ev);
    snd_seq_ev_set_controller(&ev, chn, param, value);
    snd_seq_ev_set_direct(&ev);
    int err = snd_seq_event_output_direct(seq, &ev);
    if (err < 0){
        cerr<<"Could not send MIDI control change: "<<snd_strerror(err)<<endl;
    }
}

void reset_midi(snd_seq_t* seq){
    for(int k=0; k<=7; k++){
        send_midi_control(seq, k, CC_VOL,0);
        send_midi_control(seq, k, CC_PAN,64);
        send_midi_control(seq, k, CC_DOWN_ROW,0);
        send_midi_control(seq, k, CC_UP_ROW,0);
    }
}

//void send_controls(HDSPMixerCard *hdsp_card,int dst, struct channel ch, int left_value, int right_value){
//    if(ch.input){
//        hdsp_card->setInput(ch.left_map, dst,left_value,right_value);
//        if(ch.stereo){
//            hdsp_card->setInput(ch.right_map, dst,left_value,right_value);
//        }
//    } else {
//        hdsp_card->setInput(ch.left_map, dst,left_value,right_value);
//        if(ch.stereo){
//            hdsp_card->setInput(ch.right_map, dst,left_value,right_value);
//        }
//    }
//}

void send_control_normal(HDSPMixerCard *hdsp_card,int dst, struct channel ch){
    int left_value = 0;
    int right_value = 0;

    if(ch.mute==false){
        left_value = ch.volume * (1 - ch.pan);
        right_value = ch.volume * ch.pan;
    }
//    send_controls(hdsp_card,dst,ch,left_value,right_value);
    if(ch.input){
        hdsp_card->setInput(ch.left_map, dst,left_value,right_value);
        if(ch.stereo){
            hdsp_card->setInput(ch.right_map, dst,left_value,right_value);
        }
    } else {
        hdsp_card->setPlayback(ch.left_map, dst,left_value,right_value);
        if(ch.stereo){
            hdsp_card->setPlayback(ch.right_map, dst,left_value,right_value);
        }
    }
}

//void send_control_solos(HDSPMixerCard *hdsp_card,struct channel ch[]){
//    bool gsolo = false;
//    for (int k=0; k< CHANNELS_NUM; k++){
//        gsolo = ch[k].solo || gsolo;
//    }
//    if(gsolo == true){
//        for (int k=0; k< CHANNELS_NUM; k++){
//            if(ch[k].mute == true){
//                send_controls(hdsp_card,PHONES_CHANNEL,ch[k],ch[k].volume,ch[k].volume);
//            } else {
//                send_controls(hdsp_card,PHONES_CHANNEL,ch[k],0,0);
//            }
//        }
//    } else{
//        for (int k=0; k< CHANNELS_NUM; k++){
//            send_control_normal(hdsp_card,PHONES_CHANNEL,ch[k]);
//        }
//    }
//}

static void dump_event(const snd_seq_event_t *ev, struct channel *channels, HDSPMixerCard *hdsp_card)
{
    int midi_channel,midi_value,midi_param;

    //    printf("%3d:%-3d \n", ev->source.client, ev->source.port);
    switch (ev->type) {
    case SND_SEQ_EVENT_CONTROLLER:
        //        printf("Control change         %2d, controller %d, value %d\n",
        //               ev->data.control.channel, ev->data.control.param, ev->data.control.value);
        midi_channel = ev->data.control.channel;
        midi_value = ev->data.control.value;
        midi_param = ev->data.control.param;
        if(midi_param == CC_VOL){
            for (int k = 0 ; k< CHANNELS_NUM; k++){
                if (midi_channel == channels[k].idx){
                    channels[k].volume = midi_value * 65536.0 /CC_MAX ; // 65536  is the max in the driver. 127 is the max in midi
                    send_control_normal(hdsp_card,OUT_CHANNEL,channels[k]);
//                    send_control_normal(hdsp_card,PHONES_CHANNEL,channels[k]);
                    break;
                }
            }
            break;
        }
        if(midi_param == CC_PAN){
            for (int k = 0 ; k< CHANNELS_NUM; k++){
                if (midi_channel == channels[k].idx){
                    channels[k].pan = midi_value *1.0 /CC_MAX * 1; //  127 is the max in midi
                    send_control_normal(hdsp_card,OUT_CHANNEL,channels[k]);
//                    send_control_normal(hdsp_card,PHONES_CHANNEL,channels[k]);
                    break;
                }
            }
            break;
        }
        if(midi_param == CC_DOWN_ROW){ // bottom row
            for (int k = 0 ; k< CHANNELS_NUM; k++){
                if (midi_channel == channels[k].idx){
                    if(midi_value == CC_MAX){
                        channels[midi_channel].mute = true;
                    } else {
                        channels[midi_channel].mute = false;
                    }
                    cout << "mute channel " << midi_channel << " " << channels[midi_channel].mute << endl;
                    send_control_normal(hdsp_card,OUT_CHANNEL,channels[k]);
//                    send_control_normal(hdsp_card,PHONES_CHANNEL,channels[k]);
                    break;
                }
            }
            break;
        }
//        if(midi_param == CC_UP_ROW){ // upper row
//            for (int k = 0 ; k< CHANNELS_NUM; k++){
//                if (midi_channel == channels[k].idx){
//                    if(midi_value == CC_MAX){
//                        channels[midi_channel].solo = true;
//                    } else {
//                        channels[midi_channel].solo = false;
//                    }
//                    cout << "solo channel " << midi_channel << " " << channels[midi_channel].mute << endl;
//                    //                    send_control(hdsp_card,channels[k]);
//                    //                    send_control_solos(hdsp_card,channels);
//                    break;
//                }
//            }
//            break;
//        }
        break;
    case SND_SEQ_EVENT_PGMCHANGE:
        printf("Program change         %2d, program %d\n", ev->data.control.channel, ev->data.control.value);
        break;
    case SND_SEQ_EVENT_PITCHBEND:
        printf("Pitch bend             %2d, value %d\n", ev->data.control.channel, ev->data.control.value);
        break;
    case SND_SEQ_EVENT_SYSEX:
    {
        unsigned int i;
        printf("System exclusive          ");
        for (i = 0; i < ev->data.ext.len; ++i)
            printf(" %02X", ((unsigned char*)ev->data.ext.ptr)[i]);
        printf("\n");
    }
        break;
    default:
        printf("Event type %d\n",  ev->type);
    }
}

static void help(const char *argv0){
    cout << "Usage: " << argv0 << "[options]\n"
            "\nAvailable options:\n"
            "  -h,--help                  this help\n"
            "  -V,--version               show version\n"
            "  -o,--portout=client:port,...  source port(s)" << endl;
}

static void version(void){
    cout << "version " << VERSION_STR << endl;
}

static void sighandler(int sig){
    stop = 1;
}

int main(int argc, char *argv[])
{
    struct channel channels[] = \
    {{ 0, "Mic 1",false, false, false, true,   0,  0, 0, 0.5},
     { 1, "Mic 2",false, false, false, true,   1,  1, 0, 0.5},
     { 2, "CD 1", false, false, true,  true,   2,  3, 0, 0.5},
     { 3, "CD 2", false, false, true,  true,  24, 25, 0, 0.5},
     { 4, "PC 1", false, false, true,  false,  0,  1, 0, 0.5},
     { 5, "PC2 1",false, false, true,  false,  2,  3, 0, 0.5},
     { 6, "DJ 1", false, false, true,  true,   4,  5, 0, 0.5}};

    snd_seq_addr_t *ports_out = NULL;
    snd_seq_addr_t *ports_in = NULL;
    int port_count_out = 0;
    int port_count_in = 0;
    snd_seq_t *seq = NULL;
    HDSPMixerCard *hdsp_card = NULL;
    static const char short_options[] = "hVo:i:";
    static const struct option long_options[] = {
        {"help", 0, NULL, 'h'},
        {"version", 0, NULL, 'V'},
        {"portout", 1, NULL, 'o'},
        {"portin", 1, NULL, 'i'},
    };
    struct pollfd *pfds = NULL;
    int npfds = 0;
    int c, err;

    search_card(&hdsp_card);
    if (hdsp_card == NULL) {
        cout << "No RME cards found." << endl;
        exit(EXIT_FAILURE);
    }

    init_seq(&seq);

    while ((c = getopt_long(argc, argv, short_options,
                            long_options, NULL)) != -1) {
        switch (c) {
        case 'h':
            help(argv[0]);
            return 0;
        case 'V':
            version();
            return 0;
        case 'o':
            port_count_out = parse_ports(seq,&ports_out,optarg);
            break;
        case 'i':
            port_count_in = parse_ports(seq,&ports_in,optarg);
            break;
        default:
            help(argv[0]);
            return 1;
        }
    }
    if (optind < argc) {
        help(argv[0]);
        return 1;
    }


    create_port(seq);
    connect_ports(seq,ports_out, port_count_out, ports_in, port_count_in);

    err = snd_seq_nonblock(seq, 1);
    check_snd("set nonblock mode", err);

    if (port_count_out > 0)
        cout << "Waiting for data.";
    else
        cout << "Waiting for data at port " << snd_seq_client_id(seq) << ":0.";
    cout << " Press Ctrl+C to end." << endl;
    cout << "Source  Event                  Ch  Data" << endl;

    signal(SIGINT, sighandler);
    signal(SIGTERM, sighandler);

    npfds = snd_seq_poll_descriptors_count(seq, POLLIN);
    pfds = new pollfd[npfds];

    hdsp_card->resetMixer();
    reset_midi(seq);

    while(true) {
        snd_seq_poll_descriptors(seq, pfds, npfds, POLLIN);
        if (poll(pfds, npfds, -1) < 0){
            break;
        }
        do {
            snd_seq_event_t *event;
            err = snd_seq_event_input(seq, &event);
            if (err < 0){
                break;
            }
            if (event){
                dump_event(event, channels,hdsp_card);
            }
        } while (err > 0);
        if (stop){
            break;
        }
    }
    delete[] pfds;
    snd_seq_close(seq);
    return 0;
}
