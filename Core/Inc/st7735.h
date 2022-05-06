/**************************************************************************/
/*! 
    @file     ST7735.h
*/
/**************************************************************************/

#ifndef __ST7735_H__
#define __ST7735_H__

#include "gpio.h"   

#define ST7735_PANEL_WIDTH  160
#define ST7735_PANEL_HEIGHT 128

void lcd7735_sendbyte(unsigned char data);
void lcd7735_send2byte(uint8_t msb, uint8_t lsb);
void WriteCmd(unsigned char cmd);
void WriteData(unsigned char data);
void st7735SetAddrWindow(unsigned char x0, unsigned char y0, unsigned char x1, unsigned char y1);
void lcdInit(void);
unsigned short lcdGetWidth(void);
unsigned short lcdGetHeight(void);
void lcdFillRGB(unsigned short color);

unsigned int LCD_FastShowChar(unsigned int x,unsigned int y,unsigned char num);
void LCD_ShowString(unsigned short x,unsigned short y, char *p);
void LCD_SetFont(const unsigned char * font, unsigned int color);
void LCD_Refresh(void);
unsigned short FindColor (unsigned char color);
void lcdSetOrientation(unsigned char orientation);
void ClearLcdMemory(void);

void LcdDrawRectangle(unsigned short x0,unsigned short x1,unsigned short y0,unsigned short y1,unsigned short color);
void LcdDrawGraphSimple(unsigned int *buf, unsigned int color);
void LcdDrawGraph(unsigned int *bufLow,unsigned int *bufMiddle, unsigned int *bufHigh);
void LcdDrawUvGraph(unsigned int Low,unsigned int Middle, unsigned int High);
void LcdDrawASGraph(unsigned int left,unsigned int right);
void LcdDrawMgGraph(int *buf, int low, int high);
void LCD_ShowStringCentered(unsigned short y, char *p);
void LcdDenomResult(void);
void lcdDrawHLine(unsigned short x0, unsigned short x1, unsigned short y, unsigned short color);
void lcdDrawVLine(unsigned short x, unsigned short y0, unsigned short y1, unsigned short color);
void LCD_ShowStringSize(unsigned short x,unsigned short y, char *p,unsigned int size);
void ShowNotAllowNote (void);
void LCD_ShowCashBagDistance(int distance);

void LCD_DrawBMP(const char* buf, int x0, int y0, int w, int h);
void LCD_CalibrateWhiteSheet(void);
void LCD_CalibrateBlackSheet(void);	
void LCD_CalibrateYellowSheet(void);	
void LCD_ShowJam(void);


/**************************************************************************
    ST7735 CONNECTOR - HY-1.8
    -----------------------------------------------------------------------
    Pin   Function        Notes
    ===   ==============  ===============================
      1   GND
      2   VCC             3.3V
      3   NC
      4   NC
      5   NC
      6   RESET           Hardware Reset
      7   A0              Data/Command Select (1-data, 0-command)
      8   SDA             Serial Data
      9   SCK             Serial Clock
     10   CS              Chip Select
     11   SCK   --------+
     12   MISO          | SD-Card slot
     13   MOSI          |
     14   CS    --------+
     15   LED+
     16   LED-

 **************************************************************************/

#define CLR_RS  TFT_C_D(0);
#define SET_RS  TFT_C_D(1);
#define CLR_SDA TFT_SDA(0);
#define SET_SDA TFT_SDA(1);
#define CLR_SCL TFT_SCK(0);
#define SET_SCL TFT_SCK(1);
#define CLR_CS  TFT_CS(0);
#define SET_CS  TFT_CS(1);
#define CLR_RES TFT_RESET(0);
#define SET_RES TFT_RESET(1);


