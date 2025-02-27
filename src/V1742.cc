/**
 *  Copyright 2014 by Benjamin Land (a.k.a. BenLand100)
 *
 *  WbLSdaq is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  WbLSdaq is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with WbLSdaq. If not, see <http://www.gnu.org/licenses/>.
 */
 
#include <cmath>
#include <iostream>
#include <stdexcept>
 
#include "V1742.hh"

using namespace std;

V1742Settings::V1742Settings() : DigitizerSettings("") {
    //These are "do nothing" defaults  
    index = "DEFAULTS";
    card.tr_enable = 0; //1 bit tr enabled
    card.tr_readout = 0; //1 bit tr readout enabled
    card.tr_polarity = 0; //1 bit [positive,negative]
    card.tr0_threshold = 0x6666; //16 bit tr0 threshold
    card.tr1_threshold = 0x6666; //16 bit tr1 threshold
    card.tr0_dc_offset = 0x8000; //16 bit tr0 dc offset
    card.tr1_dc_offset = 0x8000; //16 bit tr1 dc offset
    card.custom_size = 0; //2 bit [1024, 520, 256, 136]
    card.sample_freq = 0; //2 bit [5, 2.5, 1]
    card.software_trigger_enable = 0; //1 bit bool
    card.external_trigger_enable = 0; //1 bit bool
    card.software_trigger_out = 0; //1 bit bool
    card.external_trigger_out = 0; //1 bit bool
    card.post_trigger = 0; //10 bit (8.5ns steps)
    for (uint32_t gr = 0; gr < 4; gr++) {
        groupDefaults(gr);
    }
    card.max_event_blt = 10; //8 bit events per transfer
}

V1742Settings::V1742Settings(RunTable &dgtz, RunDB &db) : DigitizerSettings(dgtz.getIndex()) {
    card.tr_enable = dgtz["tr_enabled"].cast<bool>() ? 1 : 0; //1 bit tr enabled
    card.tr_readout = dgtz["tr_readout"].cast<bool>() ? 1 : 0; //1 bit tr readout enabled
    card.tr_polarity = dgtz["tr_polarity"].cast<int>(); //1 bit [positive,negative]
    card.tr0_threshold = dgtz["tr0_threshold"].cast<int>(); //16 bit tr0 threshold
    card.tr1_threshold = dgtz["tr1_threshold"].cast<int>(); //16 bit tr1 threshold
    card.tr0_dc_offset = dgtz["tr0_dc_offset"].cast<int>(); //16 bit tr0 dc offset
    card.tr1_dc_offset = dgtz["tr1_dc_offset"].cast<int>(); //16 bit tr1 dc offset
    card.custom_size = dgtz["num_samples"].cast<int>(); //2 bit [1024, 520, 256, 136]
    card.sample_freq = dgtz["sample_freq"].cast<int>(); //2 bit [5, 2.5, 1]
    card.software_trigger_enable = dgtz["software_trigger"].cast<bool>() ? 1 : 0; //1 bit bool
    card.external_trigger_enable = dgtz["external_trigger"].cast<bool>() ? 1 : 0; //1 bit bool
    card.software_trigger_out = dgtz["software_trigger_out"].cast<bool>() ? 1 : 0; //1 bit bool
    card.external_trigger_out = dgtz["external_trigger_out"].cast<bool>() ? 1 : 0; //1 bit bool
    card.post_trigger = dgtz["trigger_offset"].cast<int>(); //10 bit (8.5ns steps)
    for (uint32_t gr = 0; gr < 4; gr++) {
        string grname = "GR"+to_string(gr);
        if (!db.tableExists(grname,index)) {
            groupDefaults(gr);
        } else {
            cout << "\t" << grname << endl;
            RunTable group = db.getTable(grname,index);
            card.group_enable[gr] = group["enabled"].cast<bool>() ? 1 : 0; //1 bit bool
            vector<double> offsets = group["dc_offsets"].toVector<double>();
            vector<bool> chmask = group["channel_mask"].toVector<bool>();
            if (offsets.size() != 8) throw runtime_error("Group DC offsets expected to be length 8");
            for (uint32_t ch = 0; ch < 8; ch++) {
                card.dc_offset[ch+gr*8] = round((-offsets[ch]+1.0)/2.0*pow(2.0,16.0)); //16 bit channel offsets
                card.channel_mask[gr][ch] = chmask[ch];
            }  
        }
    }
    card.max_event_blt = 10; //8 bit events per transfer
    
}

