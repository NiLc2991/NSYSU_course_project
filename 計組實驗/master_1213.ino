#include <SoftwareSerial.h>   // 引用程式庫

// #include <LiquidCrystal_I2C.h>
#include <Wire.h>
#define STEER_LEFT A1
#define SPEEDUP A2
#define SLOWDOWN A3
#define DIR_BUTT 13
// 定義連接藍牙模組的序列埠
SoftwareSerial BT(8, 10); // 接收腳, 傳送腳
char val;
String command;  // 儲存接收資料的變數

void setup() {
  Serial.begin(9600);   // 與電腦序列埠連線
  BT.begin(38400);
  Serial.println("BT is ready!");
  pinMode(STEER_LEFT, INPUT);
  pinMode(SPEEDUP, INPUT);
  pinMode(SLOWDOWN, INPUT);
  pinMode(DIR_BUTT, INPUT);

  // 藍牙透傳模式的預設連線速率。
  
}
bool state = 1;
void loop() {
  
  // 若收到「序列埠監控視窗」的資料，則送到藍牙模組
  int speedup = map(analogRead(SPEEDUP), 0, 1023, 0, 255);
  int slowdown = map( analogRead(SLOWDOWN), 0, 1023, 0, 255);
  int diff = map(analogRead(STEER_LEFT), 0, 1023, 0, 255);
  Serial.print("speedup");
  Serial.println(speedup);
  Serial.print("slowdown");
  Serial.println(slowdown);
  int speed;
  if(speedup <= 85)
    speed = 100;
  else if(speedup == 86)
    speed = 175;
  // else if(speedup == 87)
  //   speed = 250;
  else
    speed = 250;
  
  if(slowdown>81)
    speed = 0;

  if (DIR_BUTT == 0){
    if (state = 0 ) state = 1;
    else state = 0;
  }
  String command = String(diff) + " " + String(speed) + " " + String(1)+'\n'; 
  // Serial.println(command);

  // for(int i=0;i<command.length();i++)
  //   BT.print(command[i]);
    // BT.println(command);


  for(int i=0;i<command.length();i++)
    BT.print(command[i]);
  // Serial.print(command);
  // String test = String(slowdown) + " ";
  // Serial.print(test);
  // String test2 = String(speedup) + " ";
  // Serial.print(test2);
  
  delay(200);
  

}