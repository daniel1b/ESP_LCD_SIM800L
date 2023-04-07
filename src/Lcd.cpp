#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "rom/ets_sys.h"
#include "Lcd.h"

#define delay_EN 1 //em us, para processar os comando EN e RS
#define delay_DT 10 //em ms, para processar os dados enviados 


Lcd::Lcd(const ConfPinsLCD *confPins) { // constructor that takes an integer parameter
    crt_pin[0]=confPins->lcd_rs_pin;
    crt_pin[1]=confPins->lcd_en_pin;
    data_pin[0]=confPins->lcd_dt4_pin;
    data_pin[1]=confPins->lcd_dt5_pin;
    data_pin[2]=confPins->lcd_dt6_pin;
    data_pin[3]=confPins->lcd_dt7_pin;

    cursor.col = 0;
    cursor.lin = 0;
}


void Lcd::print_tag(const char * tag, int qtde, uint8_t data){
    printf("%s: 0b",tag);
    for (int i = qtde-1; i >= 0; i--) {
        printf("%d",(data >> i) & 0b0001);
    }
    printf("\n");
}
void Lcd::write_lcd_nibble(uint8_t data){
    for (int i = 4-1; i >= 0; i--) {
        gpio_set_level(data_pin[i], (data >> i) & 0b0001 );
    }
 }

void Lcd::write_lcd_crt(uint8_t crt){
    for (int i = 2-1; i >= 0; i--) {
        gpio_set_level(crt_pin[i], (crt >> i) & 0b01 );
    }    
    ets_delay_us(delay_EN);
}
void Lcd::write_EN(bool state){
    gpio_set_level(crt_pin[1], state );
    ets_delay_us(delay_EN);
    }
void Lcd::write_RS(bool state){
    gpio_set_level(crt_pin[0], state );
    ets_delay_us(delay_EN);
}

void Lcd::write_lcd_cmd(uint8_t data){
    //to indicate command
    write_RS(false);

    //write the high nibble
    write_lcd_nibble((data >> 4) & 0b00001111);    
    write_EN(true);
    write_EN(false);

    //write the low nibble
    write_lcd_nibble(data & 0b00001111);
    write_EN(true);
    write_EN(false);

    //back RS to low
    write_RS(false);

    //wait lcd procces command
    vTaskDelay(pdMS_TO_TICKS(delay_DT)); //possivel ponto de problema
}

void Lcd::write_lcd_data(uint8_t data){
    //to indicate data
    write_RS(true);
    //write the high nibble
    write_lcd_nibble((data >> 4) & 0b00001111); 
    write_EN(true);
    write_EN(false);

    //write the low nibble
    write_lcd_nibble(data & 0b00001111);
    write_EN(true);
    write_EN(false);

    //back RS to low
    write_RS(false);

    //wait lcd procces data
    vTaskDelay(pdMS_TO_TICKS(delay_DT)); //possivel ponto de problema
}


void Lcd::setup_lcd(){
    
    for (int i = 0; i < 4; i++) {
        gpio_set_direction(data_pin[i], GPIO_MODE_OUTPUT);
    }
    for (int i = 0; i < 2; i++) {
        gpio_set_direction(crt_pin[i], GPIO_MODE_OUTPUT);
    }
    write_lcd_nibble(0b0000);
    write_lcd_crt(0b00);
    vTaskDelay(pdMS_TO_TICKS(150));

    write_lcd_cmd(0x02);//modo de 4 bits
    write_lcd_cmd(0x28); //modo de duas linhas

    home_lcd();
}


void Lcd::write_char_array(const char *texto){
    int i = 0;
    while (texto[i] != '\0') {
        write_lcd_data(texto[i]);
        i++;
    }
}
void Lcd::write_char_array(char *texto){
    int i = 0;
    while (texto[i] != '\0') {
        write_lcd_data(texto[i]);
        i++;
    }
}



void Lcd::clear_lcd(){    
    write_lcd_cmd(0b00000001);
}
void Lcd::home_lcd(){    
    write_lcd_cmd(0b00000010);
}
void Lcd::blink_cursor(bool state){    
    //0b00001DCB
    //D: 1->diplay on, 0->diplay off
    //C: 1->crusor on, 0->crusor off
    //B: 1->blink on, 0->blink off
    if (state){
        write_lcd_cmd(0b00001111);
    }else{
        write_lcd_cmd(0b00001100);
    }
}


void Lcd::set_cursor(uint8_t lin, uint8_t col){
    home_lcd();
    for(int i = 0; i<lin; i++){
        for(int j = 0; j<40; j++){
            incr_cursor();
        }
    }
    for(int i = 0; i<col; i++){
        incr_cursor();
    }


    cursor.col = col;
    cursor.lin = lin;
}

Cursor Lcd::get_cursor(){
    return cursor; 
}

void Lcd::incr_cursor(){
    write_lcd_cmd(0b00010100);
}
void Lcd::decr_cursor(){
    write_lcd_cmd(0b00010000);
}