V1742Settings::~V1742Settings() {

}

void V1742Settings::groupDefaults(uint32_t gr) {
    card.group_enable[gr] = 0; //1 bit bool
    for (uint32_t ch = 0; ch < 8; ch++) {
        card.dc_offset[ch+gr*8] = 0x8000; //16 bit channel offsets
    }
}
        
void V1742Settings::validate() {
    if (card.tr_enable & (~0x1)) throw runtime_error("tr_enable must be 1 bit");
    if (card.tr_readout & (~0x1)) throw runtime_error("tr_readout must be 1 bit");
    if (card.tr_polarity & (~0x1)) throw runtime_error("tr_polarity must be 1 bit");
    if (card.custom_size > 3) throw runtime_error("custom_size must be < 4");
    if (card.sample_freq > 2) throw runtime_error("tr_polarity must be < 3");
    if (card.software_trigger_enable & (~0x1)) throw runtime_error("software_trigger_enable must be 1 bit");
    if (card.external_trigger_enable & (~0x1)) throw runtime_error("external_trigger_enable must be 1 bit");
    if (card.software_trigger_out & (~0x1)) throw runtime_error("software_trigger_out must be 1 bit");
    if (card.external_trigger_out & (~0x1)) throw runtime_error("external_trigger_out must be 1 bit");
    if (card.post_trigger > 1023) throw runtime_error("post_trigger must be < 1024");
    for (uint32_t gr = 0; gr < 4; gr++) {
        if (card.group_enable[gr] & (~0x1)) throw runtime_error("external_trigger_enable must be 1 bit");
    }

}

V1742calib::V1742calib(CAEN_DGTZ_DRS4Correction_t *dat) {
    for (size_t gr = 0; gr < 4; gr++) {
        CAEN_DGTZ_DRS4Correction_t &cal = dat[gr];
        for (size_t i = 0; i < 1024; i++) {
            groups[gr].cell_delay[i] = cal.time[i];
        }
        for (size_t ch = 0; ch < 9; ch++) {
            for (size_t i = 0; i < 1024; i++) {
                groups[gr].chans[ch].cell_offset[i] = cal.cell[ch][i];
                groups[gr].chans[ch].seq_offset[i] = cal.nsample[ch][i];
            }
        }
    }
}

V1742calib::~V1742calib() {

}

void V1742calib::calibrate(uint16_t *samples[4][8], uint16_t *trn_samples[4], size_t sampPerEv, uint16_t *start_index[4], bool grActive[4], bool trActive[4], size_t numEv) {
    
    cout << "\tCalibrating V1742 data..." << endl;
    
    for (size_t gr = 0; gr < 4; gr++) {
        if (!grActive[gr]) continue;
        for (size_t ev = 0; ev < numEv; ev++) {
            uint16_t *samps[9];
            uint16_t cellidx = start_index[gr][ev];
            for (size_t ch = 0; ch < (trActive[gr] ? 9 : 8); ch++) {
                if (ch < 8) {
                    samps[ch] = samples[gr][ch]+ev*sampPerEv;
                } else {
                    samps[ch] = trn_samples[gr]+ev*sampPerEv;
                }
                //Apply CAEN offsets 
                for (size_t i = 0; i < sampPerEv; i++) {
                    if (samps[ch][i] == 0 || samps[ch][i] == 4095) continue; //don't correct rails
                    samps[ch][i] = samps[ch][i] - groups[gr].chans[ch].seq_offset[i] - groups[gr].chans[ch].cell_offset[(cellidx+i)%1024];
                    if (samps[ch][i] >= 0xF000) {
                        samps[ch][i] = 0; //fix correction below lower rail
                    } else if (samps[ch][i] >= 0x0FFF) {
                        samps[ch][i] = 0x0FFF; //fix correction above upper rail 
                    }
                }
            }
            for (size_t i = 0; i < sampPerEv; i++) {
                int identified = 0;
                for (size_t ch = 0; ch < (trActive[gr] ? 9 : 8); ch++) {
                    if (samps[ch][i]-samps[ch][(i+1)%sampPerEv] > 30 && samps[ch][(i+3)%sampPerEv]-samps[ch][(i+2)%sampPerEv] > 30) {
                        identified++;
                    }
                }
                if (identified > 4) {
                    for (size_t ch = 0; ch < 8; ch++) {
                        samps[ch][(i+1)%sampPerEv] += 53;
                        samps[ch][(i+2)%sampPerEv] += 53;
                    }
                }
            }
        }
    }

}

