#ifndef LCD // include guards to avoid multiple inclusions
#define LCD
#include "driver/gpio.h"




typedef struct {
    gpio_num_t lcd_rs_pin;  
    gpio_num_t lcd_en_pin;  
    gpio_num_t lcd_dt4_pin;  
    gpio_num_t lcd_dt5_pin;  
    gpio_num_t lcd_dt6_pin;  
    gpio_num_t lcd_dt7_pin;
} ConfPinsLCD;

typedef struct {
    uint8_t lin;  
    uint8_t col; 
} Cursor;

class Lcd {
private:
    gpio_num_t data_pin[4];
    gpio_num_t crt_pin[2];
    Cursor cursor;


public:
    Lcd(const ConfPinsLCD *confPins);

    void print_tag(const char * tag, int qtde, uint8_t data);
    void write_lcd_nibble(uint8_t data);
    void write_lcd_crt(uint8_t crt);
    void write_EN(bool state);
    void write_RS(bool state);
    void setup_lcd();
    void write_lcd_cmd(uint8_t data);
    void write_lcd_data(uint8_t data);
    void clear_lcd();
    void home_lcd();
    void blink_cursor(bool state);
    void set_cursor(uint8_t lin, uint8_t col);
    Cursor get_cursor();
    void incr_cursor();
    void decr_cursor();
    void write_char_array(char *texto);
    void write_char_array(const char *texto);
};

#endif // end of include guards