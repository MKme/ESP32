#define LGFX_USE_V1

#include <SPI.h>
#include <Simple_HCSR04.h>
#include <LovyanGFX.hpp>
#include <Arduino.h>

#define LCD_CS 37
#define LCD_BLK 45

#define ECHO_PIN 17 /// the pin at which the sensor echo is connected
#define TRIG_PIN 18 /// the pin at which the sensor trig is connected

// Define meter size as multiplier of 240 pixels wide 1.0 and 1.3333 work OK
//#define M_SIZE 1.3333
#define M_SIZE 2
Simple_HCSR04 *sensor;

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

static LGFX tft;
const char union_text[] = "cm";

#define TFT_GREY 0x5AEB

float ltx = 0;                                   // Saved x coord of bottom of needle
uint16_t osx = M_SIZE * 120, osy = M_SIZE * 120; // Saved x & y coords
uint32_t updateTime = 0;                         // time for next update

int old_analog = -999; // Value last displayed

int value[6] = {0, 0, 0, 0, 0, 0};
int old_value[6] = {-1, -1, -1, -1, -1, -1};
int d = 0;

void setup(void)
{
    pinMode(LCD_CS, OUTPUT);
    pinMode(LCD_BLK, OUTPUT);

    digitalWrite(LCD_CS, LOW);
    digitalWrite(LCD_BLK, HIGH);

    Serial.begin(115200); // For debug

    sensor = new Simple_HCSR04(ECHO_PIN, TRIG_PIN);

    tft.init();
    tft.setRotation(3);
    tft.fillScreen(TFT_BLACK);
    analogMeter(); // Draw analogue meter

    tft.setCursor(10, 300);
    tft.setTextSize(2);
    tft.println("Makerfabs Ultrasonic Ranging");

    updateTime = millis(); // Next update time
}

void loop()
{
    if (updateTime <= millis())
    {
        unsigned long distance = sensor->measure()->cm();
        if (distance > 100)
            distance = 100;
        Serial.print("distance: ");
        Serial.print(distance);
        Serial.print("cm\n");
        updateTime = millis() + 35; // Update emter every 35 milliseconds

        // Create a Sine wave for testing
        // d += 4;
        // if (d >= 360)
        //   d = 0;
        // value[0] = 50 + 50 * sin((d + 0) * 0.0174532925);

        // plotNeedle(value[0], 0); // It takes between 2 and 12ms to replot the needle with zero delay

        plotNeedle(distance, 0);
    }
}

// #########################################################################
//  Draw the analogue meter on the screen
// #########################################################################
void analogMeter()
{

    // Meter outline
    tft.fillRect(0, 0, M_SIZE * 239, M_SIZE * 126, TFT_GREY);
    tft.fillRect(5, 3, M_SIZE * 230, M_SIZE * 119, TFT_WHITE);

    tft.setTextColor(TFT_BLACK); // Text colour

    // Draw ticks every 5 degrees from -50 to +50 degrees (100 deg. FSD swing)
    for (int i = -50; i < 51; i += 5)
    {
        // Long scale tick length
        int tl = 15;

        // Coodinates of tick to draw
        float sx = cos((i - 90) * 0.0174532925);
        float sy = sin((i - 90) * 0.0174532925);
        uint16_t x0 = sx * (M_SIZE * 100 + tl) + M_SIZE * 120;
        uint16_t y0 = sy * (M_SIZE * 100 + tl) + M_SIZE * 140;
        uint16_t x1 = sx * M_SIZE * 100 + M_SIZE * 120;
        uint16_t y1 = sy * M_SIZE * 100 + M_SIZE * 140;

        // Coordinates of next tick for zone fill
        float sx2 = cos((i + 5 - 90) * 0.0174532925);
        float sy2 = sin((i + 5 - 90) * 0.0174532925);
        int x2 = sx2 * (M_SIZE * 100 + tl) + M_SIZE * 120;
        int y2 = sy2 * (M_SIZE * 100 + tl) + M_SIZE * 140;
        int x3 = sx2 * M_SIZE * 100 + M_SIZE * 120;
        int y3 = sy2 * M_SIZE * 100 + M_SIZE * 140;

        // Yellow zone limits
        //if (i >= -50 && i < 0) {
        //  tft.fillTriangle(x0, y0, x1, y1, x2, y2, TFT_YELLOW);
        //  tft.fillTriangle(x1, y1, x2, y2, x3, y3, TFT_YELLOW);
        //}

        // Green zone limits
        if (i >= 0 && i < 25)
        {
            tft.fillTriangle(x0, y0, x1, y1, x2, y2, TFT_GREEN);
            tft.fillTriangle(x1, y1, x2, y2, x3, y3, TFT_GREEN);
        }

        // Orange zone limits
        if (i >= 25 && i < 50)
        {
            tft.fillTriangle(x0, y0, x1, y1, x2, y2, TFT_ORANGE);
            tft.fillTriangle(x1, y1, x2, y2, x3, y3, TFT_ORANGE);
        }

        // Short scale tick length
        if (i % 25 != 0)
            tl = 8;

        // Recalculate coords incase tick lenght changed
        x0 = sx * (M_SIZE * 100 + tl) + M_SIZE * 120;
        y0 = sy * (M_SIZE * 100 + tl) + M_SIZE * 140;
        x1 = sx * M_SIZE * 100 + M_SIZE * 120;
        y1 = sy * M_SIZE * 100 + M_SIZE * 140;

        // Draw tick
        tft.drawLine(x0, y0, x1, y1, TFT_BLACK);

        // Check if labels should be drawn, with position tweaks
        if (i % 25 == 0)
        {
            // Calculate label positions
            x0 = sx * (M_SIZE * 100 + tl + 10) + M_SIZE * 120;
            y0 = sy * (M_SIZE * 100 + tl + 10) + M_SIZE * 140;
            switch (i / 25)
            {
            case -2:
                tft.drawCentreString("0", x0, y0 - 12, 2);
                break;
            case -1:
                tft.drawCentreString("25", x0, y0 - 9, 2);
                break;
            case 0:
                tft.drawCentreString("50", x0, y0 - 7, 2);
                break;
            case 1:
                tft.drawCentreString("75", x0, y0 - 9, 2);
                break;
            case 2:
                tft.drawCentreString("100", x0, y0 - 12, 2);
                break;
            }
        }

        // Now draw the arc of the scale
        sx = cos((i + 5 - 90) * 0.0174532925);
        sy = sin((i + 5 - 90) * 0.0174532925);
        x0 = sx * M_SIZE * 100 + M_SIZE * 120;
        y0 = sy * M_SIZE * 100 + M_SIZE * 140;
        // Draw scale arc, don't draw the last part
        if (i < 50)
            tft.drawLine(x0, y0, x1, y1, TFT_BLACK);
    }

    tft.drawString(union_text, M_SIZE * (5 + 230 - 40), M_SIZE * (119 - 20), 1); // Units at bottom right
    //tft.drawCentreString(union_text, M_SIZE * 120, M_SIZE * 70, 1);              // Comment out to avoid font 4
    tft.drawRect(5, 3, M_SIZE * 230, M_SIZE * 119, TFT_BLACK);                   // Draw bezel line

    plotNeedle(0, 0); // Put meter needle at 0
}

