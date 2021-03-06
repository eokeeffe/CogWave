/////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////
///////                            Author: Vincent Le Nir                                         ///////
///////                                 Royal Military Academy                                    ///////
///////           Department Communications, Information Systems & Sensors (CISS)                 ///////
///////                             30, Avenue de la Renaissance                                  ///////
///////                                B-1000 Brussels BELGIUM                                    ///////
///////                                   Tel: +3227426624                                        ///////
///////                                 email:vincent.lenir@rma.ac.be                             ///////
/////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////
#include "simulatordevice.h"



SimulatorDevice::SimulatorDevice()
{

    timeout=1.0;
    tx_rate=1.0e6;
    tx_freq=433.92e6;
    tx_gain=30.0;
    tx_amplitude=1.0;
    rx_rate=1.0e6;
    rx_freq=433.92e6;
    rx_gain=0;
    timer=0;
    cout << "Setting device timestamp to 0" << endl;
    is_sending=false;
    is_receiving=false;
    correction=0;
    previous_correction=0;
    has_sent=false;
    tx_buff.set_size(1);
    tx_buff.zeros();
    rx_buff.set_size(1);
    rx_buff.zeros();
    timestamp=0;
    tx_timestamp=0;
    duplex_mode=1;
    RNG_randomize();
    rx_timestamp=0;
    tx_timestamp2=/*randu()*1.0e-4*/0;
    channel_model= new Channel_Models(rx_rate);
    awgn_noise.set_noise(1.0e-7);
    processing_delay=/*100000.0e-6*/0;

    is_synchronized=true;


}

void SimulatorDevice::waiting_time(double time_value){

    timer=time()+time_value;

}

void SimulatorDevice::update_timer(double time_value){

    timer=time_value;
}

void SimulatorDevice::recv_vector(timestamp_data tmp){

    mutex2.lock();
    time_vector.push_back(tmp);
    //cout << "time vector has changed " << time_vector.size() << endl;
    mutex2.unlock();

}

void SimulatorDevice::setvalue(double value){


    correction=value;
    //cout << "################ CORRECTION " << correction << endl;
}


void SimulatorDevice::init()
{


    if(tx_amplitude<0)
        tx_amplitude=0;
    if(tx_amplitude>1)
        tx_amplitude=1;
    cout << "Setting TX Amplitude " << tx_amplitude << endl;


}


double SimulatorDevice::time()
{
    return timer;
}

cvec SimulatorDevice::readsamplesnow(int Nsamples)
{

    cvec rx_buff;
    rx_buff=recv(Nsamples);
    return rx_buff;

}

cvec SimulatorDevice::readsamples(int Nsamples,double timestamp)
{


    cvec rx_buff;
    if(time()>timestamp){
         //cout << "WRONG RX TIMESTAMP!!!!!!!!!!!!!!!!" << timestamp << " time " << time() << endl;
         while(time()>timestamp)
                 timestamp=timestamp+time_gap;
    }
    rx_buff=recv(Nsamples,timestamp);
    return rx_buff;

}


double SimulatorDevice::sendsamples(cvec tx_buff,double timestamp){


    tx_buff=tx_buff*tx_amplitude;
    if(time()>timestamp){
        //cout << "WRONG TX TIMESTAMP!!!!!!!!!!!!!!!! " << timestamp << " time " << time() << endl;
        while(time()>timestamp){
                timestamp=timestamp+time_gap;
        }
    }
    send(tx_buff, timestamp);
    return timestamp;

}


