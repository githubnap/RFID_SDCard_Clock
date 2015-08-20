/* 
                 >>>>>> Connect with DW.mini.ESP8266-12 <<<<<
  -----------------------------------------------------------------------------------------
                    MFRC522        MicroSD-Card            Clock          Buzzer
  Signal             Pin        Adapter v1.1 (Catalex)      Pin            Pin     
  -----------------------------------------------------------------------------------------
  RST/Reset          0                 
  SPI SS(SDA)(CS)    15                2
  SPI MOSI           13                13
  SPI MISO           12   <<< R >>>    12 
  SPI SCK            14                14
  SPI SDA                                                    5
  SPI SCL                                                    4
  Active pin                                                                16
 
  * //ต้องต่อ R คร่อม ระหว่างขา MISO ของ RFID กับ MISO ของ SD-Card 
    เพราะว่า 2 Module ทำงานที่ไฟต่าง V กัน


   RFID  >>>  loop  >>>  Nap_Check_ID()  >>>  Read_SD_Card() //ถ้าหน้าว่างให้ Write_in_SD()  
 
   >>>    Redy_to_Write() //ถ้า = out ให้เว้นบรรทัด Write_in_SD()    >>>    Write_in_SD()  







 */

#include <SPI.h>
#include <MFRC522.h>
#include <SD.h>
#include <Wire.h>
#include "RTClib.h"

const int chipSelect = 2; 
File myFile;
RTC_DS3231 RTC;

#define Bib_pin         16          // Bib sound pin

#define RST_PIN        0           // Configurable, see typical pin layout above
#define SS_PIN         15          // Configurable, see typical pin layout above

MFRC522 mfrc522(SS_PIN, RST_PIN);   // Create MFRC522 instance.

// Number of known default keys (hard-coded)
// NOTE: Synchronize the NR_KNOWN_KEYS define with the defaultKeys[] array
#define NR_KNOWN_KEYS   8
// Known keys, see: https://code.google.com/p/mfcuk/wiki/MifareClassicDefaultKeys
byte knownKeys[NR_KNOWN_KEYS][MFRC522::MF_KEY_SIZE] =  {
    {0xff, 0xff, 0xff, 0xff, 0xff, 0xff}, // FF FF FF FF FF FF = factory default
    {0xa0, 0xa1, 0xa2, 0xa3, 0xa4, 0xa5}, // A0 A1 A2 A3 A4 A5
    {0xb0, 0xb1, 0xb2, 0xb3, 0xb4, 0xb5}, // B0 B1 B2 B3 B4 B5
    {0x4d, 0x3a, 0x99, 0xc3, 0x51, 0xdd}, // 4D 3A 99 C3 51 DD
    {0x1a, 0x98, 0x2c, 0x7e, 0x45, 0x9a}, // 1A 98 2C 7E 45 9A
    {0xd3, 0xf7, 0xd3, 0xf7, 0xd3, 0xf7}, // D3 F7 D3 F7 D3 F7
    {0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff}, // AA BB CC DD EE FF
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00}  // 00 00 00 00 00 00
};

/*
 * Initialize.
 */
 
 String ID_Card;
 String ID_Flie_Name;
 String Write_in_text;
 String IN_OUT;
 String ID_Flie_Name_Record_All;
 
 
void setup() {
    Serial.begin(9600);         // Initialize serial communications with the PC
    while (!Serial);            // Do nothing if no serial port is opened (added for Arduinos based on ATMEGA32U4)
   
    //Disable SD Card
    pinMode(chipSelect, OUTPUT);
    digitalWrite(chipSelect, HIGH);
    //Disable SD Card
    pinMode(SS_PIN, OUTPUT);
    digitalWrite(SS_PIN, HIGH);
    SPI.begin();                // Init SPI bus
    mfrc522.PCD_Init();         // Init MFRC522 card
    Serial.println(F("Try the most used default keys to print block 0 of a MIFARE PICC."));
    
    pinMode(Bib_pin,OUTPUT);
    digitalWrite(Bib_pin, LOW);
    
   
    
    //Initializing Clock
    Wire.begin();
    RTC.begin();
    //RTC.adjust(DateTime(__DATE__, __TIME__));
    if (! RTC.isrunning()) {
      Serial.println("RTC is NOT running!");
      BibBib();
      // following line sets the RTC to the date & time this sketch was compiled
      RTC.adjust(DateTime(__DATE__, __TIME__));
    }
    DateTime now = RTC.now();
    RTC.setAlarm1Simple(23, 9);
    RTC.turnOnAlarm(1);
    if (RTC.checkAlarmEnabled(1)) {
      Serial.println("Alarm Enabled");
    }
    
     //Initializing SD card
    // Serial.print("Initializing SD card...");
    //   if (!SD.begin(4)) {
    //     Serial.println("initialization failed!");
    //     BibBib();
    //     return;
    //   }
    // Serial.println("initialization done.");
    while (!Serial) {}
    Serial.print("Initializing SD card...");
    // see if the card is present and can be initialized:
    if (!SD.begin(chipSelect)) {
      Serial.println("Card failed, or not present");
      // don't do anything more:
      BibBib();
      return;
    }
    Serial.println("card initialized.");
    
    // Enable RFID
    digitalWrite(SS_PIN, LOW);  
    
}

