#include <MPU9250_asukiaaa.h>
#include "BluetoothSerial.h"

#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include <Adafruit_TFTShield18.h>

Adafruit_ST7735 tft = Adafruit_ST7735(16, 17, 23, 5, 9); // CS,A0,SDA,SCK,RESET

//9軸I2C引腳
#define SDA_PIN 21
#define SCL_PIN 22
MPU9250_asukiaaa mySensor(MPU9250_ADDRESS_AD0_HIGH);//9軸library

BluetoothSerial SerialBT;//藍牙Library
boolean confirmRequestPending = true;//控制藍牙配對的值

#define BUTTON_PIN_BITMASK 0x200000000 // 2^33 in hex
RTC_DATA_ATTR int RTCBootCount = 0; //使用RTC記憶體紀錄喚醒次數

String ComeValue;     //傳來的藍牙數據
String GetValue;      //傳去的藍芽數據
String SPPName="Dof9Sys0";//藍牙名稱+自訂義廠編

float aX, aY, aZ, aSqrt, gX, gY, gZ, mDirection, mX, mY, mZ;//感測值

const int buttonA = 39; 
const int buttonB = 37; 
const int buttonC = 36; 
int ButtonAState = 0;
int ButtonBState = 0;
int ButtonCState = 0;
uint32_t numValTemp;

//喚醒原因輸出
void print_wakeup_reason(){
  esp_sleep_wakeup_cause_t wakeup_reason;
  wakeup_reason = esp_sleep_get_wakeup_cause();
  switch(wakeup_reason)
  {
  
    case ESP_SLEEP_WAKEUP_EXT0 : Serial.println("Wakeup caused by external signal using RTC_IO"); break;
    case ESP_SLEEP_WAKEUP_EXT1 : Serial.println("Wakeup caused by external signal using RTC_CNTL"); break;
    case ESP_SLEEP_WAKEUP_TIMER : Serial.println("Wakeup caused by timer"); break;
    case ESP_SLEEP_WAKEUP_TOUCHPAD : Serial.println("Wakeup caused by touchpad"); break;
    case ESP_SLEEP_WAKEUP_ULP : Serial.println("Wakeup caused by ULP program"); break;
    default : Serial.printf("Wakeup was not caused by deep sleep: %d\n",wakeup_reason); break;
  }
}

void BTConfirmRequestCallback(uint32_t numVal)
{
    confirmRequestPending = true;
    //Serial.println(numVal);
    numValTemp = numVal;    
}

void BTAuthCompleteCallback(boolean success)
{
  confirmRequestPending = false;
  if (success)
  {
    Serial.println("Pairing success!!");
  }
  else
  {
    Serial.println("Pairing failed, rejected by user!!");
  }
}

void tftClearScreen() {//完全清除畫面副程式
  tft.setRotation(0);
  tft.fillScreen(ST7735_BLACK);
  tft.setRotation(1);
  tft.fillScreen(ST7735_BLACK);
  tft.setRotation(2);
  tft.fillScreen(ST7735_BLACK);
  tft.setRotation(3);
  tft.fillScreen(ST7735_BLACK);
}

void tftPrintTitle() {
  tftClearScreen();//完全清除畫面,消除邊界線用的,若未使用會殘留邊界不明像素
  tft.setRotation(0);
  tft.setTextSize(2);
  tft.setCursor(10,50);
  tft.setTextColor(ST7735_YELLOW);
  tft.println("Welcome~!");
  delay(2000);
  tft.fillScreen(ST7735_BLACK);
  tft.setTextSize(2);
  tft.setTextColor(ST7735_YELLOW);
  tft.setCursor(17,8);
  tft.println(SPPName);
  delay(2000);
}

void setup() {
  Serial.begin(9600);
  //print_wakeup_reason();//印出喚醒原因
  esp_sleep_enable_ext0_wakeup(GPIO_NUM_36,0); //設置喚醒GPIO36(按鈕C) 0=low ; 1=high
    
  pinMode(27,OUTPUT);//背光控制腳位
  digitalWrite(27,HIGH);//拉高背光腳位;若進省電模式時要拉低。
  tft.initR(INITR_BLACKTAB);//初始化ST7735S chip
  tftPrintTitle();//初始資訊
  
  pinMode(buttonA, INPUT);  //A鍵作為關機鍵
  //pinMode(buttonB, INPUT);//暫時不用按鍵B
  pinMode(buttonC, INPUT);  //C鍵作為藍芽配對確認鍵+關機時喚醒鍵
  
  SerialBT.enableSSP();
  SerialBT.onConfirmRequest(BTConfirmRequestCallback);
  SerialBT.onAuthComplete(BTAuthCompleteCallback);
  SerialBT.begin(SPPName);//設定藍芽名稱
  
  Wire.begin(SDA_PIN, SCL_PIN);//初始化MPU9250 Library
  mySensor.setWire(&Wire);
  mySensor.beginAccel();
  mySensor.beginGyro();
  mySensor.beginMag();
  
}

