#include <WiFi.h>
#include <WiFiUdp.h>
#include <math.h>


#include "secrets.h"
"""
#ifndef SECRETS_H
#define SECRETS_H

const char* networkName = "";
const char* networkPswd = "";

#endif
"""

// UDPデータを送信するためのIPアドレス：
// サーバーのIPアドレスを使用するか、
// ネットワークブロードキャストアドレスを使用します
const char* udpAddress = "192.168.8.117";
const int udpPort = 5001;

// 接続されているかどうか？
boolean connected = false;

// UDPライブラリクラス
WiFiUDP udp;

// 波形を生成するための変数
const float amplitude = 1.0; // Vpp (ピーク間電圧) は1Vに設定
const int samples = 100;     // 1つの波形あたりのサンプル数

// オフセット電圧（1.5V）
const float offsetVoltage = 1.5;

// 波形のルックアップテーブル
uint16_t sineWave[samples];
uint16_t triangleWave[samples];
uint16_t squareWave[samples];
uint16_t sawtoothWave[samples];

// 波形の選択
int selectedWaveform;          // 0: 正弦波, 1: 三角波, 2: 矩形波, 3: ノコギリ波
int selectedFrequencyIndex = 0; // デフォルトの周波数インデックスは0 (500Hz)

// 選択可能な周波数リスト
const float frequencies[] = {500.0, 1000.0, 5000.0};
const int numFrequencies = sizeof(frequencies) / sizeof(frequencies[0]);

// 分解能（ビット）
const int resolution = 10;

void setup()
{
  // ハードウェアシリアルを初期化：
  Serial.begin(115200);

  // WiFiネットワークに接続
  connectToWiFi(networkName, networkPswd);

  // 波形のルックアップテーブルを生成
  for (int i = 0; i < samples; i++)
  {
    float radians = (2.0 * PI * frequencies[selectedFrequencyIndex] * i) / samples;
    sineWave[i] = (uint16_t)((amplitude * ((sin(radians) + 1.0) / 2.0) + offsetVoltage) * pow(2, resolution));
    triangleWave[i] = (uint16_t)((amplitude * abs(2 * fmod(i * frequencies[selectedFrequencyIndex] / samples + 0.5, 1.0) - 0.5) + offsetVoltage) * pow(2, resolution));
    squareWave[i] = (i < samples / 2) ? (uint16_t)((amplitude + offsetVoltage) * pow(2, resolution)) : (uint16_t)((-amplitude + offsetVoltage) * pow(2, resolution));
    sawtoothWave[i] = (uint16_t)((amplitude * (i / (float)samples) + offsetVoltage) * pow(2, resolution));
  }
}

void loop()
{
  // シリアル入力をチェックして周波数または波形を選択
  if (Serial.available() > 0)
  {
    String inputString = Serial.readStringUntil('\n');
    inputString.trim();
    int selection = inputString.toInt();

    if (selection >= 0 && selection < numFrequencies)
    {
      // 選択された周波数インデックスを更新
      selectedFrequencyIndex = selection;
      Serial.print("選択された周波数: ");
      Serial.println(frequencies[selectedFrequencyIndex]);
    }
    else if (selection >= 100 && selection <= 103)
    {
      // 選択された波形を更新
      selectedWaveform = selection - 100;
      switch (selectedWaveform)
      {
      case 0:
        Serial.println("選択された波形: 正弦波");
        break;
      case 1:
        Serial.println("選択された波形: 三角波");
        break;
      case 2:
        Serial.println("選択された波形: 矩形波");
        break;
      case 3:
        Serial.println("選択された波形: ノコギリ波");
        break;
      default:
        Serial.println("無効な波形選択。有効な選択肢は0から3です。");
        break;
      }
    }
    else
    {
      Serial.println("無効な選択。有効な周波数選択は0から" + String(numFrequencies - 1) + "、波形選択は100から103です。");
    }
  }

  // 通信が接続されている場合のみデータを送信
  if (connected)
  {
    // 選択された波形を選択
    uint16_t* waveform = NULL;
    switch (selectedWaveform)
    {
    case 0:
      waveform = sineWave;
      break;
    case 1:
      waveform = triangleWave;
      break;
    case 2:
      waveform = squareWave;
      break;
    case 3:
      waveform = sawtoothWave;
      break;
    default:
      waveform = sineWave; // デフォルトは正弦波
      break;
    }

    // 選択された波形をUDPパケットとして送信
    udp.beginPacket(udpAddress, udpPort);
    udp.write((uint8_t*)waveform, sizeof(uint16_t) * samples);
    udp.endPacket();
  }

  // 次の更新まで短時間待機
  delay(10);
}

void connectToWiFi(const char* ssid, const char* pwd)
{
  Serial.println("WiFiネットワークに接続中: " + String(ssid));

  // 古い設定を削除
  WiFi.disconnect(true);
  // イベントハンドラを登録
  WiFi.onEvent(WiFiEvent);

  // 接続を開始
  WiFi.begin(ssid, pwd);

  Serial.println("WiFi接続を待っています...");
}

// WiFiイベントハンドラ
void WiFiEvent(WiFiEvent_t event)
{
  switch (event)
  {
  case ARDUINO_EVENT_WIFI_STA_GOT_IP:
    // 接続されたら
    Serial.print("WiFi接続成功. IPアドレス: ");
    Serial.println(WiFi.localIP());
    udp.begin(WiFi.localIP(), udpPort);
    connected = true;
    break;
  case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
    Serial.println("WiFi接続が切断されました");
    connected = false;
    break;
  default:
    break;
  }
}