V1742::V1742(VMEBridge &_bridge, uint32_t _baseaddr) : Digitizer(_bridge,_baseaddr) {

}

V1742::~V1742() {
    //Fully reset the board just in case
    write32(REG_BOARD_CONFIGURATION_RELOAD,0);
}

bool V1742::program(DigitizerSettings &_settings) {
    V1742Settings &settings = dynamic_cast<V1742Settings&>(_settings);
    try {
        settings.validate();
    } catch (runtime_error &e) {
        cout << "Could not program V1742: " << e.what() << endl;
        return false;
    }
    
    uint32_t data;
    
    //Fully reset the board just in case
    write32(REG_BOARD_CONFIGURATION_RELOAD,0);
    
    usleep(10000);

    //Print firmware version
    cout << "Current firmware version: " << read32(0x8124) << "\n\n";
    
    //Set TTL logic levels, ignore LVDS and debug settings
    data = (1<<0) // ttl levels
         | (0<<2) | (0<<3) | (0<<4) | (0<<5) // lvds all input
         | (2<<6) // pattern mode
         | (0<<14) // trgout level
         | (0<<15);// trgout ctrl
    write32(REG_FRONT_PANEL_CONTROL,data);
    
    write32(REG_TR_THRESHOLD|(0<<8),settings.card.tr0_threshold);
    write32(REG_TR_THRESHOLD|(2<<8),settings.card.tr1_threshold);
    write32(REG_TR_DC_OFFSET|(0<<8),settings.card.tr0_dc_offset);
    write32(REG_TR_DC_OFFSET|(2<<8),settings.card.tr1_dc_offset);
    
    data = (((uint32_t)settings.card.tr_enable)<<12)
         | (((uint32_t)settings.card.tr_readout)<<11)
         | (1<<8)
         | (((uint32_t)settings.card.tr_polarity)<<6)
         | (1<<4);
    write32(REG_GROUP_CONFIG,data);
    
    write32(REG_CUSTOM_SIZE,settings.card.custom_size);
    write32(REG_SAMPLE_FREQ,settings.card.sample_freq);
    write32(REG_POST_TRIGGER,settings.card.post_trigger);
    
    data = (((uint32_t)settings.card.software_trigger_enable)<<31)
         | (((uint32_t)settings.card.external_trigger_enable)<<30);
    write32(REG_TRIGGER_SOURCE,data);
    
    
    data = (((uint32_t)settings.card.software_trigger_out)<<31)
         | (((uint32_t)settings.card.external_trigger_out)<<30);
    write32(REG_TRIGGER_OUT,data);
    
    uint32_t group_enable = 0;
    for (uint32_t gr = 0; gr < 4; gr++) {
        group_enable |= settings.card.group_enable[gr]<<gr;
        for (uint32_t ch = 0; ch < 8; ch++) {
            data = (ch<<16) | ((uint32_t)settings.card.dc_offset[ch+gr*8]);
            write32(REG_DC_OFFSET|(gr<<8),data);
        }
    }
    write32(REG_GROUP_ENABLE,group_enable);
    
    //Set max board aggregates to transver per readout
    write32(REG_MAX_EVENT_BLT,settings.card.max_event_blt);
    
    //Enable VME BLT readout
    write32(REG_READOUT_CONTROL,1<<4);
    
    return true;
}

void V1742::softTrig() {
    write32(REG_SOFTWARE_TRIGGER,0xDEADBEEF);
}

void V1742::startAcquisition() {
    write32(REG_ACQUISITION_CONTROL,(1<<3)|(1<<2));
}

void V1742::stopAcquisition() {
    write32(REG_ACQUISITION_CONTROL,0);
}

bool V1742::acquisitionRunning() {
    return read32(REG_ACQUISITION_STATUS) & (1 << 2);
}

bool V1742::readoutReady() {
    return read32(REG_ACQUISITION_STATUS) & (1 << 3);
}

