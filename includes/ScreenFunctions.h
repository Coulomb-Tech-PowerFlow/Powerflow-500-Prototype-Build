#include "tftspi.h"
#include "tft.h"
#include "spiffs_vfs.h"
#include "esp_spiffs.h"
#include "esp_log.h"

#define TurnOffScreen gpio_set_level(PIN_NUM_BCKL, PIN_BCKL_OFF);
#define TurnOnScreen gpio_set_level(PIN_NUM_BCKL, PIN_BCKL_ON);
#define ClearScreen TFT_fillScreen(TFT_BLACK);
#define delay(milli) vTaskDelay(milli / portTICK_RATE_MS);
#define SPI_BUS TFT_HSPI_HOST

int batteryPercentage = 80;

typedef enum
{
	Normal,
	Charging
}SystemState;

SystemState sysState = Normal;

//screen initialisations
void init_Screen(){

    tft_disp_type = DISP_TYPE_ILI9341;
	_width = DEFAULT_TFT_DISPLAY_WIDTH;	  // smaller dimension
	_height = DEFAULT_TFT_DISPLAY_HEIGHT; // larger dimension

	max_rdclock = 8000000;
	TFT_PinsInit();

	spi_lobo_device_handle_t spi;

	spi_lobo_bus_config_t buscfg ={
		.miso_io_num = PIN_NUM_MISO, // set SPI MISO pin
		.mosi_io_num = PIN_NUM_MOSI, // set SPI MOSI pin
		.sclk_io_num = PIN_NUM_CLK,	 // set SPI CLK pin
		.quadwp_io_num = -1,
		.quadhd_io_num = -1,
		.max_transfer_sz = 6 * 1024,
	};

	spi_lobo_device_interface_config_t devcfg ={
		.clock_speed_hz = 8000000,		   // Initial clock out at 8 MHz
		.mode = 0,						   // SPI mode 0
		.spics_io_num = -1,				   // we will use external CS pin
		.spics_ext_io_num = PIN_NUM_CS,	   // external CS pin
		.flags = LB_SPI_DEVICE_HALFDUPLEX, // ALWAYS SET  to HALF DUPLEX MODE!! for display spi
	};

	esp_err_t ret;
    ret = spi_lobo_bus_add_device(SPI_BUS, &buscfg, &devcfg, &spi);
	assert(ret == ESP_OK);
	printf("SPI: display device added to spi bus (%d)\r\n", SPI_BUS);
	disp_spi = spi;

	ret = spi_lobo_device_select(spi, 1);
	assert(ret == ESP_OK);
	ret = spi_lobo_device_deselect(spi);
	assert(ret == ESP_OK);

	printf("SPI: attached display device, speed=%u\r\n",
		spi_lobo_get_speed(spi));
	printf("SPI: bus uses native pins: %s\r\n",
		spi_lobo_uses_native_pins(spi) ? "true" : "false");

	TFT_display_init();

	max_rdclock = find_rd_speed();
	printf("SPI: Max rd speed = %u\r\n", max_rdclock);

	spi_lobo_set_speed(spi, DEFAULT_SPI_CLOCK);
	printf("SPI: Changed speed to %u\r\n", spi_lobo_get_speed(spi));

	font_rotate = 0;
	text_wrap = 0;
	font_transparent = 0;
	font_forceFixed = 0;
	gray_scale = 0;
	TFT_setGammaCurve(DEFAULT_GAMMA_CURVE);
	TFT_setRotation(LANDSCAPE_FLIP);
	TFT_setFont(TestFont, NULL);
	TFT_resetclipwin();

	vfs_spiffs_register();
	if (!spiffs_is_mounted)
		printf("SPIFFS not mounted");
	else
		printf("SPIFFS mounted");

    _fg = TFT_CYAN;
	TFT_setFont(TestFont, NULL);
	TFT_print("The Future", CENTER, CENTER);
    delay(1000);
    ClearScreen;
    TurnOffScreen;
	sysState = Charging;
}

void clear_batGauage(int x, int y, int w, int h, int r){
	TFT_fillRoundRect(x, y, w, h, r, TFT_WHITE);
}

// void clear_batStates(bool &b1,bool &b2, bool &b3, bool &b4, bool &b5, bool &b6){
// 	b1 = b2 = b3 = b4 = b5 = b6 = false;
// }

