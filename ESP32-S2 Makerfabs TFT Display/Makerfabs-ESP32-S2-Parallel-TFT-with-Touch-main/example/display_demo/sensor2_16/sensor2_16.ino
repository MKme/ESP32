#define LGFX_USE_V1
#include <LovyanGFX.hpp>

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

static LGFX tft;

int list_1[50] = {0};
int list_1_length = 10;
int list_2[50] = {0};
int list_2_length = 30;
int interval = 50;

void setup()
{
    pinMode(LCD_CS, OUTPUT);
    pinMode(LCD_BLK, OUTPUT);

    digitalWrite(LCD_CS, LOW);
    digitalWrite(LCD_BLK, HIGH);

    Serial.begin(115200);

    //Draw frame
    tft.init();
    tft.fillScreen(TFT_BLACK);

    tft.setTextColor(TFT_WHITE);
    tft.setTextSize(2);

    tft.setCursor(10, 40);
    tft.println("Makerfabs line chart");

    delay(1000);
    tft.fillScreen(TFT_BLACK);
    draw_line_chart_window("Random 1", (String) "Interval : " + interval + " ms", list_1_length, 0, 1200, TFT_WHITE);
    draw_line_chart_window2("Random 2", (String) "Interval : " + interval + " ms", list_2_length, 0, 5000, TFT_WHITE);
}

void loop()
{
    draw_line_chart(list_1, list_1_length, 0, 1200, TFT_WHITE);
    draw_line_chart2(list_2, list_2_length, 0, 5000, TFT_YELLOW);
    delay(interval);

    int random_num_1 = random(0, 1200);
    int random_num_2 = random(0, 5000);

    draw_line_chart(list_1, list_1_length, 0, 1200, TFT_BLACK);
    draw_line_chart2(list_2, list_2_length, 0, 5000, TFT_BLACK);
    add_list(list_1, list_1_length, random_num_1);
    add_list(list_2, list_2_length, random_num_2);
}

// Add a num to the beginning of the list.
void add_list(int *list, int length, int num)
{
    for (int i = length - 2; i >= 0; i--)
    {
        *(list + i + 1) = *(list + i);
    }
    *list = num;
}

void draw_line_chart_window(String text1, String text2, int length, int low, int high, int color)
{
    //draw rect and unit
    //tft.drawRect(20, 20, 280, 200, color);
    tft.drawLine(30, 20, 30, 220, color);
    tft.drawLine(30, 220, 300, 220, color);

    tft.drawLine(20, 20, 30, 20, color);
    tft.drawLine(20, 120, 30, 120, color);

    tft.setTextColor(color);
    tft.setTextSize(1);

    tft.setCursor(0, 10);
    tft.println(high);

    tft.setCursor(80, 10);
    tft.println(text1);

    tft.setCursor(0, 110);
    tft.println((high + low) / 2);

    tft.setCursor(0, 210);
    tft.println(low);

    tft.setCursor(80, 230);
    tft.println(text2);

    int x_start = 32;
    int x_unit = 280 / (length - 1);
    for (int i = 0; i < length; i++)
    {
        int x = x_start + x_unit * i;
        if (i != 0 && i != length - 1)
            tft.drawLine(x, 220, x, 225, color);
    }
}

void draw_line_chart(int *list, int length, int low, int high, int color)
{
    //list to position
    int pos[50][2] = {0};
    int detail = 50;
    int x_start = 32;
    int y_start = 218;
    int x_unit = 280 / (length - 1);
    int y_unit = -200 / (detail - 1);
    for (int i = 0; i < length; i++)
    {
        pos[i][0] = x_start + i * x_unit;
        int y = map(*(list + i), low, high, 0, detail);
        if (y > detail)
            y = detail;
        pos[i][1] = y_start + y_unit * y;
    }

    //draw line chart
    for (int i = 0; i < length - 1; i++)
    {
        tft.drawLine(pos[i][0], pos[i][1], pos[i + 1][0], pos[i + 1][1], color);
    }
}

void draw_line_chart_window2(String text1, String text2, int length, int low, int high, int color)
{
    //draw rect and unit
    //tft.drawRect(20, 20, 280, 200, color);
    tft.drawLine(30, 260, 30, 460, color);
    tft.drawLine(30, 460, 300, 460, color);

    tft.drawLine(20, 260, 30, 260, color);
    tft.drawLine(20, 360, 30, 360, color);

    tft.setTextColor(color);
    tft.setTextSize(1);

    tft.setCursor(0, 250);
    tft.println(high);

    tft.setCursor(80, 250);
    tft.println(text1);

    tft.setCursor(0, 350);
    tft.println((high + low) / 2);

    tft.setCursor(0, 450);
    tft.println(low);

    tft.setCursor(80, 470);
    tft.println(text2);

    int x_start = 32;
    int x_unit = 280 / (length - 1);
    for (int i = 0; i < length; i++)
    {
        int x = x_start + x_unit * i;
        if (i != 0 && i != length - 1)
            tft.drawLine(x, 460, x, 465, color);
    }
}

void draw_line_chart2(int *list, int length, int low, int high, int color)
{
    //list to position
    int pos[50][2] = {0};
    int detail = 50;
    int x_start = 32;
    int y_start = 458;
    int x_unit = 280 / (length - 1);
    int y_unit = -200 / (detail - 1);
    for (int i = 0; i < length; i++)
    {
        pos[i][0] = x_start + i * x_unit;
        int y = map(*(list + i), low, high, 0, detail);
        if (y > detail)
            y = detail;
        pos[i][1] = y_start + y_unit * y;
    }

    //draw line chart
    for (int i = 0; i < length - 1; i++)
    {
        tft.drawLine(pos[i][0], pos[i][1], pos[i + 1][0], pos[i + 1][1], color);
    }
}