bool V1742::checkTemps(vector<uint32_t> &temps, uint32_t danger) {
    temps.resize(4);
    bool over = false;
    for (int gr = 0; gr < 4; gr++) {
        temps[gr] = read32(REG_DRS4_TEMP|(gr<<8))&0xFF;
        if (temps[gr] >= danger) over = true;
    }
    return over;
}

V1742calib* V1742::staticGetCalib(V1742SampleFreq freq, int link, uint32_t baseaddr) {
    int handle = 0;
    int res = CAEN_DGTZ_OpenDigitizer(CAEN_DGTZ_USB, link, 0, baseaddr, &handle);
    if (res != 0) throw runtime_error("getCalib: Could not open digitizer "+to_string(res));
    
    CAEN_DGTZ_DRS4Correction_t corr[4];
    switch (freq) {
        case GHz_5:
            res = CAEN_DGTZ_GetCorrectionTables(handle,CAEN_DGTZ_DRS4_5GHz,&corr);
            break;
        case GHz_2_5:
            res = CAEN_DGTZ_GetCorrectionTables(handle,CAEN_DGTZ_DRS4_2_5GHz,&corr);
            break;
        case GHz_1:
            res = CAEN_DGTZ_GetCorrectionTables(handle,CAEN_DGTZ_DRS4_1GHz,&corr);
            break;
        default: throw runtime_error("getCalib: Invalid sample rate");
    }
    if (res != 0) throw runtime_error("getCalib: Could not get calibration data "+to_string(res));
    
    res = CAEN_DGTZ_CloseDigitizer(handle);
    if (res != 0) throw runtime_error("getCalib: Could not close digitizer "+to_string(res));
    
    return new V1742calib(corr);
}


V1742calib* V1742::getCalib(V1742SampleFreq freq) {
    //This ***will not work*** here due to a bug in CAENDigitizer
    //call the static version BEFORE invoking any CAENVME methods
    return staticGetCalib(freq,bridge.getLinkNum(),baseaddr);
}

V1742Decoder::V1742Decoder(size_t _eventBuffer, V1742calib *_calib, V1742Settings &_settings) : eventBuffer(_eventBuffer), calib(_calib), settings(_settings) {

    dispatch_index = group_counter = event_counter = decode_counter = 0;
    
    nSamples = settings.getNumSamples();
    for (size_t gr = 0; gr < 4; gr++) {
        if (settings.getGroupEnabled(gr)) {
            grActive[gr] = true;
            grGrabbed[gr] = 0;
            if (eventBuffer) {
                for (size_t ch = 0; ch < 8; ch++) {
                    samples[gr][ch] = new uint16_t[eventBuffer*nSamples];
                }
                start_index[gr] = new uint16_t[eventBuffer];
                patterns[gr] = new uint16_t[eventBuffer];
                trigger_count[gr] = new uint32_t[eventBuffer];
                trigger_time[gr] = new uint32_t[eventBuffer];
            }
        } else {
            grActive[gr] = false;
        }
        for (size_t ch = 0; ch < 8; ch++) {
            chActive[gr][ch] = settings.getChannelMask(gr,ch);
        }
    }
    
    if (settings.getTrReadout() && eventBuffer) {
        for (size_t gr = 0; gr < 4; gr++) {
            if (settings.getGroupEnabled(gr)) {
                trn_samples[gr] = new uint16_t[eventBuffer*nSamples];
                trnActive[gr] = true;
            }
        }
    } else {
        trnActive[0] = trnActive[1] = trnActive[2] = trnActive[3] = false;
    }
    
    clock_gettime(CLOCK_MONOTONIC,&last_decode_time);
    
}

V1742Decoder::~V1742Decoder() {
    if (calib) delete calib;
    if (eventBuffer) {
        for (size_t gr = 0; gr < 4; gr++) {
            if (grActive[gr]) {
                for (size_t ch = 0; ch < 8; ch++) {
                    delete [] samples[gr][ch];
                }
                delete [] patterns[gr];
                delete [] start_index[gr];
                delete [] trigger_count[gr];
                delete [] trigger_time[gr];
            }
            if (trnActive[gr]) delete [] trn_samples[gr];
        }
    }
}

