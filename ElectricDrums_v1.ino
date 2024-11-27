/*
＜修正点＞
・トリガリングのアルゴリズムを変更
・入力に複数のピエゾを用いる⇒打点推定モデルの実装
・打点推定を元に、打点のセンター/エッジで異なる音声を再生
・A0に用いるセンサをフェーダーに変更
・レイテンシ対策
*/

#include <MozziGuts.h>
#include <Sample.h>
#include <samples/tomc.h>
#include <samples/tome.h>
Sample <tomc_NUM_CELLS, AUDIO_RATE> tomc(tomc_DATA); //タム(センター)の音声データ
Sample <tome_NUM_CELLS, AUDIO_RATE> tome(tome_DATA); //タム(エッジ)の音声データ

#define CONTROL_RATE 64


//入力(フェーダー)に関わる変数
int pitch;


//入力(ピエゾ素子)に関わる変数
int piezo0_present;
int piezo0_past;
int piezo0_delta_present;
int piezo0_delta_past;

int piezo1_present;
int piezo1_past;
int piezo1_delta_present;
int piezo1_delta_past;

int piezo2_present;
int piezo2_past;
int piezo2_delta_present;
int piezo2_delta_past;

int piezo3_present;
int piezo3_past;
int piezo3_delta_present;
int piezo3_delta_past;


//トリガリングに関わる定数・変数
const int THRESHOLD_CENTER = 100;
const int THRESHOLD_EDGE = 30;
const int PASS_CENTER = 100;
const int PASS_EDGE = 50;
const int COUNT = 4;
const int LIMITTIME = 8;
const int COOLTIME = 4;

int count0;
bool count0_flag;
int cooltime0;
int count1;
bool count1_flag;
int cooltime1;
int count2;
bool count2_flag;
int cooltime2;
int count3;
bool count3_flag;
int cooltime3;

int count_all;
bool count_all_flag;


//ゲイン推定に関わる変数
int gain0_predict;
bool gain0_ok;
int gain1_predict;
bool gain1_ok;
int gain2_predict;
bool gain2_ok;
int gain3_predict;
bool gain3_ok;

int gain[4] = {0};
int gain_edge;

//出力に関わる定数・変数
int gain_out; //0-1023
byte volume = 0; //0-255
bool mode; //センターかエッジか
const float BasicFrequency_c = (float) tomc_SAMPLERATE / (float) tomc_NUM_CELLS;  // 音源を再生するサンプリング周波数
const float BasicFrequency_e = (float) tome_SAMPLERATE / (float) tome_NUM_CELLS;  // 音源を再生するサンプリング周波数

float freq_c; //再生時のサンプリング周波数
float freq_e;


//初期設定
void setup() {
  //Serial.begin(9600);
  startMozzi(CONTROL_RATE);
  count0 = 0;
  piezo0_past = 0;
  cooltime0 = COOLTIME;
  count0_flag = false;
  gain0_ok = false;

  count1 = 0;
  piezo1_past = 0;
  cooltime1 = COOLTIME;
  count1_flag = false;
  gain1_ok = false;

  count2 = 0;
  piezo2_past = 0;
  cooltime2 = COOLTIME;
  count2_flag = false;
  gain2_ok = false;

  count3 = 0;
  piezo3_past = 0;
  cooltime3 = COOLTIME;
  count3_flag = false;
  gain3_ok = false;

  count_all = 0;
  count_all_flag = false;
}

