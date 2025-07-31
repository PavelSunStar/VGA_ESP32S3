#pragma once

#include <Arduino.h>
#include <esp_lcd_panel_rgb.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>

class VGA_esp32s3{
    public:
        VGA_esp32s3();         // Конструктор
        ~VGA_esp32s3();        // Деструктор

	    bool initWithSize(int frameWidth, int frameHeight, int bits, bool dBuff = true);
    	bool init(int width, int height, int scale = 2, int hborder = 0, int yborder = 0, int bits = 8, int* pins = NULL, bool usePsram = true);
	    bool deinit();
        void setViewport(int x1, int x2, int y1, int y2);

	    void swap();        
	    uint8_t* getDrawBuffer();
		uint16_t* getDrawBuffer16();

	    int frameWidth(){return _frameWidth;}
	    int frameHeight(){return _frameHeight;}
	    int getColorBits(){return _colorBits;}
		int getBytesPerPixel(){return _bytePerPixel;}
		int getMaxCol(){return _maxCol;}

        int getvX1(){return _vX1;}
        int getvX2(){return _vX2;}
        int getvY1(){return _vY1;}
        int getvY2(){return _vY2;}
        int getvWidth(){return _vWidth;}
        int getvHeight(){return _vHeight;}

    private:
      // Добавьте любые переменные, которые нужно инициализировать/освободить в конструкторе и деструкторе

    protected:
	    SemaphoreHandle_t _sem_vsync_end;
	    SemaphoreHandle_t _sem_gui_ready;
	    esp_lcd_panel_handle_t _panel_handle = NULL;

		uint8_t *_frameBuffers[2] = {nullptr, nullptr};
		uint16_t *_frameBuffers16[2] = {nullptr, nullptr};

		bool _dBuff = true;
	    int _frameBufferIndex;
	    int _frameWidth;
	    int _frameHeight;
	    int _screenWidth;
	    int _screenHeight;
	    int _frameScale = 2;
	    int _colorBits = 8;
		int _bytePerPixel = 1;
		int _maxCol;
	    int _bounceBufferLines;
	    int _hBorder;
	    int _vBorder;
	    int _lastBounceBufferPos;
	    bool _frameBuffersInPsram = false;

		//Viewport
        int _vX1, 
            _vY1, 
            _vX2, 
            _vY2, 
            _vWidth, 
            _vHeight;        

	    bool validConfig(int width, int height, int scale = 2, int hborder = 0, int yborder = 0, int bits = 8, int* pins = NULL, bool usePsram = false);
	    static bool IRAM_ATTR vsyncEvent(esp_lcd_panel_handle_t panel, const esp_lcd_rgb_panel_event_data_t *edata, void *user_ctx);
	    static bool IRAM_ATTR bounceEvent(esp_lcd_panel_handle_t panel, void* bounce_buf, int pos_px, int len_bytes, void* user_ctx);
	    void swapBuffers();
};




/*
#pragma once

#include <esp_lcd_panel_rgb.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>

class VGA_esp32s3 {
    public:
        VGA_esp32s3();         // Конструктор
        ~VGA_esp32s3();        // Деструктор

        void init(int scrWidth, int scrHeight, int colorBits = 8, int dBuff = true, bool psRam = true);

        bool usePsRam()         {return _psRam;}
        int  getWidth()         {return _scrWidth;}
        int  getHeight()        {return _scrHeight;}
        int  getColorBits()     {return _scrColorBits;}
        int  getBytesPerPixel() {return _scrBytesPerPixel;}
        int  getSize()          {return _scrSize;}
        bool getDBuff()         {return _dBuff;}

        uint8_t* getBuff();
        uint16_t* getBuff16();

        void vsyncWait();

    private:

    protected:
		bool _psRam = false;
    	int _scrWidth, _scrHeight;
		int _scrColorBits;
		int _scrBytesPerPixel;
        int _scrSize;
        bool _dBuff;

		uint8_t *_scrBuffers[2];
		uint16_t *_scrBuffers16[2]; 

	//----------------------------------------------------------------------------------------------------
		int _scrBufferIndex = 0;
        int _bounceBufferLines = 0; 
        int _lastBounceBufferPos = 0;
		
	    SemaphoreHandle_t _sem_vsync_end;
	    SemaphoreHandle_t _sem_gui_ready;
	    esp_lcd_panel_handle_t _panel_handle = NULL;

	    static bool vsyncEvent(esp_lcd_panel_handle_t panel, const esp_lcd_rgb_panel_event_data_t *edata, void *user_ctx);
	    static bool bounceEvent(esp_lcd_panel_handle_t panel, void* bounce_buf, int pos_px, int len_bytes, void* user_ctx);
	    void swapBuffers();         
   
};        
*/