static void drawBatIcon(){
	
	TFT_drawRoundRect(42, 35, 25, 30, 5, TFT_WHITE);
	TFT_fillRoundRect(42, 35, 25, 30, 5, TFT_WHITE);

	TFT_drawRoundRect(20, 50, 70, 130, 5, TFT_WHITE);
	TFT_fillRoundRect(20, 50, 70, 130, 5, TFT_WHITE);

	bool bat1 = false,bat2=false,bat3=false,bat4=false,bat5=false,bat6=false;

	while (1){

		if(sysState == Charging){

			bool seg5_6 = (batteryPercentage >= 0 && batteryPercentage <20) ? true : false;
			bool seg5_4 = (batteryPercentage >= 20 && batteryPercentage <40) ? true : false;
			bool seg5_3 = (batteryPercentage >= 40 && batteryPercentage <60) ? true : false;
			bool seg5_2 = (batteryPercentage >= 60 && batteryPercentage <80) ? true : false;
			bool seg5_1 = (batteryPercentage >= 80 && batteryPercentage <=100) ? true : false;

			if(!seg5_6)
				bat6 = false;
			if(seg5_6)
				bat6 = true;

			else if(seg5_4)
				bat5 = true;

			else if(seg5_3)
				bat5 = bat4 = true;

			else if(seg5_2)
				bat5 = bat4 = bat3 = true;

			else if(seg5_1)
				bat5 = bat4 = bat3 = bat2 = true;

			if(seg5_6 ){
				bat5 = !bat5;
				if(bat5){
					TFT_drawRoundRect(25, 155, 60, 20, 5, TFT_GREEN);
					TFT_fillRoundRect(25, 155, 60, 20, 5, TFT_GREEN);
				}
				else
					clear_batGauage(25, 155, 60, 20, 5);

				if(bat6){
					TFT_drawRoundRect(25, 177, 60, 2, 0, TFT_RED);
					TFT_fillRoundRect(25, 177, 60, 2, 0, TFT_RED);
				}
			}//

			else if(seg5_4){
				bat4 = !bat4;
				if(bat4){
					TFT_drawRoundRect(25, 130, 60, 20, 5, TFT_GREEN);
					TFT_fillRoundRect(25, 130, 60, 20, 5, TFT_GREEN);
				}
				else clear_batGauage(25, 130, 60, 20, 5);

				if(bat5){
					TFT_drawRoundRect(25, 155, 60, 20, 5, TFT_GREEN);
					TFT_fillRoundRect(25, 155, 60, 20, 5, TFT_GREEN);
				}
			}//

			else if(seg5_3){
				bat3 = !bat3;
				if(bat3){
					TFT_drawRoundRect(25, 105, 60, 20, 5, TFT_GREEN);
					TFT_fillRoundRect(25, 105, 60, 20, 5, TFT_GREEN);
				}
				else clear_batGauage(25, 105, 60, 20, 5);

				if(bat5 && bat4){
					TFT_drawRoundRect(25, 130, 60, 20, 5, TFT_GREEN);
					TFT_fillRoundRect(25, 130, 60, 20, 5, TFT_GREEN);
					TFT_drawRoundRect(25, 155, 60, 20, 5, TFT_GREEN);
					TFT_fillRoundRect(25, 155, 60, 20, 5, TFT_GREEN);
				}
			}//

			else if(seg5_2){
				bat2 = !bat2;
				if(bat2){
				TFT_drawRoundRect(25, 80, 60, 20, 5, TFT_GREEN);
				TFT_fillRoundRect(25, 80, 60, 20, 5, TFT_GREEN);
				}
				else clear_batGauage(25, 80, 60, 20, 5);

				if(bat5 && bat4 && bat3){
					TFT_drawRoundRect(25, 105, 60, 20, 5, TFT_GREEN);
					TFT_fillRoundRect(25, 105, 60, 20, 5, TFT_GREEN);
					TFT_drawRoundRect(25, 130, 60, 20, 5, TFT_GREEN);
					TFT_fillRoundRect(25, 130, 60, 20, 5, TFT_GREEN);
					TFT_drawRoundRect(25, 155, 60, 20, 5, TFT_GREEN);
					TFT_fillRoundRect(25, 155, 60, 20, 5, TFT_GREEN);
				}
			}//

			else if(seg5_1){
				bat1 = !bat1;
				if(bat1){
				TFT_drawRoundRect(25, 55, 60, 20, 5, TFT_GREEN);
				TFT_fillRoundRect(25, 55, 60, 20, 5, TFT_GREEN);
				}
				else clear_batGauage(25, 55, 60, 20, 5);

				if(bat5 && bat4 && bat3 && bat2){
					TFT_drawRoundRect(25, 80, 60, 20, 5, TFT_GREEN);
					TFT_fillRoundRect(25, 80, 60, 20, 5, TFT_GREEN);
					TFT_drawRoundRect(25, 105, 60, 20, 5, TFT_GREEN);
					TFT_fillRoundRect(25, 105, 60, 20, 5, TFT_GREEN);
					TFT_drawRoundRect(25, 130, 60, 20, 5, TFT_GREEN);
					TFT_fillRoundRect(25, 130, 60, 20, 5, TFT_GREEN);
					TFT_drawRoundRect(25, 155, 60, 20, 5, TFT_GREEN);
					TFT_fillRoundRect(25, 155, 60, 20, 5, TFT_GREEN);
				}
			}//

		}//

		else if(sysState == Normal){
			bool bat1Condition = (batteryPercentage >= 90) ? true : false;
			bool bat2Condition = (batteryPercentage >= 80 && batteryPercentage<=89) ? true : false;
			bool bat3Condition = (batteryPercentage >= 60 && batteryPercentage<=79) ? true : false;
			bool bat4Condition = (batteryPercentage >= 40 && batteryPercentage<=59) ? true : false;
			bool bat5Condition = (batteryPercentage >= 10 && batteryPercentage<=39) ? true : false;

			if(bat1Condition){	
				TFT_drawRoundRect(25, 55, 60, 20, 5, TFT_GREEN);
				TFT_fillRoundRect(25, 55, 60, 20, 5, TFT_GREEN);
				TFT_setFont(UBUNTU16_FONT, NULL);
				_fg = TFT_WHITE;
				_bg = TFT_GREEN;
				TFT_print("FULL", 40, 60);
			}

			if(bat1Condition || bat2Condition){
				TFT_drawRoundRect(25, 80, 60, 20, 5, TFT_GREEN);
				TFT_fillRoundRect(25, 80, 60, 20, 5, TFT_GREEN);
				TFT_setFont(UBUNTU16_FONT, NULL);
				_fg = TFT_WHITE;
				_bg = TFT_GREEN;
				TFT_print("80%", 40, 85);
			}

			if(bat1Condition || bat2Condition || bat3Condition){
				TFT_drawRoundRect(25, 105, 60, 20, 5, TFT_GREEN);
				TFT_fillRoundRect(25, 105, 60, 20, 5, TFT_GREEN);
				TFT_setFont(UBUNTU16_FONT, NULL);
				_fg = TFT_WHITE;
				_bg = TFT_GREEN;
				TFT_print("60%", 40, 110);
			}

			if(bat1Condition || bat2Condition || bat3Condition || bat4Condition){
				TFT_drawRoundRect(25, 130, 60, 20, 5, TFT_GREEN);
				TFT_fillRoundRect(25, 130, 60, 20, 5, TFT_GREEN);
				TFT_setFont(UBUNTU16_FONT, NULL);
				_fg = TFT_WHITE;
				_bg = TFT_GREEN;
				TFT_print("40%", 40, 135);
			}

			if(bat1Condition || bat2Condition || bat3Condition || bat4Condition || bat5Condition){
				color_t temp = TFT_GREEN;
				_fg = TFT_WHITE;
				_bg = TFT_GREEN;
				if(bat5Condition){
					temp = TFT_RED;
					_bg = TFT_RED;
				}
				TFT_drawRoundRect(25, 155, 60, 20, 5, temp);
				TFT_fillRoundRect(25, 155, 60, 20, 5, temp);
				TFT_setFont(UBUNTU16_FONT, NULL);
				TFT_print("20%", 40, 160);
			}

			else if(batteryPercentage>0 && batteryPercentage<10){
				TFT_drawRoundRect(25, 177, 60, 2, 0, TFT_RED);
				TFT_fillRoundRect(25, 177, 60, 2, 0, TFT_RED);
			}
	}

	delay(500);
	}
}