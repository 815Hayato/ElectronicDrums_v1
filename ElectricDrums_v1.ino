/*
＜変更点＞
・トリガリングのアルゴリズムを変更
＜今後の変更点(予定)＞
・入力に複数のピエゾを用いる⇒打点推定モデルの実装
・A0に用いるセンサをスライドボリュームに変更
・レイテンシ対策
*/

#include <MozziGuts.h>
#include <Sample.h>
#include <samples/tom.h>
Sample <tom_NUM_CELLS, AUDIO_RATE> tom(tom_DATA); //タムの音声データ

#define CONTROL_RATE 64

//入力に関わる変数
int pitch; //可変抵抗→A0,タムのピッチを定めるために使う

int piezo1_present; //圧電素子→A2,タムのトリガリング・音量・位置推定に使う
int piezo1_past;
int piezo1_delta_present;
int piezo1_delta_past;


//制御に関わる変数
int threshold = 100; //threshold以上の信号がきたらアクション(トリガー判定)を起こす
int delta_trigger = 15; //前の信号の値との差がdelta_triggerであればアクションを起こす
int count; //トリガー判定用
bool count_flag; //トリガー判定中かどうか
int max_predict; //推定ゲイン
int gain; //確定ゲイン

//出力に関わる変数
byte volume = 0; //出力のゲイン(0-255)
const float BasicFrequency = (float) tom_SAMPLERATE / (float) tom_NUM_CELLS;  // 音源を再生するサンプリング周波数の係数
float freq; //BasicFrequencyの係数(0.5-2)


//初期設定
void setup() {
  //Serial.begin(9600);
  startMozzi(CONTROL_RATE);
  count = 0;
  piezo1_past = 0;
  count_flag = false;
}


//繰り返し処理
void updateControl() {
  
  pitch = analogRead(A0);
  freq = BasicFrequency * pow(2, pitch/512.000) / 2.00;
  tom.setFreq(freq);
  piezo1_present = analogRead(A2);
  //Serial.println(piezo1_present);

  piezo1_delta_present = piezo1_present - piezo1_past;
  if(count_flag == false){ //トリガー判定中でないならば⇒トリガー判定を始めるべきか判断する
    if(piezo1_past > threshold){
      if(piezo1_delta_present < -delta_trigger && piezo1_delta_past > delta_trigger){
        count = 1;
        count_flag = true;
        max_predict = piezo1_past;
        //Serial.print(count);
        //Serial.print(max_predict);
      }
    }
  }else if(count_flag == true){ //トリガー判定中なら⇒推定ゲインの更新orゲインの確定
    if(max_predict > piezo1_present){
      count += 1;

      if(count == 10){ //推定ゲインの確定⇒発音
        count = 0;
        count_flag = false;
        gain = max_predict;
        //Serial.println(gain);
        volume = map(gain,0,1023,0,255);
        tom.start(); //発音
      }
    }else{ //推定ゲインの更新
      count = 0;
      max_predict = piezo1_present;
    }
    
  }

  piezo1_past = piezo1_present;
  piezo1_delta_past = piezo1_delta_present;
}

//出力処理
AudioOutput updateAudio(){
  return (tom.next()*volume) >> 8;
}

void loop(){
  audioHook();
}