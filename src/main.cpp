#include <Arduino.h>
#include <Wire.h>
#include <midi_msg.h>
#include <instruction_set.h>
#include <synth_control.h>
#include <midi_control.h>
#include <display_control.h>
#include <note_manager.h>
#include <wokwi.h>

// 共通変数
bool i2c_is_synth = true;
bool i2c_is_debug = false;
uint8_t synthMode = SYNTH_SINGLE;
bool isLed = false;
String synthCacheData = ""; // 次のSYNTH通信開始時に命令を送信するためのキャッシュ
uint8_t synthCacheId = 0x00; // 〃送信対象(0xffはブロードキャスト)
uint8_t response = 0x00; // レスポンス用
bool isPause = false; // シンセの制御が停止中か

// 各種制御クラス
NoteManager note;
DisplayControl* DisplayControl::instance = nullptr;
DisplayControl display(
    &i2c_is_synth, &i2c_is_debug, &synthMode,
    &synthCacheData, &synthCacheId, &response, &isPause
);
SynthControl synth(&i2c_is_synth, &synthCacheData, &synthCacheId, &display);
MIDIControl midi(&i2c_is_synth, &i2c_is_debug, &synthMode, &isLed, &note, &synth, &isPause);

TwoWire& disp = Wire;

void dispISR() {
    if(i2c_is_synth) {
        isLed = true;
        display.beginDisp();
    }else{
        isLed = false;
        synth.beginSynth();
    }
}

void setup() {
    // 初期化
    synth.beginSynth();
    midi.begin();

    // DebugPin
    Serial2.setTX(8);
    Serial2.setRX(9);
    Serial2.begin(115200);

    pinMode(LED_BUILTIN, OUTPUT);

    delay(10);

    // １音目にフェードアウトが適用されないため１音鳴らしておく
    uint8_t firstin[] = {INS_BEGIN, SYNTH_NOTE_ON, DATA_BEGIN, 0x02, 0x00, 0x00};
    uint8_t firstout[] = {INS_BEGIN, SYNTH_NOTE_OFF, DATA_BEGIN, 0x02, 0x00, 0x00};
    synth.synth1Write(firstin, sizeof(firstin));
    synth.synth2Write(firstin, sizeof(firstin));
    delay(10);
    synth.synth1Write(firstout, sizeof(firstout));
    synth.synth2Write(firstout, sizeof(firstout));

    pinMode(DISP_SW_PIN, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(DISP_SW_PIN), dispISR, FALLING);
}

void loop() {
    while(1){

        #if WOKWI_MODE == 1
            delay(10);
            if(isLed) {
                digitalWrite(LED_BUILTIN, HIGH);
            } else {
                digitalWrite(LED_BUILTIN, LOW);
            }
        #endif

        midi.read();
    }
}

#if WOKWI_MODE != 1
void loop1() {
    while (1) {
        if(isLed) {
            digitalWrite(LED_BUILTIN, HIGH);
        } else {
            digitalWrite(LED_BUILTIN, LOW);
        }
    }
}
#endif