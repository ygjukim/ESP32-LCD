#ifndef LGFX_ESP32S3_ILI9488_GT911_35_H
#define LGFX_ESP32S3_ILI9488_GT911_35_H

#define LGFX_USE_V1
#include <LovyanGFX.hpp>

#include <driver/i2c.h>

class LGFX : public lgfx::LGFX_Device
{
  lgfx::Panel_ILI9488     _panel_instance;
  lgfx::Bus_SPI           _bus_instance;
  lgfx::Light_PWM         _light_instance;
//  lgfx::Touch_GT911       _touch_instance;

public:

  LGFX(void)
  {
    {
      auto cfg = _bus_instance.config();

      cfg.spi_host = SPI3_HOST;
      cfg.spi_mode = 0;             // SPI通信モードを設定 (0 ~ 3)
      cfg.freq_write = 40000000;    // 送信時のSPIクロック (最大80MHz, 80MHzを整数で割った値に丸められます)
      cfg.freq_read  = 16000000;    // 受信時のSPIクロック
      cfg.spi_3wire  = true;        // 受信をMOSIピンで行う場合はtrueを設定
      cfg.use_lock   = true;        // トランザクションロックを使用する場合はtrueを設定
      cfg.dma_channel = SPI_DMA_CH_AUTO; // 使用するDMAチャンネルを設定 (0=DMA不使用 / 1=1ch / 2=ch / SPI_DMA_CH_AUTO=自動設定)
      // ※ ESP-IDFバージョンアップに伴い、DMAチャンネルはSPI_DMA_CH_AUTO(自動設定)が推奨になりました。1ch,2chの指定は非推奨になります。
      cfg.pin_sclk = 12;            // SPIのSCLKピン番号を設定
      cfg.pin_mosi = 11;            // SPIのMOSIピン番号を設定
      cfg.pin_miso = 13;            // SPIのMISOピン番号を設定 (-1 = disable)
      cfg.pin_dc   = 5;             // SPIのD/Cピン番号を設定  (-1 = disable)
      // SDカードと共通のSPIバスを使う場合、MISOは省略せず必ず設定してください。

      _bus_instance.config(cfg);    // 設定値をバスに反映します。
      _panel_instance.setBus(&_bus_instance);      // バスをパネルにセットします。
    }

    {
      auto cfg = _panel_instance.config();    // 表示パネル設定用の構造体を取得します。

      cfg.pin_cs           =    10;  // CSが接続されているピン番号   (-1 = disable)
      cfg.pin_rst          =    4;  // RSTが接続されているピン番号  (-1 = disable)
      cfg.pin_busy         =    -1;  // BUSYが接続されているピン番号 (-1 = disable)

      // ※ 以下の設定値はパネル毎に一般的な初期値が設定されていますので、不明な項目はコメントアウトして試してみてください。

      cfg.panel_width      =   320;  // 実際に表示可能な幅
      cfg.panel_height     =   480;  // 実際に表示可能な高さ
      cfg.offset_x         =     0;  // パネルのX方向オフセット量
      cfg.offset_y         =     0;  // パネルのY方向オフセット量
      cfg.offset_rotation  =     0;  // 回転方向の値のオフセット 0~7 (4~7は上下反転)
      cfg.dummy_read_pixel =     8;  // ピクセル読出し前のダミーリードのビット数
      cfg.dummy_read_bits  =     1;  // ピクセル以外のデータ読出し前のダミーリードのビット数
      cfg.readable         =  true;  // データ読出しが可能な場合 trueに設定
      cfg.invert           = false;  // パネルの明暗が反転してしまう場合 trueに設定
      cfg.rgb_order        = false;  // パネルの赤と青が入れ替わってしまう場合 trueに設定
      cfg.dlen_16bit       = false;  // 16bitパラレルやSPIでデータ長を16bit単位で送信するパネルの場合 trueに設定
      cfg.bus_shared       =  true;  // SDカードとバスを共有している場合 trueに設定(drawJpgFile等でバス制御を行います)

// 以下はST7735やILI9163のようにピクセル数が可変のドライバで表示がずれる場合にのみ設定してください。
//    cfg.memory_width     =   240;  // ドライバICがサポートしている最大の幅
//    cfg.memory_height    =   320;  // ドライバICがサポートしている最大の高さ

      _panel_instance.config(cfg);
    }

    { 
      auto cfg = _light_instance.config();    // バックライト設定用の構造体を取得します。

      cfg.pin_bl = 6;              // バックライトが接続されているピン番号
      // cfg.invert = false;           // バックライトの輝度を反転させる場合 true
      // cfg.freq   = 44100;           // バックライトのPWM周波数
      // cfg.pwm_channel = 7;          // 使用するPWMのチャンネル番号

      _light_instance.config(cfg);
      _panel_instance.setLight(&_light_instance);  // バックライトをパネルにセットします。
    }
/*
    { 
      auto cfg = _touch_instance.config();

      cfg.x_min      = 0;    // タッチスクリーンから得られる最小のX値(生の値)
      cfg.x_max      = 319;  // タッチスクリーンから得られる最大のX値(生の値)
      cfg.y_min      = 0;    // タッチスクリーンから得られる最小のY値(生の値)
      cfg.y_max      = 479;  // タッチスクリーンから得られる最大のY値(生の値)
      cfg.offset_rotation = 0;// 表示とタッチの向きのが一致しない場合の調整 0~7の値で設定

      cfg.i2c_port   =  I2C_NUM_0;      // 使用するI2Cを選択 (0 or 1)
      cfg.pin_sda    =  GPIO_NUM_8;     // SDAが接続されているピン番号
      cfg.pin_scl    =  GPIO_NUM_9;     // SCLが接続されているピン番号
      cfg.pin_int    =  GPIO_NUM_NC;    // INTが接続されているピン番号
      cfg.pin_rst    =  GPIO_NUM_3;    // INTが接続されているピン番号
//      cfg.i2c_addr   =  0x5D;   // I2Cデバイスアドレス番号
      cfg.freq       =  400000;     // I2Cクロックを設定
      cfg.bus_shared =  false; // 画面と共通のバスを使用している場合 trueを設定

      _touch_instance.config(cfg);
      _panel_instance.setTouch(&_touch_instance);  // タッチスクリーンをパネルにセットします。
    }
*/
    setPanel(&_panel_instance); // 使用するパネルをセットします。
  }
};

#endif  // LGFX_ESP32S3_ILI9488_GT911_35_H