/*
 * Helper routine to dump a byte array as hex values to Serial.
 */
void dump_byte_array(byte *buffer, byte bufferSize) {
    for (byte i = 0; i < bufferSize; i++) {
        Serial.print(buffer[i] < 0x10 ? " 0" : " ");
        Serial.print(buffer[i], HEX);
    }
}

/*
 * Try using the PICC (the tag/card) with the given key to access block 0.
 * On success, it will show the key details, and dump the block data on Serial.
 *
 * @return true when the given key worked, false otherwise.
 */
boolean try_key(MFRC522::MIFARE_Key *key)
{
    boolean result = false;
    byte buffer[18];
    byte block = 0;
    byte status;
    
    // Serial.println(F("Authenticating using key A..."));
    status = mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, block, key, &(mfrc522.uid));
    if (status != MFRC522::STATUS_OK) {
        // Serial.print(F("PCD_Authenticate() failed: "));
        // Serial.println(mfrc522.GetStatusCodeName(status));
        return false;
    }

    // Read block
    byte byteCount = sizeof(buffer);
    status = mfrc522.MIFARE_Read(block, buffer, &byteCount);
    if (status != MFRC522::STATUS_OK) {
        // Serial.print(F("MIFARE_Read() failed: "));
        // Serial.println(mfrc522.GetStatusCodeName(status));
    }
    else {
        // Successful read
        result = true;
        Serial.print(F("Success with key:"));
        dump_byte_array((*key).keyByte, MFRC522::MF_KEY_SIZE);
        Serial.println();
        // Dump block data
        Serial.print(F("Block ")); Serial.print(block); Serial.print(F(":"));
        dump_byte_array(buffer, 16);
        Serial.println();
    }
    Serial.println();

    mfrc522.PICC_HaltA();       // Halt PICC
    mfrc522.PCD_StopCrypto1();  // Stop encryption on PCD
    return result;
}

/*
 * Main loop.
 */
void loop() {
    // Look for new cards
    if ( ! mfrc522.PICC_IsNewCardPresent())
        return;

    // Select one of the cards
    if ( ! mfrc522.PICC_ReadCardSerial())
        return;
    Serial.println("vvvvvvvvvvvvvvvvvvvvvvvvvvvv");
    // Show some details of the PICC (that is: the tag/card)
    Serial.print(F("Card UID:"));
    dump_byte_array(mfrc522.uid.uidByte, mfrc522.uid.size);
    
    
     //Serial.print("-------------");
     for(int i=0; i<= mfrc522.uid.size; i++){
       //Serial.print(mfrc522.uid.uidByte[i]);
       ID_Card += (mfrc522.uid.uidByte[i]);
      
     }
     Serial.println();
     Serial.println(ID_Card);
    // Serial.print("-------------");
     
    
//    Serial.println();
//    Serial.print(F("PICC type: "));
      byte piccType = mfrc522.PICC_GetType(mfrc522.uid.sak);
//    Serial.println(mfrc522.PICC_GetTypeName(piccType));
    
    // Try the known default keys
    MFRC522::MIFARE_Key key;
    for (byte k = 0; k < NR_KNOWN_KEYS; k++) {
        // Copy the known key into the MIFARE_Key structure
        for (byte i = 0; i < MFRC522::MF_KEY_SIZE; i++) {
            key.keyByte[i] = knownKeys[k][i];
        }
        Serial.println("^^^^^^^^^^^^^^^^^^^^^^^^^");
        // Try the key
        if (try_key(&key)) {
          

          Nap_Check_ID(ID_Card); // check macth ID and Name
          Redy_to_Write();


          delay(100);
          
          // Disable RFID
          digitalWrite(SS_PIN, HIGH);
          //Enable SD Card
          digitalWrite(chipSelect, LOW);

          // Enable RFID
          digitalWrite(SS_PIN, LOW);
          //Disable SD Card
          digitalWrite(chipSelect, HIGH);
     
            break;
        }
    }
    ID_Card = ""; //clear ID
    IN_OUT = "";
    Write_in_text = "";
}