void SimulatorDevice::start(){

    if(duplex_mode==1){

        if(is_sending){
            //while(is_sending){

                if(time()>tx_timestamp){
                    //cout << "WRONG TX TIMESTAMP!!!!!!!!!!!!!!!!" << endl;
                    while(time()>tx_timestamp)
                            tx_timestamp=tx_timestamp+time_gap;
                }
                send(tx_buff, tx_timestamp);
                if(tx_buff2!=tx_buff){
                    tx_buff=tx_buff2;
                }
                tx_timestamp=tx_timestamp+time_gap;
                has_sent=true;
            //}
            has_sent=false;
            is_sending=false;




        }
        if(is_receiving){

            is_synchronized=false;
            while(!is_synchronized) 0;
            usleep(100000);
            cvec rx_buff2;

            //while(is_receiving){
                 if(rx_buff_size!=rx_buff.size()){
                     rx_buff_size=rx_buff.size();
                     rx_buff2.set_size(rx_buff_size);
                     rx_buff2.zeros();
                 }
                 //timestamp=timestamp+time_gap;
                 if((previous_correction!=correction)&&(abs(previous_correction-correction)*rx_rate>0)){
                      timestamp=timestamp+correction;
                      previous_correction=correction;
                      //cout << "CORRECTION!!!!!!!!!!!!!!!!!!!!!!!!" << endl;

                 }
                if(time()>timestamp){
                    //cout << "WRONG RX TIMESTAMP!!!!!!!!!!!!!!!!" << timestamp << " time " << time() << endl;
                    while(time()>timestamp)
                            timestamp=timestamp+time_gap;

                }

                //cout << my_address << " : TIMESTAMP RX " << timestamp << endl;
                rx_buff2=recv(rx_buff_size/*,timestamp*/);
                mutex.lock();
                rx_buff=rx_buff2;
                mutex.unlock();
                is_receiving=false;
             //}


        }
    }

    if(duplex_mode==2){

        if(is_sending){
            //while(is_sending){
            if(tx_buff2!=tx_buff){
                tx_buff=tx_buff2;
            }
            send(tx_buff);
            has_sent=true;
            //}
            has_sent=false;
            is_sending=false;
        }
        if(is_receiving){


            is_synchronized=false;
            while(!is_synchronized) 0;
            usleep(100000);
            int rx_buff_size=rx_buff.size();
            rx_buff2.set_size(rx_buff_size);
            rx_buff2.zeros();
            //Receiving mode
            //while(is_receiving){
                if((previous_correction!=correction)&&(abs(previous_correction-correction)>10)&&(abs(rx_buff_size-correction)>10)){
                     rx_buff2.set_size(correction);
                     rx_buff2=recv(rx_buff2.size());
                     if(correction>rx_buff_size)
                         rx_buff=rx_buff2.get(0,rx_buff_size-1);
                     else
                         rx_buff=concat(rx_buff2,zeros_c(rx_buff_size-correction));
                     rx_buff2.set_size(rx_buff_size);
                     previous_correction=correction;

                }

                rx_buff2=recv(rx_buff2.size());
                mutex.lock();
                rx_buff=rx_buff2;
                mutex.unlock();
                is_receiving=false;
            //}

        }

    }


}


void SimulatorDevice::sendsamplesnow(cvec tx_buff){

    timestamp_data tmp;
    tmp.src_address=my_address;
    tmp.vector_timestamp=time()/*+tx_buff.size()/tx_rate*/;
    tmp.vector_data=tx_buff;
    //tmp.vector_data=tx_buff;
    //cout << "Emit vector " << tx_buff.size() << " with time " << tx_timestamp << endl;
    emit send_vector(tmp);
    //time_vector=time_vector2;
}

void SimulatorDevice::send(cvec tx_buff, double tx_timestamp){

    timestamp_data tmp;
    tmp.src_address=my_address;
    tmp.vector_timestamp=tx_timestamp;
    tmp.vector_data=tx_buff;
    //tmp.vector_data=tx_buff;
    //cout << "Emit vector " << tx_buff.size() << " with time " << tx_timestamp << endl;
    emit send_vector(tmp);
    //time_vector=time_vector2;
    //timer=tx_timestamp;
    while(time()<tx_timestamp) 0;

}

void SimulatorDevice::send(cvec tx_buff){

    timestamp_data tmp;
    tmp.src_address=my_address;
    tmp.vector_timestamp=tx_timestamp2;
    //tmp.vector_timestamp=time()+tx_buff.size()/tx_rate;
    tmp.vector_data=tx_buff;
    //tmp.vector_data=tx_buff;
    //cout << "Emit vector " << tx_buff.size() << " with time " << tx_timestamp2 << endl;
    emit send_vector(tmp);
    tx_timestamp2=tx_timestamp2+tx_buff.size()/tx_rate;
    //wait until next slot but allow some time to compute the channel of next transmission
    //while(time()<tx_timestamp2-tx_buff.size()/tx_rate/2) 0;

}

