/*
 *   HDSPMixer
 *
 *   Copyright (C) 2003 Thomas Charbonnel (thomas@undata.org)
 *
 *   Copyright (C) 2011 Adrian Knoth (adi@drcomp.erfurt.thur.de)
 *                      Fredrik Lingvall (fredrik.lingvall@gmail.com)
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#pragma implementation
#include "HDSPMixerCard.h"
#include <string>
#include <sys/ioctl.h>
#include "channelmap.h"

void HDSPMixerCard::search_card(HDSPMixerCard **hdsp_card){
    char *name, *shortname;
    int card = -1;;

    while (snd_card_next(&card) >= 0) {
        if (card < 0) {
            break;
        }
        snd_card_get_longname(card, &name);
        snd_card_get_name(card, &shortname);
        if (!strncmp(name, "RME Hammerfall DSP + Multiface", 30)) {
            *hdsp_card = new HDSPMixerCard(Multiface, card, shortname);
            break;
        } else if (!strncmp(name, "RME Hammerfall DSP + Digiface", 29)) {
            *hdsp_card = new HDSPMixerCard(Digiface, card, shortname);
            break;
        } else if (!strncmp(name, "RME Hammerfall DSP + RPM", 24)) {
            *hdsp_card = new HDSPMixerCard(RPM, card, shortname);
            break;
        } else if (!strncmp(name, "RME Hammerfall HDSP 9652", 24)) {
            *hdsp_card = new HDSPMixerCard(H9652, card, shortname);
            break;
        } else if (!strncmp(name, "RME Hammerfall HDSP 9632", 24)) {
            *hdsp_card = new HDSPMixerCard(H9632, card, shortname);
            break;
        } else if (!strncmp(name, "RME MADIface", 12)) {
            *hdsp_card = new HDSPMixerCard(HDSPeMADI, card, shortname);
            break;
        } else if (!strncmp(name, "RME MADI", 8)) {
            *hdsp_card = new HDSPMixerCard(HDSPeMADI, card, shortname);
            break;
        } else if (!strncmp(name, "RME AES32", 8)) {
            *hdsp_card = new HDSPMixerCard(HDSP_AES, card, shortname);
            break;
        } else if (!strncmp(name, "RME RayDAT", 10)) {
            *hdsp_card = new HDSPMixerCard(HDSPeRayDAT, card, shortname);
            break;
        } else if (!strncmp(name, "RME AIO", 7)) {
            *hdsp_card = new HDSPMixerCard(HDSPeAIO, card, shortname);
            break;
        } else if (!strncmp(name, "RME Hammerfall DSP", 18)) {
            fprintf(stderr, "Uninitialized HDSP card found.\nUse hdsploader to upload configuration data to the card.\n");
        }
    }
    free(name);
}

static void alsactl_cb(snd_async_handler_t *handler)
{
    int err;
    snd_ctl_t *ctl;
    snd_ctl_event_t *event;
    snd_ctl_elem_value_t *elemval;
    snd_ctl_elem_id_t *elemid;
    HDSPMixerCard *card;
    
    card = (HDSPMixerCard *)snd_async_handler_get_callback_private(handler);
    
    snd_ctl_elem_value_alloca(&elemval);
    snd_ctl_elem_id_alloca(&elemid);
    
    ctl = snd_async_handler_get_ctl(handler);
    
    if ((err = snd_ctl_nonblock(ctl, 1))) {
        printf("Error setting non blocking mode for card %s\n", card->name);
        return;
    }
    
    snd_ctl_event_malloc(&event);
    
    while ((err = snd_ctl_read(ctl, event)) > 0) {
        // what is 11 ?   Possible answer: is the index of array struct snd_kcontrol_new snd_hdsp_controls[] in the kernel driver: Sample Clock Source
        if (snd_ctl_event_elem_get_numid(event) == 11){// && (card == card->basew->cards[card->basew->current_card])) {
            /* We have a clock change and are the focused card */
            snd_ctl_event_elem_get_id(event, elemid);
            snd_ctl_elem_value_set_id(elemval, elemid);
            if ((err = snd_ctl_elem_read(ctl, elemval)) < 0) {
                fprintf(stderr, "Error reading snd_ctl_elem_t\n");
                snd_ctl_event_free(event);
                return;
            }
            int speed = card->getSpeed();
            if(speed != card->speed_mode){
                card->setMode(speed);
//                card->basew->updateMode();
            }
        }
        snd_ctl_event_clear(event);
    }
    snd_ctl_event_free(event);
}

/*! returns 0 for normal speed
 *  returns 1 for double speed
 *  returns 2 for quad speed
 */