void updateControl() {
  pitch = analogRead(A0);
  freq_c = BasicFrequency_c * pow(2, pitch/512.000) / 2.00;
  freq_e = BasicFrequency_e * pow(2, pitch/512.000) / 2.00;
  tomc.setFreq(freq_c);
  tome.setFreq(freq_e);

  piezo0_present = analogRead(A2);
  piezo1_present = analogRead(A3);
  piezo2_present = analogRead(A4);
  piezo3_present = analogRead(A5);
  //Serial.println(piezo0_present);

  //piezo0
  piezo0_delta_present = piezo0_present - piezo0_past;
  if(count0_flag==true && count_all_flag==true){
    if(gain0_predict >= piezo0_present){
      count0 += 1;
      if(count0==COUNT){
        count0 = 0;
        count0_flag = false;
        gain[0] = gain0_predict;
        cooltime0 = 0;
        //Serial.print("gain0: ");
        //Serial.println(gain[0]);
        gain0_ok = true;
      }

    }else{
      count0 = 1;
      gain0_predict = piezo0_present;
    }
  }else if(count0_flag==false){
    if(cooltime0 < COOLTIME){
      cooltime0 += 1;
    }
    if(piezo0_past > THRESHOLD_CENTER && cooltime0 >= COOLTIME){
      if(piezo0_delta_present<0 && piezo0_delta_past>=0){
        count0 = 1;
        count0_flag = true;
        if(count_all_flag==false){
          count_all_flag = true;
          count_all = 1;
          //Serial.println("countStart:0");
        }
        gain0_predict = piezo0_past;
      }
    }
  }

  //piezo1
  piezo1_delta_present = piezo1_present - piezo1_past;
  if(count1_flag==true && count_all_flag==true){
    if(gain1_predict >= piezo1_present){
      count1 += 1;
      if(count1==COUNT){
        count1 = 0;
        count1_flag = false;
        gain[1] = gain1_predict;
        cooltime1 = 0;
        //Serial.print("gain1: ");
        //Serial.println(gain[1]);
        gain1_ok = true;
      }

    }else{
      count1 = 1;
      gain1_predict = piezo1_present;
    }
  }else if(count1_flag==false){
    if(cooltime1 < COOLTIME){
      cooltime1 += 1;
    }
    if(piezo1_past > THRESHOLD_EDGE && cooltime1 >= COOLTIME){
      if(piezo1_delta_present<0 && piezo1_delta_past>=0){
        count1 = 1;
        count1_flag = true;
        if(count_all_flag==false){
          count_all_flag = true;
          count_all = 1;
          //Serial.println("countStart:1");
        }
        gain1_predict = piezo1_past;
      }
    }
  }

  //piezo2
  piezo2_delta_present = piezo2_present - piezo2_past;
  if(count2_flag==true && count_all_flag==true){
    if(gain2_predict >= piezo2_present){
      count2 += 1;
      if(count2==COUNT){
        count2 = 0;
        count2_flag = false;
        gain[2] = gain2_predict;
        cooltime2 = 0;
        //Serial.print("gain2: ");
        //Serial.println(gain[2]);
        gain2_ok = true;
      }

    }else{
      count2 = 1;
      gain2_predict = piezo2_present;
    }
  }else if(count2_flag==false){
    if(cooltime2 < COOLTIME){
      cooltime2 += 1;
    }
    if(piezo2_past > THRESHOLD_EDGE && cooltime2 >= COOLTIME){
      if(piezo2_delta_present<0 && piezo2_delta_past>=0){
        count2 = 1;
        count2_flag = true;
        if(count_all_flag==false){
          count_all_flag = true;
          count_all = 1;
          //Serial.println("countStart:2");
        }
        gain2_predict = piezo2_past;
      }
    }
  }

  //piezo3
  piezo3_delta_present = piezo3_present - piezo3_past;
  if(count3_flag==true && count_all_flag==true){
    if(gain3_predict >= piezo3_present){
      count3 += 1;
      if(count3==COUNT){
        count3 = 0;
        count3_flag = false;
        gain[3] = gain3_predict;
        cooltime3 = 0;
        //Serial.print("gain3: ");
        //Serial.println(gain[3]);
        gain3_ok = true;
      }

    }else{
      count3 = 1;
      gain3_predict = piezo3_present;
    }
  }else if(count3_flag==false){
    if(cooltime3 < COOLTIME){
      cooltime3 += 1;
    }
    if(piezo3_past > THRESHOLD_EDGE && cooltime3 >= COOLTIME){
      if(piezo3_delta_present<0 && piezo3_delta_past>=0){
        count3 = 1;
        count3_flag = true;
        if(count_all_flag==false){
          count_all_flag = true;
          count_all = 1;
          //Serial.println("countStart:3");
        }
        gain3_predict = piezo3_past;
      }
    }
  }


  if(count_all_flag==true){
    count_all += 1;
    if(count_all < LIMITTIME){
      if(gain0_ok+gain1_ok+gain2_ok+gain3_ok==4){
        gain_edge = 0;
        for(int j=1;j<4;j++){
          gain_edge = max(gain_edge,gain[j]);
        }
        if(gain[0]>=gain_edge){
          gain_out = gain[0];
          mode = true;
          //Serial.print("gain_out[center]: ");
          //Serial.println(gain_out);
          volume = map(gain_out,0,1023,0,255);
          tomc.start(); //発音
        }else{
          gain_out = gain_edge;
          mode = false;
          //Serial.print("gain_out[edge]: ");
          //Serial.println(gain_out);
          volume = map(gain_out,0,1023,0,255);
          tome.start(); //発音
        }

        /*
        for(int i=0;i<4;i++){
          Serial.print(gain[i]);
          Serial.print(" ");
          gain[i] = 0;
        }
        Serial.println("all_prepared");
        Serial.println(" ");
        */

        gain0_ok = false;
        gain1_ok = false;
        gain2_ok = false;
        gain3_ok = false;
        count_all = 0;
        count_all_flag = false;
        gain0_predict = 0;
        gain1_predict = 0;
        gain2_predict = 0;
        gain3_predict = 0;
      }
    }else{
      if(gain0_ok+gain1_ok+gain2_ok+gain3_ok >= 2){
        gain_edge = 0;
        for(int j=1;j<4;j++){
          gain_edge = max(gain_edge,gain[j]);
        }
        if(gain[0]>=gain_edge){
          gain_out = gain[0];
          mode = true;
          //Serial.print("gain_out[center]: ");
          //Serial.println(gain_out);
          volume = map(gain_out,0,1023,0,255);
          tomc.start(); //発音
        }else{
          gain_out = gain_edge;
          mode = false;
          //Serial.print("gain_out[edge]: ");
          //Serial.println(gain_out);
          volume = map(gain_out,0,1023,0,255);
          tome.start(); //発音
        }

        /*
        for(int i=0;i<4;i++){
          Serial.print(gain[i]);
          Serial.print(" ");
          gain[i] = 0;
        }
        Serial.println("ended");
        */
          
      }else if(gain0_ok+gain1_ok+gain2_ok+gain3_ok == 1){
        if(gain[0]>=PASS_CENTER){
          gain_out = gain[0];
          mode = true;
          //Serial.print("gain_out[center]: ");
          //Serial.println(gain_out);
          volume = map(gain_out,0,1023,0,255);
          tomc.start(); //発音
        }else if(gain[1]>=PASS_EDGE){
          gain_out = gain[1];
          mode = false;
          //Serial.print("gain_out[edge]: ");
          //Serial.println(gain_out);
          volume = map(gain_out,0,1023,0,255);
          tome.start(); //発音
        }else if(gain[2]>=PASS_EDGE){
          gain_out = gain[2];
          mode = false;
          //Serial.print("gain_out[edge]: ");
          //Serial.println(gain_out);
          volume = map(gain_out,0,1023,0,255);
          tome.start(); //発音
        }else if(gain[3]>=PASS_EDGE){
          gain_out = gain[3];
          mode = false;
          //Serial.print("gain_out[edge]: ");
          //Serial.println(gain_out);
          volume = map(gain_out,0,1023,0,255);
          tome.start(); //発音
        }

        /*
        for(int i=0;i<4;i++){
          Serial.print(gain[i]);
          Serial.print(" ");
          gain[i] = 0;
        }
        Serial.println("abolished");
        //Serial.println();
        */

      }else{
        ;
        /*
        for(int i=0;i<4;i++){
          Serial.print(gain[i]);
          Serial.print(" ");
          gain[i] = 0;
        }
        Serial.println("abolished");
        Serial.println(" ");
        */
      }
      /*
      if(gain0_ok==true && gain1_ok+gain2_ok+gain3_ok > 0){
        gain_edge = 0;
        for(int j=1;j<4;j++){
          gain_edge = max(gain_edge,gain[j]);
        }
        if(gain[0]>=gain_edge){
          gain_out = gain[0];
          mode = true;
          //Serial.print("gain_out[center]: ");
          //Serial.println(gain_out);
          volume = map(gain_out,0,1023,0,255);
          tomc.start(); //発音
        }else{
          gain_out = gain_edge;
          mode = false;
          //Serial.print("gain_out[edge]: ");
          //Serial.println(gain_out);
          volume = map(gain_out,0,1023,0,255);
          //tome.start(); //発音
          tomc.start(); //代替
        }

        for(int i=0;i<4;i++){
          Serial.print(gain[i]);
          Serial.print(" ");
          gain[i] = 0;
        }
        Serial.println("ended");
        //Serial.println();
        
      }else{
        for(int i=0;i<4;i++){
          Serial.print(gain[i]);
          Serial.print(" ");
          gain[i] = 0;
        }
        Serial.println("abolished");
        //Serial.println(" ");

      }
      */
      gain0_ok = false;
      gain1_ok = false;
      gain2_ok = false;
      gain3_ok = false;
      count_all = 0;
      count_all_flag = false;
      gain0_predict = 0;
      gain1_predict = 0;
      gain2_predict = 0;
      gain3_predict = 0;
    }
  }

  piezo0_past = piezo0_present;
  piezo0_delta_past = piezo0_delta_present;
  piezo1_past = piezo1_present;
  piezo1_delta_past = piezo1_delta_present;
  piezo2_past = piezo2_present;
  piezo2_delta_past = piezo2_delta_present;
  piezo3_past = piezo3_present;
  piezo3_delta_past = piezo3_delta_present;
  //Serial.println();
}

int updateAudio(){

  if(mode==true){
    return (tomc.next()*volume) >> 8;
  }else if(mode==false){
    return (tome.next()*volume) >> 8;

  }

}

void loop(){
  audioHook();
}