cvec SimulatorDevice::recv(int rx_size){

    double tx_timestamp;
    int src_address;
    cvec received_vector(rx_size);
    cvec tmp;
    received_vector.zeros();
    received_vector=awgn_noise(received_vector);
    int i=0;
    mutex2.lock();
    vector<timestamp_data> time_vector2=time_vector;
    while(i<time_vector2.size()){
        src_address=time_vector2[i].src_address;
        tx_timestamp=time_vector2[i].vector_timestamp;
        tmp=channel_model->OTA_Transmission(time_vector2[i].vector_data,"PathLossOnly",src_address,my_address);
        //cout << my_address << " : Tx timestamp " << tx_timestamp << " with size " << tmp.size() << " - source address : "<<  src_address << endl;
        //cout << my_address << " : Rx timestamp " << rx_timestamp << " with size " << rx_size << endl;
        if((floor(tx_timestamp*tx_rate+0.5)<floor(rx_timestamp*rx_rate+rx_size+0.5))&&(floor(tx_timestamp*tx_rate+0.5)+tmp.size()>floor(rx_timestamp*rx_rate+0.5)+rx_size)){
            //cout << "1" << endl;
            received_vector.set_subvector(floor(tx_timestamp*tx_rate+0.5)-floor(rx_timestamp*rx_rate+0.5),received_vector.get(floor(tx_timestamp*tx_rate+0.5)-floor(rx_timestamp*rx_rate+0.5),rx_size-1)+tmp.get(0,rx_size-(floor(tx_timestamp*tx_rate+0.5)-floor(rx_timestamp*rx_rate+0.5))-1));
            //cout <<   rx_size-(floor(tx_timestamp*tx_rate+0.5)-floor(rx_timestamp*rx_rate+0.5)) << endl;
        }
        if((floor(tx_timestamp*tx_rate+0.5)>=floor(rx_timestamp*rx_rate+0.5))&&(floor(tx_timestamp*tx_rate+0.5)+tmp.size()<=floor(rx_timestamp*rx_rate+0.5)+rx_size)){
            //cout << "2" << endl;
            received_vector.set_subvector(floor(tx_timestamp*tx_rate+0.5)-floor(rx_timestamp*rx_rate+0.5),received_vector.get(floor(tx_timestamp*tx_rate+0.5)-floor(rx_timestamp*rx_rate+0.5),tmp.size()-1)+tmp.get(0,tmp.size()-1));
            //cout <<   tmp.size() << endl;
        }
        if((floor(tx_timestamp*tx_rate+0.5)<floor(rx_timestamp*rx_rate+0.5))&&(floor(tx_timestamp*tx_rate+0.5)+tmp.size()>floor(rx_timestamp*rx_rate+0.5))){
            //cout << "3" << endl;
            received_vector.set_subvector(0,received_vector.get(0,tmp.size()-(floor(rx_timestamp*rx_rate+0.5)-floor(tx_timestamp*tx_rate+0.5))-1)+tmp.get(floor(rx_timestamp*rx_rate+0.5)-floor(tx_timestamp*tx_rate+0.5),tmp.size()-1));
            //cout << tmp.size()-(floor(rx_timestamp*rx_rate+0.5)-floor(tx_timestamp*tx_rate+0.5)) << endl;
        }
        if(floor(tx_timestamp*tx_rate+0.5)+tmp.size()<=floor(rx_timestamp*rx_rate+0.5)){
            //cout << "4" << endl;
            time_vector2.erase(time_vector2.begin()+i);
            //emit send_vector(time_vector2);
            i--;
        }
        i++;
    }
    time_vector=time_vector2;
    mutex2.unlock();
    rx_md.time_spec=rx_timestamp;
    rx_timestamp=rx_timestamp+rx_size/rx_rate;
    timer=rx_timestamp;
    emit updated_timer(rx_timestamp+processing_delay);
    return received_vector;

}

