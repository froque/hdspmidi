#include "bridge.h"
#include <iostream>

using namespace std;

Bridge::Bridge()
{
    hdsp_card = NULL;
}

Bridge::~Bridge()
{
    delete hdsp_card;
}

void Bridge::restore(void)
{

#ifdef NO_RME
    // reset all to 0.
    hdsp_card->resetMixer();
    // restore channels gains from file
    for (int k = 0 ; k< channels.getNum(); k++){
        control_normal(main,channels.channels_data[k]);
    }
    control_solos();
#endif //NO_RME
    // restore channels midi from file
    midicontroller.restore_midi(&channels);

}


void Bridge::send_control(int dst, struct channel ch, int left_value, int right_value){
    if(ch.input){
#ifdef NO_RME
        hdsp_card->setInput(ch.left_map, dst,left_value,right_value);
#endif //NO_RME
        if(ch.stereo){
#ifdef NO_RME
            hdsp_card->setInput(ch.right_map, dst,left_value,right_value);
#endif //NO_RME
        }
    } else {
#ifdef NO_RME
        hdsp_card->setPlayback(ch.left_map, dst,left_value,right_value);
#endif //NO_RME
        if(ch.stereo){
#ifdef NO_RME
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
                send_control(phones,channels.channels_data[k],ZERO_DB,ZERO_DB);
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
                    channels.channels_data[k].volume = midi_value * RME_MAX *1.0 /CC_MAX ; // 65536  is the max in the driver. 127 is the max in midi
                    control_normal(main,channels.channels_data[k]);
                    control_solos();
                    break;
                }
            }
            break;
        }
        if(midi_param == CC_PAN){
            for (int k = 0 ; k< channels.getNum(); k++){
                if (midi_channel == channels.channels_data[k].idx){
                    channels.channels_data[k].balance = midi_value *1.0 /CC_MAX * 1; //  127 is the max in midi
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
                    control_normal(main,channels.channels_data[k]);
                    control_solos();
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