// System control functions
#define ST7735_NOP       (0x0)
#define ST7735_SWRESET   (0x01)
#define ST7735_RDDID     (0x04)
#define ST7735_RDDST     (0x09)
#define ST7735_RDDPM     (0x0A)
#define ST7735_RDDMADCTL (0x0B)
#define ST7735_RDDCOLMOD (0x0C)
#define ST7735_RDDIM     (0x0D)
#define ST7735_RDDSM     (0x0E)
#define ST7735_SLPIN     (0x10)
#define ST7735_SLPOUT    (0x11)
#define ST7735_PTLON     (0x12)
#define ST7735_NORON     (0x13)
#define ST7735_INVOFF    (0x20)
#define ST7735_INVON     (0x21)
#define ST7735_GAMSET    (0x26)
#define ST7735_DISPOFF   (0x28)
#define ST7735_DISPON    (0x29)
#define ST7735_CASET     (0x2A)
#define ST7735_RASET     (0x2B)
#define ST7735_RAMWR     (0x2C)
#define ST7735_RAMRD     (0x2E)
#define ST7735_PTLAR     (0x30)
#define ST7735_TEOFF     (0x34)
#define ST7735_TEON      (0x35)
#define ST7735_MADCTL    (0x36)
#define ST7735_IDMOFF    (0x38)
#define ST7735_IDMON     (0x39)
#define ST7735_COLMOD    (0x3A)
#define ST7735_RDID1     (0xDA)
#define ST7735_RDID2     (0xDB)
#define ST7735_RDID3     (0xDC)

// Panel control functions
#define ST7735_FRMCTR1   (0xB1)
#define ST7735_FRMCTR2   (0xB2)
#define ST7735_FRMCTR3   (0xB3)
#define ST7735_INVCTR    (0xB4)
#define ST7735_DISSET5   (0xB6)
#define ST7735_PWCTR1    (0xC0)
#define ST7735_PWCTR2    (0xC1)
#define ST7735_PWCTR3    (0xC2)
#define ST7735_PWCTR4    (0xC3)
#define ST7735_PWCTR5    (0xC4)
#define ST7735_VMCTR1    (0xC5)
#define ST7735_VMOFCTR   (0xC7)
#define ST7735_WRID2     (0xD1)
#define ST7735_WRID3     (0xD2)
#define ST7735_PWCTR6    (0xFC)
#define ST7735_NVFCTR1   (0xD9)
#define ST7735_NVFCTR2   (0xDE)
#define ST7735_NVFCTR3   (0xDF)

#define ST7735_GMCTRP1   (0xE0)
#define ST7735_GMCTRN1   (0xE1)
#define ST7735_EXTCTRL   (0xF0)
#define ST7735_VCOM4L    (0xFF)

// Color definitions
#define BLACK     0x0000
#define BLUE      0x001F
#define RED       0xF800
#define GREEN     0x07E0
#define CYAN      0x07FF
#define MAGENTA   0xF81F
#define YELLOW    0xFFE0
#define WHITE     0xFFFF
#define BRED      0XF81F
#define GRED 			0XFFE0
#define GBLUE			0X07FF
#define BROWN 		0XBC40 
#define BRRED 		0XFC07
#define GRAY  		0X8430 
#define DARKBLUE    0X01CF	
#define LIGHTBLUE   0X7D7C	 
#define GRAYBLUE    0X5458 
#define LIGHTGREEN  0X841F 
#define LGRAY 			0XC618 
#define LGRAYBLUE   0XA651 
#define LBBLUE      0X2B12 

#define black 0x00
#define blue  0x1F
#define green 0x7E
#define white 0xFF
#define red   0xF8
#define yellow 0xFE
#define grey 0xEE

extern const unsigned char * GlobalFont;
extern unsigned int Paint_Color;
extern unsigned int Back_Color;

extern const unsigned char Arial_15x17[224*34];
extern const unsigned char Arial_22x23[224*69];	  	// 14pt
extern const unsigned char Arial_26x28[224*112];
extern const unsigned char Arial_31x33[224*132];	// 20pt
extern const unsigned char Arial_36x37[224*185];	// 24pt
extern const unsigned char Arial_44x46[224*276];  	// 30pt

#endif
