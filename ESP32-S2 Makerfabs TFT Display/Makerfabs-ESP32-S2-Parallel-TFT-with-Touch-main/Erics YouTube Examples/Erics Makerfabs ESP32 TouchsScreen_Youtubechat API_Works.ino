 // Erics Notes ESP32 V2.01 Works!!!!  Newer does NOTTTTTTTTTTT
// http://youtube.com/mkmeorg
 // ArduinoJason Newest works 
 
 
 
 //From original Brian Code:
 /*******************************************************************
    Display messages and Super chats/stickers from a live stream
    on a given channel.

    This technically works on an ESP8266, but it does not have enough
    memory to handle all the messages, just use an ESP32 for this use
    case.

    Compatible Boards:
	  - Any ESP32 board

    Parts:
    ESP32 Mini Kit (ESP32 D1 Mini) * - https://s.click.aliexpress.com/e/_AYPehO (pick the CP2104 Drive version)

 *  * = Affiliate

    If you find what I do useful and would like to support me,
    please consider becoming a sponsor on Github
    https://github.com/sponsors/witnessmenow/


    Written by Brian Lough
    YouTube: https://www.youtube.com/brianlough
    Tindie: https://www.tindie.com/stores/brianlough/
    Twitter: https://twitter.com/witnessmenow
 *******************************************************************/
// ----------------------------
// Standard Libraries
// ----------------------------



#include <WiFi.h>
//#include <WiFiClient.h>
#include <WiFiClientSecure.h>

// ----------------------------
// Additional Libraries - each one of these will need to be installed.
// ----------------------------

#include <YouTubeLiveStream.h>
// Library for interacting with YouTube Livestreams

// Only available on Github
// https://github.com/witnessmenow/youtube-livestream-arduino

#include <YouTubeLiveStreamCert.h> // Comes with above, For HTTPS certs if you need them


#define ARDUINOJSON_DECODE_UNICODE 1 // Tell ArduinoJson to decide unicode, needs to be before the #include!

#include <ArduinoJson.h>
// Library used for parsing Json from the API responses

// Search for "Arduino Json" in the Arduino Library manager
// https://github.com/bblanchon/ArduinoJson

//------- Replace the following! ------

char ssid[] = "yourssid";         // your network SSID (name)
char password[] = "yourpw"; // your network password
int i = 0;
// You need 1 API key per roughly 2 hours of chat you plan to monitor 
// So you can pass in just one:

//#define YT_API_TOKEN "xxxxxxxxxxxxxxxxxxxxxxx"

// Or you can pass in an array of keys, 2 keys gives 4 hours, 3 == 6 etc (See Github readme for info)

#define NUM_API_KEYS 2
const char *keys[NUM_API_KEYS] = {"xxxxxxxxxxxxxxxxxxxx", "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx"};


//#define CHANNEL_ID "UCezJOfu7OtqGzd5xrP3q6WA" //Brian Lough
//#define CHANNEL_ID "UCSJ4gkVC6NrvII8umztf0Ow" //Lo-fi beats (basically always live)


#define CHANNEL_ID "UCfDqotmv53kzbfgqoDdL7NQ" //MKME Lab

//

//#define LED_PIN 45

//------- ---------------------- ------

WiFiClientSecure client;

#ifdef NUM_API_KEYS
YouTubeLiveStream ytVideo(client, keys, NUM_API_KEYS);
#else
YouTubeLiveStream ytVideo(client, YT_API_TOKEN);
#endif


unsigned long requestDueTime;               //time when request due
unsigned long delayBetweenRequests = 5000; // Time between requests (5 seconds)

LiveStreamDetails details;
char liveChatId[YOUTUBE_LIVE_CHAT_ID_CHAR_LENGTH];
char videoId[YOUTUBE_VIDEO_ID_LENGTH];
bool haveVideoId = false;
bool haveLiveChatId = false;
//bool ledState = false;

char lastMessageReceived[YOUTUBE_MSG_CHAR_LENGTH];


//----------------------

//FPS test
#define LGFX_USE_V1

#include <LovyanGFX.hpp>
#include <vector>

#define LCD_CS 37
#define LCD_BLK 45