cvec SimulatorDevice::recv(int rx_size, double rx_timestamp){

    double tx_timestamp;
    int src_address;
    cvec received_vector(rx_size);
    cvec tmp;
    received_vector.zeros();
    received_vector=awgn_noise(received_vector);
    int i=0;
    mutex2.lock();
    vector<timestamp_data> time_vector2=time_vector;
    while(i<time_vector2.size()){
        src_address=time_vector2[i].src_address;
        tx_timestamp=time_vector2[i].vector_timestamp;
        tmp=channel_model->OTA_Transmission(time_vector2[i].vector_data,"PathLossOnly",src_address,my_address);
        //cout << my_address << " : Tx timestamp " << tx_timestamp << " with size " << tmp.size() << " - source address : "<<  src_address << endl;
        //cout << my_address << " : Rx timestamp " << rx_timestamp << " with size " << rx_size << endl;
        if((floor(tx_timestamp*tx_rate+0.5)<floor(rx_timestamp*rx_rate+rx_size+0.5))&&(floor(tx_timestamp*tx_rate+0.5)+tmp.size()>floor(rx_timestamp*rx_rate+0.5)+rx_size)){
            //cout << "1" << endl;
            received_vector.set_subvector(floor(tx_timestamp*tx_rate+0.5)-floor(rx_timestamp*rx_rate+0.5),received_vector.get(floor(tx_timestamp*tx_rate+0.5)-floor(rx_timestamp*rx_rate+0.5),rx_size-1)+tmp.get(0,rx_size-(floor(tx_timestamp*tx_rate+0.5)-floor(rx_timestamp*rx_rate+0.5))-1));
            //cout <<   rx_size-(floor(tx_timestamp*tx_rate+0.5)-floor(rx_timestamp*rx_rate+0.5)) << endl;
        }
        if((floor(tx_timestamp*tx_rate+0.5)>=floor(rx_timestamp*rx_rate+0.5))&&(floor(tx_timestamp*tx_rate+0.5)+tmp.size()<=floor(rx_timestamp*rx_rate+0.5)+rx_size)){
            //cout << "2" << endl;
            received_vector.set_subvector(floor(tx_timestamp*tx_rate+0.5)-floor(rx_timestamp*rx_rate+0.5),received_vector.get(floor(tx_timestamp*tx_rate+0.5)-floor(rx_timestamp*rx_rate+0.5),tmp.size()-1)+tmp.get(0,tmp.size()-1));
            //cout <<   tmp.size() << endl;
        }
        if((floor(tx_timestamp*tx_rate+0.5)<floor(rx_timestamp*rx_rate+0.5))&&(floor(tx_timestamp*tx_rate+0.5)+tmp.size()>floor(rx_timestamp*rx_rate+0.5))){
            //cout << "3" << endl;
            received_vector.set_subvector(0,received_vector.get(0,tmp.size()-(floor(rx_timestamp*rx_rate+0.5)-floor(tx_timestamp*tx_rate+0.5))-1)+tmp.get(floor(rx_timestamp*rx_rate+0.5)-floor(tx_timestamp*tx_rate+0.5),tmp.size()-1));
            //cout << tmp.size()-(floor(rx_timestamp*rx_rate+0.5)-floor(tx_timestamp*tx_rate+0.5)) << endl;
        }
        if(floor(tx_timestamp*tx_rate+0.5)+tmp.size()<=floor(rx_timestamp*rx_rate+0.5)){
            //cout << "4" << endl;
            time_vector2.erase(time_vector2.begin()+i);
            //emit send_vector(time_vector2);
            i--;
        }
        i++;
    }
    time_vector=time_vector2;
    mutex2.unlock();
    rx_md.time_spec=rx_timestamp;
    while(tx_timestamp>rx_timestamp+rx_size/rx_rate)
          rx_timestamp=rx_timestamp+rx_size/rx_rate;
    if(rx_timestamp+rx_size/rx_rate>tx_timestamp)
        rx_timestamp=rx_timestamp-rx_size/rx_rate;
    rx_timestamp=rx_timestamp-rx_size/rx_rate;
    timer=rx_timestamp;
    emit updated_timer(rx_timestamp+processing_delay);
    return received_vector;

}

