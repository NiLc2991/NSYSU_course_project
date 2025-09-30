#include <SoftwareSerial.h>   // 引用程式庫
#define IN1 5 
#define IN2 4
#define IN3 3
#define IN4 2
#define ENB 10
#define ENA 11
#define LED_PIN 7

// 定義連接藍牙模組的序列埠
SoftwareSerial BT(8, 9); // 接收腳, 傳送腳
// String command;  // 儲存接收資料的變數
int val;
void setup() {
  // 設定HC-05藍牙模組，AT命令模式的連線速率。
  BT.begin(38400);
  Serial.begin(9600);
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);
  pinMode(ENA, OUTPUT);
  pinMode(ENB, OUTPUT);
  pinMode(LED_PIN, OUTPUT);
}
// 儲存上一次合法的速度與差值
int last_diff = 0;
int last_speed = 0;
bool last_initialized = false;

void processString(const String& input) {
    // Serial.print(input);
    // 分解字串
    int firstSpace = input.indexOf(' ');
    int secondSpace = input.indexOf(' ', firstSpace + 1);
    int thirdSpace = input.indexOf(' ', secondSpace + 1);

    // 檢查是否超過三個數字
    if (thirdSpace != -1) {
        // 無效輸入：包含超過三個數字
        return;
    }

    if (firstSpace == -1 || secondSpace == -1) {
        // 無效輸入：少於三個數字
        return;
    }

    // 提取數值部分
    String num1Str = input.substring(0, firstSpace);
    String num2Str = input.substring(firstSpace + 1, secondSpace);
    String booleanStr = input.substring(secondSpace + 1);

    // 將字串轉換為整數
    int diff = num1Str.toInt();
    int speed = num2Str.toInt();
    int booleanInt = booleanStr.toInt();

    // 檢查數值範圍
    if (diff < 0 || diff > 255 || speed < 0 || speed > 255 || (booleanInt != 0 && booleanInt != 1)) {
        // 無效輸入：數值超過範圍
        return;
    }

    // 檢查和上一次的合法數值差異

    // 更新上一次的合法數值
    last_diff = diff;
    last_speed = speed;
    last_initialized = true;

    if(diff >= 81 && diff <= 82)
      diff = 0;
    else if(diff<=80)
      diff = -250;
    else
      diff = 250;
    // 計算速度
    bool dir = (booleanInt != 0);
    int right_speed;
    int left_speed;
    if(diff >0 )
    {
      left_speed = speed/2;
      right_speed = speed;
      if (left_speed<0)
        left_speed = 0;
    }
    else if(diff == 0)
    {
      left_speed = speed;
      right_speed = speed;
    }
    else
    {
      right_speed = speed/2;
      left_speed = speed;
      if (right_speed<0)
        right_speed = 0;
      
    }
    Serial.print(right_speed);
    Serial.print(" ");
    Serial.print(left_speed);
    Serial.println(" ");
      
    


    // 控制馬達方向與速度
    if (dir) {
        // 前進
        digitalWrite(IN1, HIGH);
        digitalWrite(IN2, LOW);
        digitalWrite(IN3, HIGH);
        digitalWrite(IN4, LOW);
        analogWrite(ENA, constrain(left_speed, 0, 255));
        analogWrite(ENB, constrain(right_speed, 0, 255));
        digitalWrite(LED_PIN, LOW);
    } else {
        // 後退
        digitalWrite(IN1, LOW);
        digitalWrite(IN2, HIGH);
        digitalWrite(IN3, LOW);
        digitalWrite(IN4, HIGH);
        analogWrite(ENA, constrain(left_speed, 0, 255));
        analogWrite(ENB, constrain(right_speed, 0, 255));
        digitalWrite(LED_PIN, HIGH);
    }
}


String readStringFromSoftwareSerial() {
  String result = ""; // 儲存接收的字串
  unsigned long startTime = millis(); // 紀錄開始時間
  Serial.print("result:");
  while (true) {
    if (BT.available()) {
      char receivedChar = BT.read(); // 讀取一個字符
      if(receivedChar != '\0' && receivedChar != '\n' && receivedChar != ' ' && (receivedChar<'0' || receivedChar>'9'))
        continue;
      Serial.print(receivedChar);
      Serial.print(" ");
      if (receivedChar == '\n') {          // 如果遇到換行符號
        Serial.println('\n');
        break;                             // 結束讀取
      }
      // result.concat(receivedChar);
      result += receivedChar;              // 拼接到結果字串
    }

    // 超時控制（例如 1000 毫秒）
    if (millis() - startTime > 1000) {
      break; // 如果超過 1 秒，退出迴圈
    }
  }

  return result; // 返回完整的字串
}

void loop() {
  // Serial.println("test");
  // 若收到藍牙模組的資料，則送到「序列埠監控視窗」
  // String test1 = "0 250 1";
  // String test2 = "0 250 1";
  // String test3 = "0 250 0";
  // String test4 = "0 250 0";
  // processString(test1);
  // delay(2000);
  // processString(test2);
  // delay(2000);
  // processString(test3);
  // delay(2000);
  // processString(test4);
  // delay(2000);


  String command;
  if (BT.available()) {
    Serial.println("run");
    command = readStringFromSoftwareSerial();
    Serial.println(command);
  }
  processString(command);
}