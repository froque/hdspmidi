#include "Channel.h"
#include <string.h>
#include <iostream>

using namespace std;
using namespace libconfig;

void Channels::print(){
    for (int k=0;k<num;k++){
        cout << "idx " << channels_data[k].idx;
        cout << " name " << channels_data[k].name;
        cout << " mute " << channels_data[k].mute;
        cout << " solo " << channels_data[k].solo;
        cout << " stereo " << channels_data[k].stereo;
        cout << " input " << channels_data[k].input;
        cout << " microphone " << channels_data[k].microphone;
        cout << " left_map " << channels_data[k].left_map;
        cout << " right_map " << channels_data[k].right_map;
        cout << " volume " << channels_data[k].volume;
        cout << " balance " << channels_data[k].balance;
        cout << endl;
    }
    cout << endl;
}

bool Channels::read(Config *cfg){
    int idx;
    const char *name;
    bool mute;
    bool solo;
    bool stereo;
    bool input;
    bool microphone;
    int left_map;
    int right_map;
    double volume;
    double balance;
    if ( num != 0){
        return false;
    }
    try
    {
        Setting& sch = cfg->lookup("channels");
        for (int k=0; k<sch.getLength() && num <CH_MAX ;k++){

            if (sch[k].lookupValue("idx",idx) &&
                    sch[k].lookupValue("name",name) &&
                    sch[k].lookupValue("mute",mute) &&
                    sch[k].lookupValue("solo",solo) &&
                    sch[k].lookupValue("stereo",stereo) &&
                    sch[k].lookupValue("input",input) &&
                    sch[k].lookupValue("microphone",microphone) &&
                    sch[k].lookupValue("left_map",left_map) &&
                    sch[k].lookupValue("right_map",right_map) &&
                    sch[k].lookupValue("volume",volume) &&
                    sch[k].lookupValue("balance",balance) ){

                channels_data[num].idx = idx;
                strcpy(channels_data[num].name,name);
                channels_data[num].mute = mute;
                channels_data[num].solo = solo;
                channels_data[num].stereo = stereo;
                channels_data[num].input = input;
                channels_data[num].microphone = microphone;
                channels_data[num].left_map = left_map;
                channels_data[num].right_map = right_map;
                channels_data[num].volume = volume;
                channels_data[num].balance = balance;
                num++;
            } else {
                cout << "error parsing channel " << k << endl;
            }
        }
    }
    catch (ConfigException& e)
    {
        return false;
    }
    return true;
}

bool Channels::save(Config *cfg){
    Setting& slist = cfg->lookup("channels");

    while(slist.getLength() >0){
        slist.remove(slist.getLength()-1);
    }
    try
    {
        for (int k=0; k<num; k++){
            Setting& sch = slist.add(Setting::TypeGroup);
            Setting& sname = sch.add("name",Setting::TypeString);
            sname = channels_data[k].name;
            Setting& sidx = sch.add("idx",Setting::TypeInt);
            sidx = channels_data[k].idx;
            Setting& smute = sch.add("mute",Setting::TypeBoolean);
            smute = channels_data[k].mute;
            Setting& ssolo = sch.add("solo",Setting::TypeBoolean);
            ssolo = channels_data[k].solo;
            Setting& sstereo = sch.add("stereo",Setting::TypeBoolean);
            sstereo = channels_data[k].stereo;
            Setting& sinput = sch.add("input",Setting::TypeBoolean);
            sinput = channels_data[k].input;
            Setting& smicrophone = sch.add("microphone",Setting::TypeBoolean);
            smicrophone = channels_data[k].microphone;
            Setting& sleft_map = sch.add("left_map",Setting::TypeInt);
            sleft_map = channels_data[k].left_map;
            Setting& sright_map = sch.add("right_map",Setting::TypeInt);
            sright_map = channels_data[k].right_map;

            Setting& svolume = sch.add("volume",Setting::TypeFloat);
            svolume = channels_data[k].volume;
            Setting& sbalance = sch.add("balance",Setting::TypeFloat);
            sbalance = channels_data[k].balance;
        }
    }
    catch (ConfigException& e)
    {
        return false;
    }
    return true;
}
