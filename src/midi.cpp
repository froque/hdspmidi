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
#include <libconfig.h++>
#include "Channel.h"
#include "midicontroller.h"

#define VERSION_STR 0.1

//#define PHONES_CHANNEL 6    // this is stereo. See dest_map_h9652_ss
#define OUT_CHANNEL 4       // again see dest_map_h9652_ss


#define DEFAULT_FILE "channels.cfg"

using namespace std;

static volatile sig_atomic_t stop = 0;

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
        left_value = ch.volume * (1 - ch.balance);
        right_value = ch.volume * ch.balance;
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

static void dump_event(const snd_seq_event_t *ev, Channels *ch, HDSPMixerCard *hdsp_card)
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
            for (int k = 0 ; k< ch->getNum(); k++){
                if (midi_channel == ch->channels_data[k].idx){
                    ch->channels_data[k].volume = midi_value * 65536.0 /CC_MAX ; // 65536  is the max in the driver. 127 is the max in midi
                    send_control_normal(hdsp_card,OUT_CHANNEL,ch->channels_data[k]);
//                    send_control_normal(hdsp_card,PHONES_CHANNEL,channels[k]);
                    break;
                }
            }
            break;
        }
        if(midi_param == CC_PAN){
            for (int k = 0 ; k< ch->getNum(); k++){
                if (midi_channel == ch->channels_data[k].idx){
                    ch->channels_data[k].balance = midi_value *1.0 /CC_MAX * 1; //  127 is the max in midi
                    send_control_normal(hdsp_card,OUT_CHANNEL,ch->channels_data[k]);
//                    send_control_normal(hdsp_card,PHONES_CHANNEL,channels[k]);
                    break;
                }
            }
            break;
        }
        if(midi_param == CC_DOWN_ROW){ // bottom row
            for (int k = 0 ; k< ch->getNum(); k++){
                if (midi_channel == ch->channels_data[k].idx){
                    if(midi_value == CC_MAX){
                        ch->channels_data[midi_channel].mute = true;
                    } else {
                        ch->channels_data[midi_channel].mute = false;
                    }
                    cout << "mute channel " << midi_channel << " " << ch->channels_data[midi_channel].mute << endl;
                    send_control_normal(hdsp_card,OUT_CHANNEL,ch->channels_data[k]);
//                    send_control_normal(hdsp_card,PHONES_CHANNEL,channels[k]);
                    break;
                }
            }
            break;
        }
//        if(midi_param == CC_UP_ROW){ // upper row
//            for (int k = 0 ; k< ch.getNum(); k++){
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
    libconfig::Config cfg;
    try{
        cfg.readFile(DEFAULT_FILE);
    }
    catch (libconfig::ConfigException& e) {
        return -1;
    }
    Channels channels;
    channels.read(&cfg);

    MidiController midicontroller;
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
            midicontroller.parse_ports_out(optarg);
            break;
        case 'i':
            midicontroller.parse_ports_in(optarg);
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

    midicontroller.connect_ports();

    signal(SIGINT, sighandler);
    signal(SIGTERM, sighandler);

    npfds = snd_seq_poll_descriptors_count(midicontroller.seq, POLLIN);
    pfds = new pollfd[npfds];

    // reset all to 0.
    hdsp_card->resetMixer();
    // restore channels gains from file
    for (int k = 0 ; k< channels.getNum(); k++){
        send_control_normal(hdsp_card,OUT_CHANNEL,channels.channels_data[k]);
    }
    // restore channels midi from file
    midicontroller.restore_midi(&channels);

    while(true) {
        snd_seq_poll_descriptors(midicontroller.seq, pfds, npfds, POLLIN);
        if (poll(pfds, npfds, -1) < 0){
            break;
        }
        do {
            snd_seq_event_t *event;
            err = snd_seq_event_input(midicontroller.seq, &event);
            if (err < 0){
                break;
            }
            if (event){
                dump_event(event, &channels,hdsp_card);
            }
        } while (err > 0);
        if (stop){
            break;
        }
    }
    channels.save(&cfg);
    cfg.writeFile(DEFAULT_FILE);
    delete[] pfds;

    return 0;
}
