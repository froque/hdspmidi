#include "bridge.h"
#include <iostream>
#include <cmath>

using namespace std;

Bridge::Bridge()
{
    hdsp_card = NULL;
    vegas = 0;
}

Bridge::~Bridge()
{
    delete hdsp_card;
}

void Bridge::restore(void)
{

#ifndef NO_RME
    // reset all to 0.
    hdsp_card->resetMixer();
    // restore channels gains from file
    for (int k = 0 ; k< channels.getNum(); k++){
        control_normal(main,channels.channels_data[k]);
    }
    control_solos();
#endif //NO_RME
    control_onair();
    // restore channels midi from file
    midicontroller.restore_midi(&channels);

}


void Bridge::send_control(int dst, struct channel ch, int left_value, int right_value){
    if(ch.input){
#ifndef NO_RME
        hdsp_card->setInput(ch.left_map, dst,left_value,right_value);
#endif //NO_RME
        if(ch.stereo){
#ifndef NO_RME
            hdsp_card->setInput(ch.right_map, dst,left_value,right_value);
#endif //NO_RME
        }
    } else {
#ifndef NO_RME
        hdsp_card->setPlayback(ch.left_map, dst,left_value,right_value);
#endif //NO_RME
        if(ch.stereo){
#ifndef NO_RME
            hdsp_card->setPlayback(ch.right_map, dst,left_value,right_value);
#endif //NO_RME
        }
    }
}

void Bridge::control_normal(int dst, struct channel ch){
    int left_value = 0;
    int right_value = 0;

    if(ch.mute==false){
        left_value = ch.volume * (1 - ch.balance);
        right_value = ch.volume * ch.balance;
    }
    send_control(dst,ch,left_value,right_value);
}

void Bridge::control_solos(){
    bool gsolo = false;

    // determine if any channel is set to solo, because affects others.
    for (int k=0; k< channels.getNum(); k++){
        gsolo = channels.channels_data[k].solo || gsolo;
    }

    if(gsolo == true){
        for (int k=0; k< channels.getNum(); k++){
            if(channels.channels_data[k].solo == true){
                send_control(phones,channels.channels_data[k],ZERO_DB/2,ZERO_DB/2);
            } else {
                send_control(phones,channels.channels_data[k],0,0);
            }
        }
    } else{
        for (int k=0; k< channels.getNum(); k++){
            control_normal(phones,channels.channels_data[k]);
        }
    }
}

void Bridge::control_onair(){
    bool g_onair = false;

    // determine if any channel that requires onair is set, because affects others.
    for (int k=0; k< channels.getNum(); k++){
        if (channels.channels_data[k].microphone == true){
            if(channels.channels_data[k].volume !=0 && channels.channels_data[k].mute == false){
                g_onair = true;
            }
        }
    }

    if(g_onair == true){
        relay.on();
    } else{
        relay.off();
    }
}

void Bridge::dump_event(const snd_seq_event_t *ev)
{
    int midi_channel,midi_value,midi_param;

    switch (ev->type) {
    case SND_SEQ_EVENT_CONTROLLER:
        midi_channel = ev->data.control.channel;
        midi_value = ev->data.control.value;
        midi_param = ev->data.control.param;
        if(midi_param == CC_VOL){
            for (int k = 0 ; k< channels.getNum(); k++){
                if (midi_channel == channels.channels_data[k].idx){
                    if (midi_value == 0){
                        channels.channels_data[k].volume = 0;
                    } else {
                        double dB = -LOWER_DB + LOWER_DB * midi_value /CC_MAX;
                        channels.channels_data[k].volume = ZERO_DB * pow(10,dB/20);
                    }

                    if (channels.channels_data[k].microphone == false){
                        control_normal(monitors,channels.channels_data[k]);
                    }
                    control_normal(main,channels.channels_data[k]);
                    control_solos();
                    control_onair();
                    break;
                }
            }
            break;
        }
        if(midi_param == CC_PAN){
            for (int k = 0 ; k< channels.getNum(); k++){
                if (midi_channel == channels.channels_data[k].idx){
                    channels.channels_data[k].balance = midi_value *1.0 /CC_MAX * 1; //  127 is the max in midi

                    if (channels.channels_data[k].microphone == false){
                        control_normal(monitors,channels.channels_data[k]);
                    }
                    control_normal(main,channels.channels_data[k]);
                    control_solos();
                    break;
                }
            }
            break;
        }
        if(midi_param == CC_PAN_CENTER){
            for (int k = 0 ; k< channels.getNum(); k++){
                if (midi_channel == channels.channels_data[k].idx){
                    channels.channels_data[k].balance = 0.5; //

                    midicontroller.send_midi_CC(k,CC_PAN,CC_MAX/2);

                    if (channels.channels_data[k].microphone == false){
                        control_normal(monitors,channels.channels_data[k]);
                    }
                    control_normal(main,channels.channels_data[k]);
                    control_solos();
                    break;
                }
            }
            break;
        }
        if(midi_param == CC_DOWN_ROW){ // bottom row
            for (int k = 0 ; k< channels.getNum(); k++){
                if (midi_channel == channels.channels_data[k].idx){
                    if(midi_value == CC_MAX){
                        channels.channels_data[midi_channel].mute = true;
                    } else {
                        channels.channels_data[midi_channel].mute = false;
                    }

                    if (channels.channels_data[k].microphone == false){
                        control_normal(monitors,channels.channels_data[k]);
                    }
                    control_normal(main,channels.channels_data[k]);
                    control_solos();
                    control_onair();
                    break;
                }
            }
            break;
        }
        if(midi_param == CC_UP_ROW){ // upper row
            for (int k = 0 ; k< channels.getNum(); k++){
                if (midi_channel == channels.channels_data[k].idx){
                    if(midi_value == CC_MAX){
                        channels.channels_data[midi_channel].solo = true;
                    } else {
                        channels.channels_data[midi_channel].solo = false;
                    }

                    control_solos();
                    break;
                }
            }
            break;
        }
        break;
    case SND_SEQ_EVENT_PGMCHANGE:
        printf("Program change         %2d, program %d\n", ev->data.control.channel, ev->data.control.value);
        break;
    case SND_SEQ_EVENT_PITCHBEND:
        printf("Pitch bend             %2d, value %d\n", ev->data.control.channel, ev->data.control.value);
        break;
    case SND_SEQ_EVENT_SYSEX:
    {
        unsigned char vegas_key1[]={0xF0,0x7F,0x7F,0x06,0x04,0xF7};
        unsigned int i;
        printf("System exclusive          ");
        for (i = 0; i < ev->data.ext.len; ++i){
            printf(" %02X", ((unsigned char*)ev->data.ext.ptr)[i]);
        }
        printf("\n");
        if ( memcmp(vegas_key1,ev->data.ext.ptr,6) == 0 ){
            vegas++;
        } else {
            vegas = 0;
        }
        if(vegas == 5){
            vegas = 0;
            midicontroller.vegas();
            midicontroller.restore_midi(&channels);
        }
        return;
    }
        break;
    default:
        printf("Event type %d\n",  ev->type);
    }
    vegas = 0;
}