void V1742Decoder::decode(Buffer &buffer) {
    size_t lastgrabbed[4]; 
    for (size_t gr = 0; gr < 4; gr++) lastgrabbed[gr] = grGrabbed[gr];
    
    decode_size = buffer.fill();
    cout << settings.getIndex() << " decoding " << decode_size << " bytes." << endl;
    uint32_t *next = (uint32_t*)buffer.rptr(), *start = (uint32_t*)buffer.rptr();
    while ((size_t)((next = decode_event_structure(next)) - start + 1)*4 < decode_size);
    buffer.dec(decode_size);
    decode_counter++;
    
    struct timespec cur_time;
    clock_gettime(CLOCK_MONOTONIC,&cur_time);
    double time_int = (cur_time.tv_sec - last_decode_time.tv_sec)+1e-9*(cur_time.tv_nsec - last_decode_time.tv_nsec);
    last_decode_time = cur_time;
    
    for (size_t gr = 0; gr < 4; gr++) {
        if (grActive[gr]) cout << "\tgr" << gr << "\tev: " << grGrabbed[gr]-lastgrabbed[gr] << " / " << (grGrabbed[gr]-lastgrabbed[gr])/time_int << " Hz / " << grGrabbed[gr] << " total " << endl;
    }
}
    
uint32_t* V1742Decoder::decode_event_structure(uint32_t *event) {
    if (event[0] == 0xFFFFFFFF) {
        event++; //sometimes padded
    }
    if ((event[0] & 0xF0000000) != 0xA0000000) 
        throw runtime_error("Event structure missing tag");
    
    uint32_t size = event[0] & 0xFFFFFFF;
    
    //uint32_t board_id = (event[1] & 0xF800000000) >> 27;
    uint32_t pattern = (event[1] & 0x7FFF00) >> 8;
    uint32_t mask = event[1] & 0xF;
    uint32_t count = event[2] & 0x3FFFFF;
    uint32_t timetag = event[3];
    
    cout << "\t(LVDS & 0xFF): " << (pattern&0xFF) << endl; 
    
    if (event_counter++) {
        if (count == trigger_last) {
            cout << "****" << settings.getIndex() << " duplicate trigger " << count << endl;
        } else if (count < trigger_last) {
            cout << "****" << settings.getIndex() << " orphaned trigger " << count << endl;
        } else if (count != trigger_last + 1) { 
            cout << "****" << settings.getIndex() << " missed " << count-trigger_last-1 << " triggers" << endl;
            trigger_last = count;
        } else {
            trigger_last = count;
        }
    } else {
        trigger_last = count;
    }
    
    uint32_t *groups = event+4;
    
    for (uint32_t gr = 0; gr < 4; gr++) {
        if (mask & (1 << gr)) {
            size_t ev = grGrabbed[gr]++;
            if (eventBuffer) {
                if (ev == eventBuffer) throw runtime_error("Decoder buffer for " + settings.getIndex() + " overflowed!");
                patterns[gr][ev] = pattern;
                trigger_time[gr][ev] = timetag;
                trigger_count[gr][ev] = count;
            }
            groups = decode_group_structure(groups,gr);
        }
    } 
    
    return event+size;

}