class LGFX : public lgfx::LGFX_Device
{
  //lgfx::Panel_ILI9341 _panel_instance;
  lgfx::Panel_ILI9488 _panel_instance;
  lgfx::Bus_Parallel16 _bus_instance; // 8ビットパラレルバスのインスタンス (ESP32のみ)

public:
  // コンストラクタを作成し、ここで各種設定を行います。
  // クラス名を変更した場合はコンストラクタも同じ名前を指定してください。
  LGFX(void)
  {
    {                                    // バス制御の設定を行います。
      auto cfg = _bus_instance.config(); // バス設定用の構造体を取得します。

      // 16位设置
      cfg.i2s_port = I2S_NUM_0;  // 使用するI2Sポートを選択 (0 or 1) (ESP32のI2S LCDモードを使用します)
      cfg.freq_write = 16000000; // 送信クロック (最大20MHz, 80MHzを整数で割った値に丸められます)
      cfg.pin_wr = 35;           // WR を接続しているピン番号
      cfg.pin_rd = 34;           // RD を接続しているピン番号
      cfg.pin_rs = 36;           // RS(D/C)を接続しているピン番号

      cfg.pin_d0 = 33;
      cfg.pin_d1 = 21;
      cfg.pin_d2 = 14;
      cfg.pin_d3 = 13;
      cfg.pin_d4 = 12;
      cfg.pin_d5 = 11;
      cfg.pin_d6 = 10;
      cfg.pin_d7 = 9;
      cfg.pin_d8 = 3;
      cfg.pin_d9 = 8;
      cfg.pin_d10 = 16;
      cfg.pin_d11 = 15;
      cfg.pin_d12 = 7;
      cfg.pin_d13 = 6;
      cfg.pin_d14 = 5;
      cfg.pin_d15 = 4;

      _bus_instance.config(cfg);              // 設定値をバスに反映します。
      _panel_instance.setBus(&_bus_instance); // バスをパネルにセットします。
    }

    {                                      // 表示パネル制御の設定を行います。
      auto cfg = _panel_instance.config(); // 表示パネル設定用の構造体を取得します。

      cfg.pin_cs = -1;   // CS要拉低
      cfg.pin_rst = -1;  // RST和开发板RST相连
      cfg.pin_busy = -1; // BUSYが接続されているピン番号 (-1 = disable)

      // ※ 以下の設定値はパネル毎に一般的な初期値が設定されていますので、不明な項目はコメントアウトして試してみてください。

      cfg.memory_width = 320;   // ドライバICがサポートしている最大の幅
      cfg.memory_height = 480;  // ドライバICがサポートしている最大の高さ
      cfg.panel_width = 320;    // 実際に表示可能な幅
      cfg.panel_height = 480;   // 実際に表示可能な高さ
      cfg.offset_x = 0;         // パネルのX方向オフセット量
      cfg.offset_y = 0;         // パネルのY方向オフセット量
      cfg.offset_rotation = 3;  // 回転方向の値のオフセット 0~7 (4~7は上下反転)
      cfg.dummy_read_pixel = 8; // ピクセル読出し前のダミーリードのビット数
      cfg.dummy_read_bits = 1;  // ピクセル以外のデータ読出し前のダミーリードのビット数
      cfg.readable = true;      // データ読出しが可能な場合 trueに設定
      cfg.invert = false;       // パネルの明暗が反転してしまう場合 trueに設定
      cfg.rgb_order = false;    // パネルの赤と青が入れ替わってしまう場合 trueに設定
      cfg.dlen_16bit = true;    // データ長を16bit単位で送信するパネルの場合 trueに設定
      cfg.bus_shared = true;    // SDカードとバスを共有している場合 trueに設定(drawJpgFile等でバス制御を行います)

      _panel_instance.config(cfg);
    }

    setPanel(&_panel_instance); // 使用するパネルをセットします。
  }
};

LGFX lcd;

//--------------------------------------------------------------













void setup() {

 //------------------------------------------------- 
  pinMode(LCD_CS, OUTPUT);
  pinMode(LCD_BLK, OUTPUT);

  digitalWrite(LCD_CS, LOW);
  digitalWrite(LCD_BLK, HIGH);

  lcd.init();
  lcd.setCursor(0, 0);
  lcd.setTextSize(1);
//------------------------------------------------------------


  
  liveChatId[0] = '\0';
  videoId[0] = '\0';
  Serial.begin(115200);

 // pinMode(LED_PIN, OUTPUT);
  //digitalWrite(LED_PIN, ledState);

  // Set WiFi to 'station' mode and disconnect
  // from the AP if it was previously connected
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);

  // Connect to the WiFi network
  Serial.print("\nConnecting to WiFi: ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }
  Serial.println("\nWiFi connected!");
  Serial.print("IP address: ");
  IPAddress ip = WiFi.localIP();
  Serial.println(ip);


  // NOTE: See "usingHTTPSCerts" example for how to verify the server you are talking to.
  client.setInsecure();

}

void printMessage(ChatMessage message) {
  Serial.print(message.displayName);

  //lcd.printf(message.displayName); ///------------------------------------------
  if (message.isChatModerator) {
    Serial.print("(mod)");
    lcd.printf("(*MOD* ");
  }

  if (message.isChatSponsor) {
    Serial.print("(sponsor)");
    lcd.printf("(*SPONSOR* )");
  }

  Serial.print(": ");
  Serial.println(message.displayMessage);
  //lcd.println(message.displayMessage);
for(int i=0; i < 320; i++) {
    
    //lcd.println(message.displayMessage);
  }
  if (i < 290){
    i= i + 20;
    
    lcd.setCursor(0, i);
    lcd.setTextColor(0xFF0000U); 
    lcd.printf(message.displayName);
    lcd.printf("--> ");
    lcd.setTextColor(0xFFFFFF);
    lcd.println(message.displayMessage);}
  else {
    i = 0;
    lcd.init();
   lcd.setCursor(0, 0); 
   lcd.setTextColor(0xFF0000U); 
   lcd.printf(message.displayName);
   lcd.printf("--> ");
   lcd.setTextColor(0xFFFFFF);
   lcd.println(message.displayMessage);
  } 



  
  //lcd.setCursor(0, 0);
  
}

