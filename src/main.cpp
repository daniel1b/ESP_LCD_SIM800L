#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/uart.h"
#include "rom/ets_sys.h"
#include "Sim800l.h"
#include "Lcd.h"


#define BAUD_RATE_MODEM 57600
#define BAUD_RATE_PC 57600

//cria o objeto da comunicação com o modem
ConfPinsSIM800L conf_sim800l = {    
    .sim_rx_pin = GPIO_NUM_4,
    .sim_tx_pin = GPIO_NUM_2,
    .sim_rst_pin = GPIO_NUM_15,
    .baud_rate = BAUD_RATE_MODEM,
    .uart = UART_NUM_2
};
Sim800l modem = Sim800l(&conf_sim800l);

//cria o objeto da comunicação com o LCD
ConfPinsLCD conf = {
    .lcd_rs_pin = GPIO_NUM_23,
    .lcd_en_pin = GPIO_NUM_22,
    .lcd_dt4_pin = GPIO_NUM_21,
    .lcd_dt5_pin = GPIO_NUM_19,
    .lcd_dt6_pin = GPIO_NUM_18,
    .lcd_dt7_pin = GPIO_NUM_5
};
Lcd lcd_obj = Lcd(&conf);


extern "C" void app_main() {
    //Uart que comunica com o computador
    uart_set_baudrate(UART_NUM_0, BAUD_RATE_PC);
    printf("uart pronta..\n");

    //inicializa LCD
    lcd_obj.setup_lcd();
    lcd_obj.clear_lcd();
    lcd_obj.blink_cursor(true);
    printf("LCD pronto..\n");

    int i = 0;

    lcd_obj.write_char_array("init modem...");

    //inicializa modem, essa func retorna um bool falando se deu certo ou não
    modem.reset(); //apagar depois é para fins de debug
    modem.setup_sim800l();
    printf("SIM800l pronto..\n");
    modem.update_kpis();

    //configurar o gprs na rede claro
    char APN[]= "claro.com.br";
    modem.configGPRS(APN,sizeof(APN));
    //ligar o GPRS, vai gastar mais energia, desligar depois que terminar modem.endGPRS();
    modem.startGPRS();



    lcd_obj.clear_lcd();
    lcd_obj.blink_cursor(false);

    printf("loop:\n");
    while(1) {    
        char arr[40];

        //modem.getCCID();
        
        sprintf(arr,"Sig:%d%% Err:%d ",modem.get_signalStrength(),modem.get_bitError());
        lcd_obj.set_cursor(0,0);
        lcd_obj.write_char_array(arr);

        //int len;
        //char net[25];
        //modem.getNetwork(net, &len);
        //modem.getCCID (net, &len);
        //sprintf(arr,"%s",net);



        //int len;
        //char net[25];
        //modem.getIP(net, &len);
        //modem.getCCID (net, &len);
        //sprintf(arr,"%s",net);



        char URL[]= "http://exploreembedded.com/wiki/images/1/15/Hello.txt";
        int tamanho = modem.getURL(URL,sizeof(URL));   



        char line[15];
        for(int i = 0; i<=tamanho/sizeof(line);i++){
            //encher o array line de dados, com os proximos x bytes
            int len_read;
            modem.readHttpResponse(line,sizeof(line),&len_read);
            modem.clean(line,len_read);
            //imprimir os dados no lcd
            lcd_obj.set_cursor(1,0);
            lcd_obj.write_char_array("                  ");
            lcd_obj.set_cursor(1,0);
            lcd_obj.write_char_array(line);
            printf("t: %d/%d; r: %s\n",i,tamanho, line);
            //esperar um segundo
            vTaskDelay(pdMS_TO_TICKS(1000));
        }
        //sprintf(arr,"t: %d", tamanho);
        //printf(arr);


        



        vTaskDelay(pdMS_TO_TICKS(5000));
        i++;
    }
}