uint32_t* V1742Decoder::decode_group_structure(uint32_t *group, uint32_t gr) {

    if (!grActive[gr]) throw runtime_error("Recieved group data for inactive group (" + to_string(gr) + ")");
    
    uint32_t cell_index = (group[0] & 0x3FF00000) >> 20;
    //uint32_t freq = (group[0] >> 16) & 0x3;
    
    uint32_t tr = (group[0] >> 12) & 0x1;
    uint32_t size = group[0] & 0xFFF;
    
    if (size/3 != nSamples) throw runtime_error("Recieved sample length " + to_string(size/3) + " does not match expected " + to_string(nSamples) + " (" + to_string(gr) + ")");
    if (tr && !settings.getTrReadout()) throw runtime_error("Received TR"+to_string(gr/2)+" data when not marked for readout (" + to_string(gr) + ")");
    
    group_counter++;
    
    if (eventBuffer) {
        size_t ev = grGrabbed[gr]-1;
        
        start_index[gr][ev] = cell_index;
        
        uint32_t *word = group+1;
        uint16_t *data[8];
        for (size_t ch = 0; ch < 8; ch++) data[ch] = samples[gr][ch] + ev*nSamples;
        for (size_t s = 0; s < nSamples; s++, word += 3) {
            data[0][s] = word[0]&0xFFF;
            data[1][s] = (word[0]>>12)&0xFFF;
            data[2][s] = ((word[1]&0xF)<<8)|((word[0]>>24)&0xFF);
            data[3][s] = (word[1]>>4)&0xFFF;
            data[4][s] = (word[1]>>16)&0xFFF;
            data[5][s] = ((word[2]&0xFF)<<4)|((word[1]>>28)&0xF);
            data[6][s] = (word[2]>>8)&0xFFF;
            data[7][s] = (word[2]>>20)&0xFFF;
        }
        
        if (tr && trnActive[gr]) {
            uint16_t *data = trn_samples[gr] + ev*nSamples;
            for (size_t s = 0; s < nSamples; word += 3) {
                data[s++] = word[0]&0xFFF;
                data[s++] = (word[0]>>12)&0xFFF;
                data[s++] = ((word[1]&0xF)<<8)|((word[0]>>24)&0xFF);
                data[s++] = (word[1]>>4)&0xFFF;
                data[s++] = (word[1]>>16)&0xFFF;
                data[s++] = ((word[2]&0xFF)<<4)|((word[1]>>28)&0xF);
                data[s++] = (word[2]>>8)&0xFFF;
                data[s++] = (word[2]>>20)&0xFFF;
            }
        }
        
    }
    
    return group + 2 + size + (tr ? size/8 : 0);
    
}

size_t V1742Decoder::eventsReady() {
    size_t grabs = INT64_MAX;//eventBuffer;
    for (size_t gr = 0; gr < 4; gr++) {
        if (grActive[gr] && grGrabbed[gr] < grabs) grabs = grGrabbed[gr];
    }
    return grabs;
}

// length, lvdsidx, dsize, nsamples, samples[], strlen, strname[]

void V1742Decoder::dispatch(int nfd, int *fds) {
    
    size_t ready = eventsReady();
    
    for ( ; dispatch_index < ready; dispatch_index++) {
        for (size_t gr = 0; gr < 4; gr++) {
            if (!grActive[gr]) continue;
            for (size_t ch = 0; ch < 8; ch++) {
                if (!chActive[gr][ch]) continue;
                uint8_t lvdsidx = patterns[gr][dispatch_index] & 0xFF; 
                uint8_t dsize = 2;
                uint16_t nsamps = nSamples;
                uint16_t *samps = &samples[gr][ch][nsamps*dispatch_index];
                string strname = "/"+settings.getIndex()+"/gr" + to_string(gr) + "/ch" + to_string(ch);
                uint16_t strlen = strname.length();
                uint16_t length = 2+strlen+2+nsamps*2+1+1;
                for (int j = 0; j < nfd; j++) {
                    writeall(fds[j],&length,2);
                    writeall(fds[j],&lvdsidx,1);
                    writeall(fds[j],&dsize,1);
                    writeall(fds[j],&nsamps,2);
                    writeall(fds[j],samps,nsamps*2);
                    writeall(fds[j],&strlen,2);
                    writeall(fds[j],strname.c_str(),strlen);
                }
            }
        }
    }
}

using namespace H5;