int HDSPMixerCard::getAutosyncSpeed()
{
    int rate;
    snd_ctl_elem_value_t *elemval;
    snd_ctl_elem_id_t * elemid;
    snd_ctl_elem_value_alloca(&elemval);
    snd_ctl_elem_id_alloca(&elemid);

    if(isOpenCtl() == false ){
        return -1;
    }
    
    snd_ctl_elem_id_set_name(elemid, "System Sample Rate");
    snd_ctl_elem_id_set_interface(elemid, SND_CTL_ELEM_IFACE_MIXER);
    snd_ctl_elem_id_set_index(elemid, 0);
    snd_ctl_elem_value_set_id(elemval, elemid);
    if (snd_ctl_elem_read(ctl_handle, elemval) < 0) {
        snd_ctl_elem_id_set_interface(elemid, SND_CTL_ELEM_IFACE_HWDEP);
        snd_ctl_elem_value_set_id(elemval, elemid);
        snd_ctl_elem_read(ctl_handle, elemval);
    }
    rate = snd_ctl_elem_value_get_integer(elemval, 0);

    if (rate > 96000) {
        return 2;
    } else if (rate > 48000) {
        return 1;
    }
    return 0;
}

/*! returns 0 for normal speed
 *  returns 1 for double speed
 *  returns 2 for quad speed
 */
int HDSPMixerCard::getSpeed()
{
    int val;
    snd_ctl_elem_value_t *elemval;
    snd_ctl_elem_id_t * elemid;
    
    snd_ctl_elem_value_alloca(&elemval);
    snd_ctl_elem_id_alloca(&elemid);

    if(isOpenCtl() == false ){
        return -1;
    }

    snd_ctl_elem_id_set_name(elemid, "Sample Clock Source");
    snd_ctl_elem_id_set_interface(elemid, SND_CTL_ELEM_IFACE_MIXER);
    snd_ctl_elem_id_set_index(elemid, 0);
    snd_ctl_elem_value_set_id(elemval, elemid);
    if (snd_ctl_elem_read(ctl_handle, elemval) < 0) {
        snd_ctl_elem_id_set_interface(elemid, SND_CTL_ELEM_IFACE_PCM);
        snd_ctl_elem_value_set_id(elemval, elemid);
        snd_ctl_elem_read(ctl_handle, elemval);
    }
    val = snd_ctl_elem_value_get_enumerated(elemval, 0);

    switch (val) {
    case 0:
        /* Autosync mode : We need to determine sample rate */
        return getAutosyncSpeed();
        break;
    case 1:
    case 2:
    case 3:
        /* SR <= 48000 - normal speed */
        return 0;
    case 4:
    case 5:
    case 6:
        /* SR > 48000 Hz - double speed */
        return 1;
    case 7:
    case 8:
    case 9:
        /* SR > 96000 Hz - quad speed */
        return 2;
    default:
        /* Should never happen */
        return 0;
    }
    return 0;
}

HDSPMixerCard::HDSPMixerCard(int cardtype, int id, char *shortname)
{
    ctl_handle = NULL;
    hw = NULL;
    type = cardtype;
    snprintf(name, 6, "hw:%i", id);
    cardname = shortname;
    h9632_aeb.aebi = 0;
    h9632_aeb.aebo = 0;

    openCtl();
    openHW();

    if (type == H9632) {
        getAeb();
        playbacks_offset = 16;
    } else {
        playbacks_offset = 26;
    }

    speed_mode = getSpeed();
    if (speed_mode < 0) {
        fprintf(stderr, "Error trying to determine speed mode for card %s, exiting.\n", name);
        exit(EXIT_FAILURE);
    }
    
    /* Set channels and mappings */
    adjustSettings();
    last_preset = last_dirty = 0;

//    basew = NULL;
}

HDSPMixerCard::~HDSPMixerCard(){
    closeHW();
    closeCtl();
}

void HDSPMixerCard::getAeb() {
    int err;

    if (isOpenHW() == false){
        return;
    }
    if ((err = snd_hwdep_ioctl(hw, SNDRV_HDSP_IOCTL_GET_9632_AEB, &h9632_aeb)) < 0) {
        fprintf(stderr, "Hwdep ioctl error on card %s : %s.\n", name, snd_strerror(err));
        snd_hwdep_close(hw);
        return;
    }
}