void Nap_Check_ID(String ID){
  DateTime now = RTC.now();

   switch (ID.toInt())
  {
        case 28312132290:
        {
          ID_Flie_Name = "Nap";
          Serial.println(">>>Sudtay_Onnuam<<<");
          break;
        }
        
        case 31224000:
        {
          ID_Flie_Name = "Name";
          Serial.println(">>>Sangtawan_Onnuam<<<");

          break;
        }
          
        case 154187812190:
        {
          ID_Flie_Name = "Nong";
          Serial.println(">>>Sonthaya_Onnuam<<<");

          break;
        }
          
        case 26238782190:
        {
          ID_Flie_Name = "Phang";
          Serial.println(">>>Sainamphang_Mejalern<<<");

          break;
        }

        default:
        {
          ID_Flie_Name = "None";
          Serial.println(">>>None<<<");
          BibBib();
          break;
        }
  }
  ID_Flie_Name_Record_All = ID_Flie_Name; //สร้างไฟล์อีกไฟล์ไว้บันทึกข้อมูลทั้งหมด
  ID_Flie_Name_Record_All += ".txt";

  if(now.month() < (uint8_t)10){
    ID_Flie_Name += "0";
  }
  ID_Flie_Name += now.month();
  ID_Flie_Name += ".txt";
  Read_SD_Card();
  
  
}


void Nap_Write(String ID ,String Text_data ){



  Write_in_SD(Text_data);
  
  
}

void Read_SD_Card(){
  myFile = SD.open(ID_Flie_Name);

  if (myFile) {
    Serial.print("Reading...");
    Serial.println(ID_Flie_Name);
    // read from the file until there's nothing else in it:

    while (myFile.available()) {
      //Serial.write(myFile.read());
      IN_OUT += (char)myFile.read(); //ดึงข้อมูลจาก SD Card
    }
    IN_OUT.trim(); //จัด formate ของประโยค เอาช่องว่างออกไป
    //Serial.println(IN_OUT);
    // Serial.println("----------------------");
    Serial.println();
    Serial.println("---------------------------------------");
    Serial.printf("length : "); Serial.println(IN_OUT.length());

    // Serial.println(IN_OUT.substring (IN_OUT.length()-30,IN_OUT.length()-28) ); //ตัดคำ เอาวันที่มา
    // Serial.println(IN_OUT.substring (IN_OUT.length()-18,IN_OUT.length()-16) );//ตัดคำ เอาชั่วโมงก่อนหน้ามา
    // Serial.println(IN_OUT.substring (IN_OUT.length()-15,IN_OUT.length()-13) );//ตัดคำ เอานาทีก่อนหน้าม
    // Serial.println(IN_OUT.substring (IN_OUT.length()-3) ); //ตัดคำ เอาคำหลังสุดมา In / Out
    // Serial.println("---------------------------------------");

    // close the file:
    myFile.close();
  } else {
    // if the file didn't open, print an error:
    Serial.print("Read_SD_Card()____error opening ");
    Serial.println(ID_Flie_Name);
    //BibBib();
  }

    if(IN_OUT.length() <=0){   //สร้างไฟล์ ถ้าตัวหน้าสือที่อ่านได้น้อยกว่า 0
      Nap_Write(ID_Card,ID_Flie_Name);
      Serial.println("make new flie"); 
    }
    if(IN_OUT.length() >10000){
      digitalWrite(Bib_pin,HIGH); //Bibbbb sound
      delay(1000);              
      digitalWrite(Bib_pin, LOW);
      delay(50);
    }

    Serial.printf("lengthAF : "); Serial.println(IN_OUT.length());

}


void Redy_to_Write(){
  DateTime now = RTC.now();

  Add_Zero(now.day());
  Write_in_text += (String)now.day();
  Write_in_text += "/";

  Add_Zero(now.month());
  Write_in_text += now.month();
  Write_in_text += "/";

  Write_in_text += now.year();
  Write_in_text += "  ";

  Add_Zero(now.hour());
  Write_in_text += now.hour();
  Write_in_text += ":";

  Add_Zero(now.minute());
  Write_in_text += now.minute();
  Write_in_text += "  ";

  Serial.println(Write_in_text);

  if(   IN_OUT.substring (IN_OUT.length()-3)   ==   "out"   ){
    if ((IN_OUT.substring (IN_OUT.length()-30,IN_OUT.length()-28)).toInt() != ((String)now.day()).toInt() ) //ถ้าวันไม่ตรงกัน ให้เว้นบรรทัด
    {
        Nap_Write(ID_Card,"");
    }
    Write_in_text += "         in";
  } else if(   IN_OUT.substring (IN_OUT.length()-2)   ==   "in"   ){
    Write_in_text += ">";
    Write_in_text += calculate_sum_time(
                        ((String)now.hour()).toInt() ,
                        ((String)now.minute()).toInt() ,
                        (IN_OUT.substring (IN_OUT.length()-18,IN_OUT.length()-14)).toInt() ,
                        (IN_OUT.substring (IN_OUT.length()-15,IN_OUT.length()-12)).toInt()
                      );
    Write_in_text += "  out";
    digitalWrite(Bib_pin,HIGH); //Bibbbb sound
    delay(400);              
    digitalWrite(Bib_pin, LOW);
  }else{
    Write_in_text += "         in";

  }
  Nap_Write(ID_Card,Write_in_text);
  Serial.println(Write_in_text);
}