// #########################################################################
// Update needle position
// This function is blocking while needle moves, time depends on ms_delay
// 10ms minimises needle flicker if text is drawn within needle sweep area
// Smaller values OK if text not in sweep area, zero for instant movement but
// does not look realistic... (note: 100 increments for full scale deflection)
// #########################################################################
void plotNeedle(int value, byte ms_delay)
{
    tft.setTextColor(TFT_BLACK, TFT_WHITE);
    char buf[8];
    dtostrf(value, 4, 0, buf);
    tft.drawRightString(buf, M_SIZE * 40, M_SIZE * (119 - 20), 2);

    if (value < -10)
        value = -10; // Limit value to emulate needle end stops
    if (value > 110)
        value = 110;

    // Move the needle until new value reached
    while (!(value == old_analog))
    {
        if (old_analog < value)
            old_analog++;
        else
            old_analog--;

        if (ms_delay == 0)
            old_analog = value; // Update immediately if delay is 0

        float sdeg = map(old_analog, -10, 110, -150, -30); // Map value to angle
        // Calcualte tip of needle coords
        float sx = cos(sdeg * 0.0174532925);
        float sy = sin(sdeg * 0.0174532925);

        // Calculate x delta of needle start (does not start at pivot point)
        float tx = tan((sdeg + 90) * 0.0174532925);

        // Erase old needle image
        tft.drawLine(M_SIZE * (120 + 20 * ltx - 1), M_SIZE * (140 - 20), osx - 1, osy, TFT_WHITE);
        tft.drawLine(M_SIZE * (120 + 20 * ltx), M_SIZE * (140 - 20), osx, osy, TFT_WHITE);
        tft.drawLine(M_SIZE * (120 + 20 * ltx + 1), M_SIZE * (140 - 20), osx + 1, osy, TFT_WHITE);

        // Re-plot text under needle
        tft.setTextColor(TFT_BLACK);
        tft.drawCentreString(union_text, M_SIZE * 120, M_SIZE * 70, 1); // // Comment out to avoid font 4

        // Store new needle end coords for next erase
        ltx = tx;
        osx = M_SIZE * (sx * 98 + 120);
        osy = M_SIZE * (sy * 98 + 140);

        // Draw the needle in the new postion, magenta makes needle a bit bolder
        // draws 3 lines to thicken needle
        tft.drawLine(M_SIZE * (120 + 20 * ltx - 1), M_SIZE * (140 - 20), osx - 1, osy, TFT_RED);
        tft.drawLine(M_SIZE * (120 + 20 * ltx), M_SIZE * (140 - 20), osx, osy, TFT_MAGENTA);
        tft.drawLine(M_SIZE * (120 + 20 * ltx + 1), M_SIZE * (140 - 20), osx + 1, osy, TFT_RED);

        // Slow needle down slightly as it approaches new postion
        if (abs(old_analog - value) < 10)
            ms_delay += ms_delay / 5;

        // Wait before next update
        delay(ms_delay);
    }
}