void HDSPMixerCard::adjustSettings() {
    if (type == Multiface) {
        switch (speed_mode) {
        case 0:
            channels_input = 18;
            channels_playback = 18;
            channels_output = 20; /* SS 8xAnalog+8xADAT+2xSPDIF+2xHeadphone */
            channel_map_input = channel_map_playback = channel_map_mf_ss;
            dest_map = dest_map_mf_ss;
            meter_map_input = meter_map_playback = channel_map_mf_ss;
            break;
        case 1:
            channels_input = 14;
            channels_playback = 14;
            channels_output = 16; /* DS 8xAnalog+4xADAT(SMUX)+2xSPDIF+2xHeadphone */
            channel_map_input = channel_map_playback = meter_map_ds;
            dest_map = dest_map_ds;
            meter_map_input = meter_map_playback = meter_map_ds;
            break;
        case 2:
            /* should never happen */
            break;
        }
    }
    
    if (type == Digiface) {
        switch (speed_mode) {
        case 0:
            channels_input = channels_playback = 26;
            channels_output = 28; /* SS 3x8xADAT+2xSPDIF+2xHeadphone */
            channel_map_input = channel_map_playback = channel_map_df_ss;
            dest_map = dest_map_df_ss;
            meter_map_input = meter_map_playback = channel_map_df_ss;
            break;
        case 1:
            channels_input = channels_playback = 14;
            channels_output = 16; /* DS 3x4xADAT(SMUX)+2xSPDIF+2xHeadphone */
            channel_map_input = channel_map_playback = meter_map_ds;
            dest_map = dest_map_ds;
            meter_map_input = meter_map_playback = meter_map_ds;
            break;
        case 2:
            /* should never happen */
            break;
        }
    }

    if (type == RPM) {
        /* RPM has no digital audio connectors, hence channel mappings don't
         * depend on speedmode */
        channels_input = 5;
        channels_playback = channels_output = 6; /* 2xMain,2xMon,2xPH */
        channel_map_input = channel_map_playback = channel_map_rpm;
        dest_map = dest_map_rpm;
        meter_map_input = meter_map_playback = channel_map_rpm;
    }


    if (type == H9652) {
        switch (speed_mode) {
        case 0:
            channels_input = channels_playback = 26;
            channels_output = 26; /* SS like Digiface, but no Headphones */
            channel_map_input = channel_map_playback = channel_map_df_ss;
            dest_map = dest_map_h9652_ss;
            meter_map_input = meter_map_playback = channel_map_df_ss;
            break;
        case 1:
            channels_input = channels_playback = 14;
            channels_output = 14; /* DS like Digiface, but no Headphones */
            channel_map_input = channel_map_playback = channel_map_ds;
            dest_map = dest_map_h9652_ds;
            meter_map_input = meter_map_playback = meter_map_ds;
            break;
        case 2:
            /* should never happen */
            break;
        }
    }

    if (type == H9632) {
        switch (speed_mode) {
        case 0:
            channels_input = channels_playback = 12 + ((h9632_aeb.aebi || h9632_aeb.aebo) ? 4 : 0);
            channels_output = channels_playback; /* untested, no idea about this card */
            channel_map_input = channel_map_playback = channel_map_h9632_ss;
            dest_map = dest_map_h9632_ss;
            meter_map_input = meter_map_playback = channel_map_h9632_ss;
            break;
        case 1:
            channels_input = channels_playback = 8 + ((h9632_aeb.aebi || h9632_aeb.aebo) ? 4 : 0);
            channels_output = channels_playback; /* untested, no idea about this card */
            channel_map_input = channel_map_playback = channel_map_h9632_ds;
            dest_map = dest_map_h9632_ds;
            meter_map_input = meter_map_playback = channel_map_h9632_ds;
            break;
        case 2:
            channels_input = channels_playback = 4 + ((h9632_aeb.aebi || h9632_aeb.aebo) ? 4 : 0);
            channels_output = channels_playback; /* untested, no idea about this card */
            channel_map_input = channel_map_playback = channel_map_h9632_qs;
            dest_map = dest_map_h9632_qs;
            meter_map_input = meter_map_playback = channel_map_h9632_qs;
            break;
        }
    }

    if (HDSPeMADI == type) {
        playbacks_offset = 64;

        switch (speed_mode) {
        case 0: // SS
            channels_input = channels_playback = 64;
            channels_output = channels_input; /* SS headphones missing, at least HDSPe MADI has some, MADIface hasn't */
            channel_map_input = channel_map_playback = channel_map_unity_ss;
            dest_map = dest_map_unity;
            meter_map_input = meter_map_playback = channel_map_unity_ss;
            break;
        case 1: // DS
            channels_input = channels_playback = 32;
            channels_output = channels_input; /* DS headphones missing, at least HDSPe MADI has some, MADIface hasn't */
            channel_map_input = channel_map_playback = channel_map_unity_ss;
            dest_map = dest_map_unity;
            meter_map_input = meter_map_playback = channel_map_unity_ss;
            break;
        case 2: // QS
            channels_input = channels_playback = 16;
            channels_output = channels_input; /* QS headphones missing, at least HDSPe MADI has some, MADIface hasn't */
            channel_map_input = channel_map_playback = channel_map_unity_ss;
            dest_map = dest_map_unity;
            meter_map_input = meter_map_playback = channel_map_unity_ss;
            break;
        }

    }

    if (HDSPeAIO == type) {
        playbacks_offset = 64;

        switch (speed_mode) {
        case 0: // SS
            channels_input = 14;
            channels_playback = 16;
            channels_output = 16; /* SS 2xAnalog+2xAES+2xSPDIF+4xADAT+2xHeadphones */
            channel_map_input = channel_map_aio_in_ss;
            channel_map_playback = channel_map_aio_out_ss;
            dest_map = dest_map_aio_ss;
            meter_map_input = channel_map_aio_in_ss;
            meter_map_playback = channel_map_aio_out_ss;
            break;
        case 1: // DS
            channels_input = 10;
            channels_playback = 12;
            channels_output = 12; /* DS 2xAnalog+2xAES+2xSPDIF+4xADAT(SMUX)+2xHeadphones */
            channel_map_input = channel_map_aio_in_ds;
            channel_map_playback = channel_map_aio_out_ds;
            dest_map = dest_map_aio_ds;
            meter_map_input = channel_map_aio_in_ds;
            meter_map_playback = channel_map_aio_out_ds;
            break;
        case 2: // QS
            channels_input = 8;
            channels_playback = 10;
            channels_output = 10; /* QS 2xAnalog+2xAES+2xSPDIF+2xADAT(SMUX)+2xHeadphones */
            channel_map_input = channel_map_aio_in_qs;
            channel_map_playback = channel_map_aio_out_qs;
            dest_map = dest_map_aio_qs;
            meter_map_input = channel_map_aio_in_qs;
            meter_map_playback = channel_map_aio_out_qs;
            break;
        }

    }

    if (HDSP_AES == type) {
        playbacks_offset = 64; /* not sure about this one? */

        /* 16 channels for all modes */
        channels_input = 16;
        channels_playback = 16;
        channels_output = 16;
        channel_map_input = channel_map_aes32;
        channel_map_playback = channel_map_aes32;
        dest_map = dest_map_aes32;
        meter_map_input = channel_map_aes32;
        meter_map_playback = channel_map_aes32;

    }

    if (HDSPeRayDAT == type) {
        playbacks_offset = 64;

        switch (speed_mode) {
        case 0: // SS
            channels_input = 36;
            channels_playback = 36;
            channels_output = 36; /* SS 4x8xADAT+2xAES/EBU+2xSPDIF */
            channel_map_input = channel_map_playback = channel_map_raydat_ss;
            dest_map = dest_map_raydat_ss;
            meter_map_input = meter_map_playback = channel_map_raydat_ss;
            break;
        case 1: // DS
            channels_input = 20;
            channels_playback = 20;
            channels_output = 20; /* DS 4x4xADAT(SMUX)+2xAES/EBU+2xSPDIF */
            channel_map_input = channel_map_playback = channel_map_raydat_ds;
            dest_map = dest_map_raydat_ds;
            meter_map_input = meter_map_playback = channel_map_raydat_ds;
            break;
        case 2: // QS
            channels_input = 12;
            channels_playback = 12;
            channels_output = 12; /* QS 4x2xADAT(SMUX)+2xAES/EBU+2xSPDIF */
            channel_map_input = channel_map_playback = channel_map_raydat_qs;
            dest_map = dest_map_raydat_qs;
            meter_map_input = meter_map_playback = channel_map_raydat_qs;
            break;
        }
    }
}