void Add_Zero(uint8_t val_clock){  //ถ้าตัวเลขของ clock น้อยกว่า 2หลัก ให้เติม 0 ไปข้างหน้า เพื่อให้ตรง formate เรา
  if(val_clock < (uint8_t)10){
      Write_in_text += "0";
  }
}

String calculate_sum_time(int Hour , int Minute , int Time_old_hour , int Time_old_minute){
  DateTime now = RTC.now();

  String Cal_time="";
  int time_old;
  int time_new;
  int time_in_work;


  Serial.println("vvvvvvvvv Calculate vvvvvvvvvv");
  Serial.print("now.Hour:"); Serial.print(Hour);
  Serial.print(":"); Serial.println(Minute);

  Serial.print("Time_old_hour:"); Serial.print(Time_old_hour);
  Serial.print(":"); Serial.println(Time_old_minute);

    Serial.print(">>>"); Serial.print((IN_OUT.substring (IN_OUT.length()-18,IN_OUT.length()-14)).toInt()); Serial.println("<<<");
  Serial.print(">>>"); Serial.print((IN_OUT.substring (IN_OUT.length()-15,IN_OUT.length()-12)).toInt());Serial.println("<<<");

  Serial.println("^^^^^^^^^ Calculate ^^^^^^^^^");

  time_new = (Hour * 60) + (Minute);
  time_old = (Time_old_hour * 60) + (Time_old_minute);







  if (time_new == time_old)
  {
    return "00:00";
  }else if(time_new >= time_old){
    time_in_work = time_new - time_old;
    if ((IN_OUT.substring (IN_OUT.length()-30,IN_OUT.length()-28)).toInt() != ((String)now.day()).toInt() ) //ถ้าวันไม่ตรงกัน + 24 ชั่วโมง
    {
        time_in_work += 1440; 
    }
    Serial.print("____________________");
    Serial.println((IN_OUT.substring (IN_OUT.length()-30,IN_OUT.length()-28)).toInt());
    Serial.print("_____________2_______");
    Serial.println(((String)now.day()).toInt());
  }else{
    time_old = ((Time_old_hour - 24 )* 60) + (Time_old_minute);
    time_in_work = time_new - time_old;
  }



    


  Serial.print("time_new:"); Serial.println(time_new);
  Serial.print("time_old:"); Serial.println(time_old);


  if((time_in_work / 60) < 10){    Cal_time += "0";   }
  Cal_time += time_in_work / 60 ;

  Cal_time+= ":";

  if((time_in_work % 60) < 10){    Cal_time += "0";   }
  Cal_time += time_in_work % 60 ;

  Serial.print("time_work_minute >>>> ");
  Serial.println(time_in_work);
  Serial.print("Cal_time >>>> ");
  Serial.println(Cal_time);

  return Cal_time;

}



void Write_in_SD(String data_Write_in_text){

  myFile = SD.open(ID_Flie_Name, FILE_WRITE);
  
  // if the file opened okay, write to it:
  if (myFile) {
    Serial.print("Writing to ");Serial.print(ID_Flie_Name);Serial.print("...");
    myFile.println(data_Write_in_text);
    // close the file:
    myFile.close();
    Serial.println("done.");
    digitalWrite(Bib_pin,HIGH); //Bibbbb sound
    delay(100);              
    digitalWrite(Bib_pin, LOW);
  } else {
    // if the file didn't open, print an error:
    Serial.print("Write_in_SD()____error opening ");
    Serial.println(ID_Flie_Name);
    BibBib();
  }

    myFile = SD.open(ID_Flie_Name_Record_All, FILE_WRITE);
  
  // if the file opened okay, write to it:
  if (myFile) {
    myFile.println(data_Write_in_text);
    // close the file:
    myFile.close();
  } else {
    // if the file didn't open, print an error:
    Serial.print("Write_in_SD()Rec____error opening ");
    Serial.println(ID_Flie_Name_Record_All);
    BibBib();
  }




}






void BibBib(){
  for (int i = 0; i < 5; ++i)
  {
    digitalWrite(Bib_pin,HIGH); //Bibbbb sound
    delay(100);              
    digitalWrite(Bib_pin, LOW);
    delay(50);
  }
}