void V1742Decoder::writeOut(H5File &file, size_t nEvents) {

    if (calib) calib->calibrate(samples, trn_samples, nSamples, start_index, grActive, trnActive, nEvents);

    cout << "\t/" << settings.getIndex() << endl;

    Group cardgroup = file.createGroup("/"+settings.getIndex());
        
    DataSpace scalar(0,NULL);
    
    double dval;
    uint32_t ival;
    
    Attribute bits = cardgroup.createAttribute("bits",PredType::NATIVE_UINT32,scalar);
    ival = 12;
    bits.write(PredType::NATIVE_INT32,&ival);
    
    Attribute ns_sample = cardgroup.createAttribute("ns_sample",PredType::NATIVE_DOUBLE,scalar);
    dval = settings.nsPerSample();
    ns_sample.write(PredType::NATIVE_DOUBLE,&dval);
            
    Attribute _samples = cardgroup.createAttribute("samples",PredType::NATIVE_UINT32,scalar);
    ival = nSamples;
    _samples.write(PredType::NATIVE_UINT32,&ival);
    
    for (size_t gr = 0; gr < 4; gr++) {
        if (!grActive[gr]) continue;
        string grname = "gr" + to_string(gr);
        Group grgroup = cardgroup.createGroup(grname);
        string grgroupname = "/"+settings.getIndex()+"/"+grname;
        
        cout << "\t" << grgroupname << endl;

        hsize_t dimensions[2];
        dimensions[0] = nEvents;
        dimensions[1] = nSamples;
        
        DataSpace samplespace(2, dimensions);
        DataSpace metaspace(1, dimensions);
        
        for (size_t ch = 0; ch < 8; ch++) {
            if (!chActive[gr][ch]) continue;
            string chname = "ch" + to_string(ch);
            Group chgroup = grgroup.createGroup(chname);
            string chgroupname = "/"+settings.getIndex()+"/"+grname+"/"+chname;
            
            cout << "\t" << chgroupname << endl;
        
            Attribute offset = chgroup.createAttribute("offset",PredType::NATIVE_UINT32,scalar);
            ival = settings.getDCOffset(gr*8+ch);
            offset.write(PredType::NATIVE_UINT32,&ival);
            
            cout << "\t" << chgroupname << "/samples" << endl;
            DataSet samples_ds = file.createDataSet(chgroupname+"/samples", PredType::NATIVE_UINT16, samplespace);
            samples_ds.write(samples[gr][ch], PredType::NATIVE_UINT16);
            memmove(samples[gr][ch],samples[gr][ch]+nEvents*nSamples,sizeof(uint16_t)*nSamples*(grGrabbed[gr]-nEvents));
        }
        
        if (trnActive[gr]) {
            string chname = "tr";
            Group chgroup = grgroup.createGroup(chname);
            string chgroupname = "/"+settings.getIndex()+"/"+grname+"/"+chname;
            
            cout << "\t" << chgroupname << endl;
        
            Attribute offset = chgroup.createAttribute("offset",PredType::NATIVE_UINT32,scalar);
            ival = settings.getTrDCOffset(gr/2);
            offset.write(PredType::NATIVE_UINT32,&ival);
            
            cout << "\t" << chgroupname << "/samples" << endl;
            DataSet samples_ds = file.createDataSet(chgroupname+"/samples", PredType::NATIVE_UINT16, samplespace);
            samples_ds.write(trn_samples[gr], PredType::NATIVE_UINT16);
            memmove(trn_samples[gr],trn_samples[gr]+nEvents*nSamples,sizeof(uint16_t)*nSamples*(grGrabbed[gr]-nEvents));
        }
            
        cout << "\t" << grgroupname << "/start_index" << endl;
        DataSet start_index_ds = file.createDataSet(grgroupname+"/start_index", PredType::NATIVE_UINT16, metaspace);
        start_index_ds.write(start_index[gr], PredType::NATIVE_UINT16);
        memmove(start_index[gr],start_index[gr]+nEvents,sizeof(uint16_t)*(grGrabbed[gr]-nEvents));
        
        cout << "\t" << grgroupname << "/patterns" << endl;
        DataSet patterns_ds = file.createDataSet(grgroupname+"/patterns", PredType::NATIVE_UINT16, metaspace);
        patterns_ds.write(patterns[gr], PredType::NATIVE_UINT16);
        memmove(patterns[gr],patterns[gr]+nEvents,sizeof(uint16_t)*(grGrabbed[gr]-nEvents));
            
        cout << "\t" << grgroupname << "/trigger_time" << endl;
        DataSet trigger_time_ds = file.createDataSet(grgroupname+"/trigger_time", PredType::NATIVE_UINT32, metaspace);
        trigger_time_ds.write(trigger_time[gr], PredType::NATIVE_UINT32);
        memmove(trigger_time[gr],trigger_time[gr]+nEvents,sizeof(uint32_t)*(grGrabbed[gr]-nEvents));
        
        cout << "\t" << grgroupname << "/trigger_count" << endl;
        DataSet trigger_count_ds = file.createDataSet(grgroupname+"/trigger_count", PredType::NATIVE_UINT32, metaspace);
        trigger_count_ds.write(trigger_count[gr], PredType::NATIVE_UINT32);
        memmove(trigger_count[gr],trigger_count[gr]+nEvents,sizeof(uint32_t)*(grGrabbed[gr]-nEvents));
        
        grGrabbed[gr] -= nEvents;
    }
    
    dispatch_index -= nEvents;
    if (dispatch_index < 0) dispatch_index = 0;
}