/*! 0 for normal speed
 *  1 for double speed
 *  2 for quad speed
 */
void HDSPMixerCard::setMode(int mode)
{
    speed_mode = mode;
    adjustSettings();
    resetMixer();
}

/*! called from HDSPMixerWindow constructor
 *
 *  This class is contructed before the HDSPMixerWindow, so this basically passes a reference to the window
 *
 *  It also sets up a ALSA callback to all events
 */
int HDSPMixerCard::initializeCard()
{
    int err;

    if(isOpenCtl() == false){
         exit(EXIT_FAILURE);
    }
    if ((err = snd_async_add_ctl_handler(&cb_handler, ctl_handle, alsactl_cb, this)) < 0) {
        fprintf(stderr, "Error registering async ctl callback for card %s - exiting\n", name);
        exit(EXIT_FAILURE);
    }
    if ((err = snd_ctl_subscribe_events(ctl_handle, 1)) < 0) {
        fprintf(stderr, "Error subscribing to ctl events for card %s - exiting\n", name);
        exit(EXIT_FAILURE);
    }
//    basew = w;
    return 0;
}

void HDSPMixerCard::setGain(int in, int out, int value)
{
    /* just a wrapper around the 'Mixer' ctl */

    int err;

    snd_ctl_elem_id_t *id;
    snd_ctl_elem_value_t *ctl;

    //printf("setGain(%d, %d, %d)\n", in, out, value);

    snd_ctl_elem_value_alloca(&ctl);
    snd_ctl_elem_id_alloca(&id);
    snd_ctl_elem_id_set_name(id, "Mixer");
    snd_ctl_elem_id_set_interface(id, SND_CTL_ELEM_IFACE_HWDEP);
    snd_ctl_elem_id_set_device(id, 0);
    snd_ctl_elem_id_set_index(id, 0);
    snd_ctl_elem_value_set_id(ctl, id);

    if (isOpenCtl() == false ){
        return;
    }

    snd_ctl_elem_value_set_integer(ctl, 0, in);
    snd_ctl_elem_value_set_integer(ctl, 1, out);
    snd_ctl_elem_value_set_integer(ctl, 2, value);
    if ((err = snd_ctl_elem_write(ctl_handle, ctl)) < 0) {
        fprintf(stderr, "Alsa error 2: %s\n", snd_strerror(err));
        closeCtl();
        return;
    }
}

