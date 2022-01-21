#define LGFX_USE_V1
#include <LovyanGFX.hpp>

#include <SPI.h>
#include <mySD.h>
#include <WiFiClient.h>
#include <ESP32WebServer.h>
#include <WiFi.h>
#include <ESPmDNS.h>

//SPI
#define SPI_MOSI 2 //1
#define SPI_MISO 41
#define SPI_SCK 42
#define SD_CS 1 //2

const char *ssid = "Makerfabs";
const char *password = "20160704";

#define PIC_NAME "/display.bmp"

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
        {                                      // バス制御の設定を行います。
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

        {                                        // 表示パネル制御の設定を行います。
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
            cfg.offset_rotation = 0;  // 回転方向の値のオフセット 0~7 (4~7は上下反転)
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

LGFX tft;
ESP32WebServer server(80);
File root;
bool opened = false;

void setup(void)
{

    pinMode(LCD_CS, OUTPUT);
    pinMode(LCD_BLK, OUTPUT);

    digitalWrite(LCD_CS, LOW);
    digitalWrite(LCD_BLK, HIGH);
    Serial.begin(115200);

    tft.init();
    tft.setRotation(3);

    tft.fillScreen(TFT_BLUE);

    tft.setCursor(10, 10);
    tft.setTextColor(TFT_WHITE);
    tft.setTextSize(4);
    tft.println("WIFI FILESYSTEM");

    //SPI init
    SPI.begin(SPI_SCK, SPI_MISO, SPI_MOSI);

    if (!SD.begin(SD_CS))
    {
        Serial.println("initialization failed!");
        return;
    }

    tft.setCursor(10, 40);
    tft.setTextColor(TFT_WHITE);
    tft.setTextSize(4);
    tft.println("SD INIT OVER");

    webserve_init();

    tft.println(WiFi.localIP());
    //print_img("/logo.bmp");
}

void loop(void)
{
    server.handleClient();
}

//Init

void webserve_init()
{
    WiFi.begin(ssid, password);
    Serial.println("");

    // Wait for connection
    while (WiFi.status() != WL_CONNECTED)
    {
        delay(500);
        Serial.print(".");
    }
    Serial.println("");
    Serial.print("Connected to ");
    Serial.println(ssid);
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());

    //use IP or iotsharing.local to access webserver
    if (MDNS.begin("iotsharing"))
    {
        Serial.println("MDNS responder started");
    }

    Serial.println("initialization done.");
    //handle uri
    server.on("/", handleRoot);
    server.onNotFound(handleNotFound);

    /*handling uploading file */
    server.on(
        "/update", HTTP_POST, []()
        { server.sendHeader("Connection", "close"); },
        []()
        {
            HTTPUpload &upload = server.upload();

            String upload_file_name = String("/") + upload.filename;
            if (opened == false)
            {
                opened = true;

                //Fresh File
                SD.remove((char *)upload_file_name.c_str());
                root = SD.open(upload_file_name.c_str(), FILE_WRITE);
                if (!root)
                {
                    Serial.println("- failed to open file for writing");
                    return;
                }
            }
            if (upload.status == UPLOAD_FILE_WRITE)
            {
                if (root.write(upload.buf, upload.currentSize) != upload.currentSize)
                {
                    Serial.println("- failed to write");
                    return;
                }
            }
            else if (upload.status == UPLOAD_FILE_END)
            {
                root.close();
                Serial.println("");
                Serial.println("UPLOAD_FILE_END");
                opened = false;

                //Display Upload File
                if (upload_file_name.endsWith(".bmp"))
                    print_img(upload_file_name);
                else if (upload_file_name.endsWith(".txt"))
                    print_txt(upload_file_name);
            }
        });
    server.begin();
    Serial.println("HTTP server started");
}

//Web Server

String serverIndex = "<script src='https://ajax.googleapis.com/ajax/libs/jquery/3.2.1/jquery.min.js'></script>"
                     "<form method='POST' action='#' enctype='multipart/form-data' id='upload_form'>"
                     "<input type='file' name='update'>"
                     "<input type='submit' value='Upload'>"
                     "</form>"
                     "<div id='prg'>progress: 0%</div>"
                     "<script>"
                     "$('form').submit(function(e){"
                     "e.preventDefault();"
                     "var form = $('#upload_form')[0];"
                     "var data = new FormData(form);"
                     " $.ajax({"
                     "url: '/update',"
                     "type: 'POST',"
                     "data: data,"
                     "contentType: false,"
                     "processData:false,"
                     "xhr: function() {"
                     "var xhr = new window.XMLHttpRequest();"
                     "xhr.upload.addEventListener('progress', function(evt) {"
                     "if (evt.lengthComputable) {"
                     "var per = evt.loaded / evt.total;"
                     "$('#prg').html('progress: ' + Math.round(per*100) + '%');"
                     "}"
                     "}, false);"
                     "return xhr;"
                     "},"
                     "success:function(d, s) {"
                     "console.log('success!')"
                     "},"
                     "error: function (a, b, c) {"
                     "}"
                     "});"
                     "});"
                     "</script>";

String printDirectory(File dir, int numTabs)
{
    String response = "";
    dir.rewindDirectory();

    while (true)
    {
        File entry = dir.openNextFile();
        if (!entry)
        {
            // no more files
            //Serial.println("**nomorefiles**");
            break;
        }
        for (uint8_t i = 0; i < numTabs; i++)
        {
            Serial.print('\t'); // we'll have a nice indentation
        }
        // Recurse for directories, otherwise print the file size
        if (entry.isDirectory())
        {
            printDirectory(entry, numTabs + 1);
        }
        else
        {
            response += String("<a href='") + String(entry.name()) + String("'>") + String(entry.name()) + String("</a>") + String("</br>");
        }
        entry.close();
    }
    return String("List files:</br>") + response + String("</br></br> Upload file:") + serverIndex;
}

void handleRoot()
{
    root = SD.open("/");
    String res = printDirectory(root, 0);
    server.send(200, "text/html", res);
}

bool loadFromSDCARD(String path)
{
    path.toLowerCase();
    String dataType = "text/plain";
    if (path.endsWith("/"))
        path += "index.htm";

    if (path.endsWith(".src"))
        path = path.substring(0, path.lastIndexOf("."));
    else if (path.endsWith(".jpg"))
        dataType = "image/jpeg";
    else if (path.endsWith(".txt"))
        dataType = "text/plain";
    else if (path.endsWith(".zip"))
        dataType = "application/zip";
    Serial.println(dataType);
    File dataFile = SD.open(path.c_str());

    Serial.println(path.c_str());

    if (!dataFile)
        return false;

    if (server.streamFile(dataFile, dataType) != dataFile.size())
    {
        Serial.println("Sent less data than expected!");
    }

    dataFile.close();
    return true;
}

void handleNotFound()
{
    if (loadFromSDCARD(server.uri()))
        return;
    String message = "SDCARD Not Detected\n\n";
    message += "URI: ";
    message += server.uri();
    message += "\nMethod: ";
    message += (server.method() == HTTP_GET) ? "GET" : "POST";
    message += "\nArguments: ";
    message += server.args();
    message += "\n";
    for (uint8_t i = 0; i < server.args(); i++)
    {
        message += " NAME:" + server.argName(i) + "\n VALUE:" + server.arg(i) + "\n";
    }
    server.send(404, "text/plain", message);
    Serial.println(message);
}

//Image Display
int print_img(String filename)
{
    File f = SD.open(filename.c_str());
    if (!f)
    {
        Serial.println("Failed to open file for reading");
        return 0;
    }

    int X = 480;
    int Y = 320;
    uint8_t RGB[3 * X];
    for (int row = 0; row < Y; row++)
    {
        f.seek(54 + 3 * X * row);
        f.read(RGB, 3 * X);
        for (int col = 0; col < X; col++)
        {
            tft.drawPixel(col, Y - 1 - row, tft.color565(RGB[col * 3 + 2], RGB[col * 3 + 1], RGB[col * 3]));
        }
    }

    f.close();

    return 0;
}

int print_txt(String filename)
{
    File f = SD.open(filename.c_str());
    if (!f)
    {
        Serial.println("Failed to open file for reading");
        return 0;
    }
    Serial.println("");

    int pos_y = 10;

    tft.fillScreen(TFT_BLACK);
    tft.setTextColor(TFT_WHITE);
    tft.setTextSize(4);

    tft.setCursor(10, pos_y);

    String oneline = "";

    while (f.available())
    {
        int c = f.read();
        if (c == '\n')
        {
            tft.println(oneline);
            Serial.println(oneline);
            oneline = "";
            tft.setCursor(10, pos_y += 32);
        }
        else
        {
            oneline += (char)c;
        }
    }

    tft.println(oneline);
    Serial.println(oneline);

    f.close();
    Serial.println("");

    return 0;
}