void printSuperThing(ChatMessage message) {
  Serial.print(message.displayName);
  if (message.isChatModerator) {
    Serial.print("(mod)");
  }
  Serial.print(": ");
  
  Serial.println(message.displayMessage);
  lcd.println(message.displayMessage);



  Serial.print(message.currency);
  Serial.print(" ");
  long cents = message.amountMicros / 10000;
  long centsOnly = cents % 100;

  Serial.print(cents / 100);
  Serial.print(".");
  if (centsOnly < 10) {
    Serial.print("0");
  }
  Serial.println(centsOnly);

  Serial.print("Tier: ");
  Serial.println(message.tier);
}

bool processMessage(ChatMessage chatMessage, int index, int numMessages) {

  if (index == 0) {
    Serial.print("Total Number of Messages: ");
    Serial.println(numMessages);
  }

  // Use the chat members details in this method
  // or if you want to store them make sure
  // you copy (using something like strcpy) them

  switch (chatMessage.type)
  {
    case yt_message_type_text:
      printMessage(chatMessage);


      //Possible to act on a message
     // if ( strcmp(chatMessage.displayMessage, "!led") == 0 )
    //  {
    //    ledState = !ledState;
    //    digitalWrite(LED_PIN, ledState);
   //   }

      // You can copy message details if you need them for something
      strncpy(lastMessageReceived, chatMessage.displayMessage, sizeof(lastMessageReceived)); //DO NOT use lastMessageReceived = chatMessage.displayMessage, it won't work as you expect!
      lastMessageReceived[sizeof(lastMessageReceived) - 1] = '\0';
      break;
    case yt_message_type_superChat:
    case yt_message_type_superSticker:
      printSuperThing(chatMessage);
      break;
    default:
      Serial.print("Unknown Message Type: ");
      Serial.println(chatMessage.type);
  }

  // return false from this method if you want to
  // stop parsing more messages.
  return true;
}


// This gets the video ID of a live stream on a given channel
void getVideoId() {
  // This is the official way to get the videoID, but it
  // uses too much of your daily quota.
  //haveVideoId = ytVideo.getLiveVideoId(CHANNEL_ID, videoId, YOUTUBE_VIDEO_ID_LENGTH);

  haveVideoId = ytVideo.scrapeIsChannelLive(CHANNEL_ID, videoId, YOUTUBE_VIDEO_ID_LENGTH);
  if (haveVideoId) {
    Serial.println("Channel is live");
    Serial.print("Video ID: ");
    Serial.println(videoId);
  } else {
    Serial.println("Channel does not seem to be live");
  }
}

// This gets the Live Chat ID of a live stream on a given Video ID.
// This is needed to get chat messages.
void getLiveChatId() {
  haveLiveChatId = false;

  details = ytVideo.getLiveStreamDetails(videoId);
  if (!details.error) {
    if (details.isLive) {
      Serial.print("Chat Id: ");
      Serial.println(details.activeLiveChatId);
      strncpy(liveChatId, details.activeLiveChatId, sizeof(liveChatId));
      liveChatId[sizeof(liveChatId) - 1] = '\0';
      haveLiveChatId = true;
    } else {
      Serial.println("Video does not seem to be live");
      haveVideoId = false;
    }
  } else {
    Serial.println("Error getting Live Stream Details");
  }
}

void loop() {

    //int fps = 0;
    //lcd.setCursor(0, 0);
    //lcd.setTextSize(4);
   // lcd.printf("fps:%03d", fps);
    //lcd.printf("TESSSSSSSSSSSSSSSSST");
     // lcd.init();
 // lcd.setCursor(0, 0);
    


    
  if (millis() > requestDueTime)
  {
    if (!haveVideoId) {
      //Don't have a video ID, so we'll try get one.
      getVideoId();
    }

    if (haveVideoId && !haveLiveChatId) {
      // We have a video ID, but not a liveChatId
      getLiveChatId();
    }

    if (haveLiveChatId) {
      // The processMessage Callback will be called for every message found.
      ChatResponses responses = ytVideo.getChatMessages(processMessage, liveChatId);
      if (!responses.error) {
        Serial.println("done");
        Serial.print("Polling interval: ");
        Serial.println(responses.pollingIntervalMillis);


        requestDueTime = millis() + responses.pollingIntervalMillis + 500;
      } else if (!responses.isStillLive) {
        //Stream is not live any more.
        haveLiveChatId = false;
        haveVideoId = false;
        requestDueTime = millis() + delayBetweenRequests;
      } else {
        Serial.println("There was an error getting Messages");
        requestDueTime = millis() + delayBetweenRequests;
      }
    } else {
      requestDueTime = millis() + delayBetweenRequests;
    }
  }
}