void loop() {
  //▼讀取9軸訊號▼
  uint8_t sensorId;//MPU9250 感測器暫存位址
  int result;//MPU9250 感測器暫存器資料
  result = mySensor.readId(&sensorId);
  if (result == 0) {
    //Serial.println("sensorId: " + String(sensorId));
  } 
  else 
  {
    Serial.println("Cannot read sensorId " + String(result));
  }
  result = mySensor.accelUpdate();
  if (mySensor.accelUpdate() == 0) {
    aX = mySensor.accelX();
    aY = mySensor.accelY();
    aZ = mySensor.accelZ();
    aSqrt = mySensor.accelSqrt();
  } 
  else 
  {
    //Serial.println("Cannod read accel values");
  }
  result = mySensor.gyroUpdate();
  if (mySensor.gyroUpdate() == 0) {
    gX = mySensor.gyroX();
    gY = mySensor.gyroY();
    gZ = mySensor.gyroZ();
  } 
  else 
  {
    //Serial.println("Cannot read gyro values");
  }
  result = mySensor.magUpdate();
  if (mySensor.magUpdate() == 0) {
    mX = mySensor.magX();
    mY = mySensor.magY();
    mZ = mySensor.magZ();
    mDirection = mySensor.magHorizDirection();
  } 
  else 
  {
    //Serial.println("Cannot read mag values");
  }
  //▲讀取9軸訊號▲

  //▼藍芽通訊▼
  if (confirmRequestPending)//配對機制--當配對時會顯示暗碼
  {
    if (ButtonCState>=2)
    {
      SerialBT.confirmReply(true);//確認暗碼沒問題後點按IO37號的按鍵執行建立連線。
      tft.fillScreen(ST7735_BLACK);
      tft.setTextColor(ST7735_YELLOW);
      tft.setCursor(25,8);
      tft.println("9Ax-Sys");
      tft.setCursor(0,25);
    }
    else
    {
      tft.setTextColor(ST7735_RED);
      tft.setCursor(0,25);
      tft.println(">Wait link");  
      if(numValTemp!=0)
      {
        tft.println(">PN:"+(String)numValTemp);
        tft.println(">if ok,\n You can\n Push C\n Button...");
      }
    }
  }
   if (SerialBT.available())//確認是否有傳送資料給ESP32
   {
       ComeValue = SerialBT.readString();
       //Serial.print("ComeValue=");//COM Port測試
       //Serial.print(ComeValue);
       //Serial.println();
       tft.setTextColor(ST7735_BLUE);
       tft.println("->"+ComeValue);
       if(ComeValue=="LinkOk")
       {
          confirmRequestPending = false;
       }
   }
   else//若沒有就持續傳遞9軸資訊過去
   {
      GetValue = String(aX)+String(",")+String(aY)+String(",")+String(aZ)+String(",")+String(gX)+String(",")+String(gY)+String(",")+String(gZ)+","+String(mDirection)+String(millis())+ "ms";
      //輸出9軸值
      SerialBT.println(GetValue);
      //Serial.println(GetValue);//COM Port測試
   }
   //▲藍芽通訊▲

   //▼按鍵事件檢測▼
   if (confirmRequestPending)//配對狀態下才可使用
   {
     if (digitalRead(buttonC) == LOW) 
      {
        ButtonCState = ButtonCState+1;
        Serial.println("ButtonCState= " + (String)ButtonCState);
      }
      else
      {
        ButtonCState=0;
      }
   }
   if (digitalRead(buttonA) == LOW) 
   {
      ButtonAState = ButtonAState+1;
      Serial.println("ButtonAState= " + (String)ButtonAState);
      if(ButtonAState>=100)//長按計數至100後休眠
      {
        esp_deep_sleep_start();//進入休眠
      }
   }
   else
   {
      ButtonAState=0;
   }
   //▲按鍵檢測▲
}