void HDSPMixerCard::resetMixer()
{
    int i, j;
    for (i = 0; i < (playbacks_offset*2) ; ++i) {
        for (j = 0; j < (playbacks_offset); ++j) {
            setGain(i,j,0);
        }
    }
}

void HDSPMixerCard::getPeakRmsMadi(struct hdspm_peak_rms *hdspm_peak_rms){
    if (isOpenHW() == false ){
        return;
    }
    if ((snd_hwdep_ioctl(hw, SNDRV_HDSPM_IOCTL_GET_PEAK_RMS, (void *)hdspm_peak_rms)) < 0) {
        fprintf(stderr, "HwDep ioctl failed. Metering stopped\n");
        snd_hwdep_close(hw);
        return;
    }
}

void HDSPMixerCard::getPeakRms(hdsp_peak_rms_t *hdsp_peak_rms){
    if (isOpenHW() == false ){
        return;
    }
    if ((snd_hwdep_ioctl(hw, SNDRV_HDSP_IOCTL_GET_PEAK_RMS, (void *)hdsp_peak_rms)) < 0) {
        fprintf(stderr, "HwDep ioctl failed. Metering stopped\n");
        snd_hwdep_close(hw);
        return;
    }
}

void HDSPMixerCard::setInput(int in_idx, int out_idx, int left_value,int right_value){
    setGain(channel_map_input[in_idx],dest_map[out_idx], left_value);
    setGain(channel_map_input[in_idx],dest_map[out_idx]+1, right_value);
}
void HDSPMixerCard::setPlayback(int in_idx, int out_idx, int left_value,int right_value){
    setGain(playbacks_offset+channel_map_playback[in_idx],dest_map[out_idx], left_value);
    setGain(playbacks_offset+channel_map_playback[in_idx],dest_map[out_idx]+1, right_value);
}

void HDSPMixerCard::openHW(){
    if (isOpenHW() == true ){
        return;
    }
    if (( snd_hwdep_open(&hw, name, SND_HWDEP_OPEN_READ)) < 0) {
        fprintf(stderr, "Couldn't open hwdep device. Metering stopped\n");
        return;
    }
}

void HDSPMixerCard::closeHW(){
    snd_hwdep_close(hw);
    hw = NULL;
}
bool HDSPMixerCard::isOpenHW(){
    if(hw != NULL){
        return true;
    } else {
        return false;
    }
}
void HDSPMixerCard::openCtl(){
    if (isOpenCtl() == true ){
        return;
    }
    if ((snd_ctl_open(&ctl_handle, name, SND_CTL_NONBLOCK)) < 0) {
        fprintf(stderr, "Error accessing ctl interface on card %s\n.", name);
        return ;
    }
}

void HDSPMixerCard::closeCtl(){
    snd_ctl_close(ctl_handle);
    ctl_handle = NULL;
}

bool HDSPMixerCard::isOpenCtl(){
    if(ctl_handle != NULL){
        return true;
    } else {
        return false;
    }
}
