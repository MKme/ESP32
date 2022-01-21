//全是球，采用
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

static LGFX lcd;

static std::uint32_t sec, psec;
static std::uint32_t fps = 0, frame_count = 0;
static std::int32_t lcd_width;
static std::int32_t lcd_height;

struct obj_info_t
{
    std::int32_t x;
    std::int32_t y;
    std::int32_t r;
    std::int32_t dx;
    std::int32_t dy;
    std::uint32_t color;

    void move()
    {
        x += dx;
        y += dy;
        if (x < 0)
        {
            x = 0;
            if (dx < 0)
                dx = -dx;
        }
        else if (x >= lcd_width)
        {
            x = lcd_width - 1;
            if (dx > 0)
                dx = -dx;
        }
        if (y < 0)
        {
            y = 0;
            if (dy < 0)
                dy = -dy;
        }
        else if (y >= lcd_height)
        {
            y = lcd_height - 1;
            if (dy > 0)
                dy = -dy;
        }
    }
};

static constexpr std::uint32_t obj_count = 200;
static obj_info_t objects[obj_count];

static LGFX_Sprite sprites[2];
static int_fast16_t sprite_height;

void setup(void)
{
    pinMode(LCD_CS, OUTPUT);
    pinMode(LCD_BLK, OUTPUT);

    digitalWrite(LCD_CS, LOW);
    digitalWrite(LCD_BLK, HIGH);

    lcd.init();

    lcd_width = lcd.width();
    lcd_height = lcd.height();
    obj_info_t *a;
    for (std::uint32_t i = 0; i < obj_count; ++i)
    {
        a = &objects[i];
        a->color = (uint32_t)rand() | 0x808080U;
        a->x = random(lcd_width);
        a->y = random(lcd_height);
        a->dx = random(1, 4) * (i & 1 ? 1 : -1);
        a->dy = random(1, 4) * (i & 2 ? 1 : -1);
        a->r = 8 + (i & 0x07);
    }

    uint32_t div = 2;
    for (;;)
    {
        sprite_height = (lcd_height + div - 1) / div;
        bool fail = false;
        for (std::uint32_t i = 0; !fail && i < 2; ++i)
        {
            sprites[i].setColorDepth(lcd.getColorDepth());
            sprites[i].setFont(&fonts::Font2);
            fail = !sprites[i].createSprite(lcd_width, sprite_height);
        }
        if (!fail)
            break;
        for (std::uint32_t i = 0; i < 2; ++i)
        {
            sprites[i].deleteSprite();
        }
        ++div;
    }
    lcd.startWrite();
    lcd.setAddrWindow(0, 0, lcd_width, lcd_height);
}

void loop(void)
{
    static std::uint_fast8_t flip = 0;

    obj_info_t *a;
    for (std::uint32_t i = 0; i != obj_count; i++)
    {
        objects[i].move();
    }
    for (std::int32_t y = 0; y < lcd_height; y += sprite_height)
    {
        flip = flip ? 0 : 1;
        sprites[flip].clear();
        for (std::uint32_t i = 0; i != obj_count; i++)
        {
            a = &objects[i];
            if ((a->y + a->r >= y) && (a->y - a->r <= y + sprite_height))
                sprites[flip].drawCircle(a->x, a->y - y, a->r, a->color);
        }

        if (y == 0)
        {
            sprites[flip].setCursor(1, 1);
            sprites[flip].setTextColor(TFT_BLACK);
            sprites[flip].printf("obj:%d fps:%d", obj_count, fps);
            sprites[flip].setCursor(0, 0);
            sprites[flip].setTextColor(TFT_WHITE);
            sprites[flip].printf("obj:%d fps:%d", obj_count, fps);
        }
        sprites[flip].pushSprite(&lcd, 0, y);
    }

    ++frame_count;
    sec = millis() / 1000;
    if (psec != sec)
    {
        psec = sec;
        fps = frame_count;
        frame_count = 0;
    }
}
