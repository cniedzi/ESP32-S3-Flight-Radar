#pragma once

#include <LovyanGFX.hpp>

#define TFT_DC 14
#define TFT_RST 18
#define TFT_CS 10
#define TFT_BL 21
#define TFT_MOSI 11 
#define TFT_SCLK 12 
#define TFT_MISO -1

class LGFX : public lgfx::LGFX_Device
{
    lgfx::Panel_ILI9488IPS _panel;
    lgfx::Bus_SPI _bus;

public:
    LGFX(void)
    {
        {
            auto cfg = _bus.config();
            cfg.spi_host = SPI2_HOST;     // ESP32-S2, C3, S3: SPI2_HOST or SPI3_HOST / ESP32 : VSPI_HOST or HSPI_HOST
            cfg.spi_mode = 0;
            cfg.freq_write = 79000000;
            cfg.freq_read  = 16000000;
            cfg.spi_3wire  = false;
            cfg.use_lock   = true;
            #ifdef ESP32P4
            cfg.dma_channel = 0;
            #else
            cfg.dma_channel = SPI_DMA_CH_AUTO; //Ustaw kanał DMA, który ma być używany (0 = brak DMA / 1 = 1 kanał / 2 = kanał / SPI_DMA_CH_AUTO = ustawienie automatyczne)
            #endif
            cfg.pin_sclk = TFT_SCLK;
            cfg.pin_mosi = TFT_MOSI;
            cfg.pin_miso = TFT_MISO;
            cfg.pin_dc   = TFT_DC;
            _bus.config(cfg);
            _panel.setBus(&_bus);
        }
        {
            auto cfg = _panel.config();
            cfg.pin_cs        = TFT_CS;
            cfg.pin_rst       = TFT_RST;
            cfg.memory_width  = 320;
            cfg.memory_height = 480;
            cfg.panel_width   = 320;
            cfg.panel_height  = 480;
            cfg.offset_x      = 0;
            cfg.offset_y      = 0;
            cfg.offset_rotation = 0;
            cfg.dummy_read_pixel = 8;
            cfg.dummy_read_bits  = 1;
            cfg.readable     = false;
            cfg.invert       = true;
            cfg.rgb_order    = false;
            cfg.bus_shared   = false;
            _panel.config(cfg);
        }
        
        setPanel(&_panel);
    }
};