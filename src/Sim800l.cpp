#include "Sim800l.h"
#include "driver/uart.h"
#include <string.h>



/*
Considerando um buffer de 1bytes e uma comunicação de 57600bps
tempo max para encher o buffer = 1 / (57600 / 10) ~ 2us
ou seja para cada char recebido pode demoarar no max 1ms, com margem de segurança 500%
*/
#define TIMEOUT_BYTE 1 //1ms é o suficiente para receber 1byte

Sim800l::Sim800l(const ConfPinsSIM800L *config){ //construtor da classe que aramzena os dados dos pinos onde está ligado o modem
    //inicializar configuraçoes
    confPins.sim_rx_pin = config->sim_rx_pin;
    confPins.sim_tx_pin = config->sim_tx_pin;
    confPins.sim_rst_pin = config->sim_rst_pin ;
    confPins.baud_rate = config->baud_rate;
    confPins.uart = config->uart;

    //inicializar kpis
    kpis.signal_strength = -1;
    kpis.bit_errors = -1;
}


bool Sim800l::setup_sim800l(){
    //configura o pino de reset do modem
    gpio_set_direction(confPins.sim_rst_pin, GPIO_MODE_OUTPUT);
    //deixar o pino sempre em high, pq low reseta o modem
    gpio_set_level(confPins.sim_rst_pin, 1);


    //configura uart    
    uart_config_t uart_config = {
        .baud_rate = confPins.baud_rate,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .rx_flow_ctrl_thresh = 122,
        .source_clk = UART_SCLK_APB
    };

    uart_param_config(confPins.uart, &uart_config);
    uart_set_pin(confPins.uart, confPins.sim_tx_pin, confPins.sim_rx_pin, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    uart_driver_install(confPins.uart, 1024, 1024, 0, NULL, 0);  
    
    //inicializa o modem
    return init();
}


bool Sim800l::init(){
    //segurar reset por 350ms em low, depois volta para high
    gpio_set_level(confPins.sim_rst_pin, 0);
    vTaskDelay(350 / portTICK_PERIOD_MS);
    gpio_set_level(confPins.sim_rst_pin, 1);

    //aguardar o modulo iniciar
    vTaskDelay(5000 / portTICK_PERIOD_MS);

    //esvaziar o buffer antes de enviar um comando
    uart_flush_input(confPins.uart);

    //enviar o handshake
    const char* to_send = "AT\r\n";
    uart_write_bytes(confPins.uart, to_send, strlen(to_send));
    bool resp = waitResponse(10);
    if (!resp) {
        for(int i = 0; i<5; i++){
            uart_flush_input(confPins.uart);
            uart_write_bytes(confPins.uart, to_send, strlen(to_send));
            resp = waitResponse(10);    
            if(resp){
                break;
            }
        }
    }
    char dt [1];
    int len_resp;
    sendcmd("AT&FE0\r\n", dt, &len_resp, 10); //enviar comando para desativar o echo
    uart_flush_input(confPins.uart);
    return resp;
}

bool Sim800l::waitResponse(int maxlen){
    char data[maxlen]; // buffer para receber os dados
    int len = uart_read_bytes(confPins.uart, &data, sizeof(data), maxlen*TIMEOUT_BYTE / portTICK_PERIOD_MS);

    if (len > 0) {//se chegou dados, processar...
        data[len] = '\0';//marcar o fim da string recebida

        // checar qual resposta foi recebida
        if (strstr(data, "OK") != nullptr) {
            return true;
        } else if (strstr(data, "ERRO") != nullptr) {
            return false;
        }
    }
    
    return false;
}

void Sim800l::sendcmd(const char *cmd, char *resp, int *len, int max_le_resp){
        //esvaziar o buffer antes de enviar um comando
        uart_flush_input(confPins.uart);
        uart_write_bytes(confPins.uart, cmd, strlen(cmd));

        int tam = uart_read_bytes(confPins.uart, resp,max_le_resp, (max_le_resp*TIMEOUT_BYTE) / portTICK_PERIOD_MS);
        resp[tam] = '\0';
        *len = tam;
}

bool Sim800l::sendCMDWaitOK(char* to_send){
    uart_write_bytes(confPins.uart, to_send, strlen(to_send));
    bool resp = waitResponse(40);
    return resp;
}

void Sim800l::getNetwork(char* net, int* len){
    char temp [20];
    int resp_len;
    sendcmd("AT+COPS?\r\n", temp, &resp_len, 20);
    //+COPS: 0,0,"
    
    int start = findstr(temp, "+COPS: 0,0,\"", resp_len, 12, 0)+12;
    int end = findstr(temp, "\"", resp_len, 1, start);

    *len = end - start;

    substring(temp,net,start,*len);
}

void Sim800l::getCCID(char* resp,int* tam){
    char dt [30];
    int len_resp;
    sendcmd("AT+CCID\r\n", dt, &len_resp, 30); //solicitar dados
    
    int start = 2;
    int end = findstr(dt, "OK", len_resp, 1, start)-4; 

    int len = end - start;
    *tam = len;

    substring(dt,resp,start,len);

}



void Sim800l::update_kpis (){
    /* força do sinal e o bit_error
    +CSQ: <rssi>,<ber>
    where <rssi> is the Received Signal Strength Indication (RSSI) value, and <ber> is the Bit Error Rate (BER) value.
    */
    char dt [20];
    int len_resp;
    sendcmd("AT+CSQ\r\n", dt, &len_resp, 20); //solicitar dados
    //char dt [] = "+CSQ: 17,15\n"; //linha de teste das funçoes abaixo, simula uma resposta do modulo
    //find strenght in dt array
    int start = findstr(dt, "+CSQ: ", len_resp, 6, 0)+6;
    int end = findstr(dt, ",", len_resp, 1, start);

    int len = end - start;

    char stren[len];

    substring(dt,stren,start,len);

    kpis.signal_strength = atoi(stren)*100/31;


    //find error bit in dt array
    start = end+1;
    end = findstr(dt, "\n", sizeof(dt),1, start);
    len = end - start;
    char error[len];
    substring(dt,error,start,len);
    kpis.bit_errors = atoi(error);

}

bool Sim800l::isRegistered(){
    char dt [15];
    int len_resp;
    sendcmd("AT+CREG?\r\n", dt, &len_resp, 15); //solicitar dados
    
    int start = findstr(dt, "+CREG: ", len_resp, 7, 0)+7;
    int end = start + 1;

    int len = end - start;

    char sub[5];
    substring(dt,sub,start,len);
    int response = atoi(sub);


    return response == 1;
}

bool Sim800l::configGPRS(char* APN,int tam){
    char dt [50];
    sprintf(dt,"AT+SAPBR=3,1,\"Contype\",\"GPRS\"\r\n");
    bool CON_r =  sendCMDWaitOK(dt); //enviar cmd  
    vTaskDelay(pdMS_TO_TICKS(200));

    sprintf(dt,"AT+SAPBR=3,1,\"APN\",\"%s\"\r\n",APN);
    bool APN_r =  sendCMDWaitOK(dt); //enviar cmd
    vTaskDelay(pdMS_TO_TICKS(200));  

    sprintf(dt,"AT+SAPBR=1,1\r\n");
    bool SRT_r =  sendCMDWaitOK(dt); //enviar cmd 
    vTaskDelay(pdMS_TO_TICKS(200)); 

    return CON_r & APN_r & SRT_r;
}

int Sim800l::getURL(char* URL,int tam){
    char dt [50];
    sprintf(dt,"AT+HTTPINIT\r\n");
    sendCMDWaitOK(dt); //enviar cmd  
    vTaskDelay(pdMS_TO_TICKS(200)); 

    sprintf(dt,"AT+HTTPPARA=\"URL\",\"%s\"\r\n",URL);
    sendCMDWaitOK(dt); //enviar cmd 
    vTaskDelay(pdMS_TO_TICKS(200)); 

    sprintf(dt,"AT+HTTPACTION=0\r\n");
    sendCMDWaitOK(dt); //enviar cmd  
    vTaskDelay(pdMS_TO_TICKS(200));

    int response = -1;

    //int len_resp;
    //waitcmd(dt, &len_resp, 30, 3000);

    int len_header = 5;
    char header[len_header];

    //esperar receber o proximo comando, pode demorar muito, então esperar somente o "+HTTP"
    //o resto da informação pegar no proxima leitra com timeout menor
    int tam_top = uart_read_bytes(confPins.uart, header,len_header, 10000 / portTICK_PERIOD_MS);
    header[tam_top] = '\0';

    //if(!(findstr(header, "+HTTP", tam_top, 5, 0)>0)){
    //    return -2;
    //}

    int len_data = 30;
    char data[len_data];  
    int tam_data = uart_read_bytes(confPins.uart, data,len_data, 300 / portTICK_PERIOD_MS);
    
    int start = findstr(data, ",", tam_data, 1, 0)+1;
    start = findstr(data, ",", tam_data, 1, start)+1;//pegar segunda virgula
    int end = findstr(data, "\n", tam_data, 1, start);

    int len = end - start;

    char sub[10];
    substring(data,sub,start,len);

    response = atoi(sub);
    response = response+17+response/10;

    //enviar o comando pra fazer o dump do que chegou no modem
    sprintf(dt,"AT+HTTPREAD\r\n");
    uart_write_bytes(confPins.uart, dt, strlen(dt));
    uart_read_bytes(confPins.uart, dt,1, 3000/ portTICK_PERIOD_MS);
    return response;
}

void Sim800l::readHttpResponse(char *line,int len, int* len_read){
    int tam = uart_read_bytes(confPins.uart, line,len, len*TIMEOUT_BYTE / portTICK_PERIOD_MS);
    line[tam] = '\0';
    *len_read = tam;
    
}

bool Sim800l::startGPRS(){
    char dt [50];
    int len_resp;
    sendcmd("AT+SAPBR=2,1\r\n", dt, &len_resp, 50); //solicitar dados
    
    int start = findstr(dt, ",", len_resp, 1, 0)+1;
    int end = start + 1;

    int len = end - start;

    char sub[5];
    substring(dt,sub,start,len);
    int response = atoi(sub);


    return response == 1;
}
bool Sim800l::endGPRS(){//importante desligar pra economizar energia
    char dt [50];
    sprintf(dt,"AT+SAPBR=0,1\r\n");
    return sendCMDWaitOK(dt); //enviar cmd  
}
void Sim800l::reset(){
    char dt [50];
    sprintf(dt,"AT+CFUN=1,1\r\n");
    vTaskDelay(500 / portTICK_PERIOD_MS);
    //uart_flush_input(confPins.uart);
}
void Sim800l::getIP(char* resp,int* tam){
    char dt [50];
    int len_resp;
    sendcmd("AT+SAPBR=2,1\r\n", dt, &len_resp, 50); //solicitar dados
    
    int start = findstr(dt, ",\"", len_resp, 2, 0)+2;
    int end = findstr(dt, "\"", len_resp, 1, start+1);

    int len = end - start;
    *tam = len;

    substring(dt,resp,start,len);
}

void Sim800l::waitcmd(char *resp, int *len, int max_le_resp, int timeout){
        //esvaziar o buffer antes de enviar um comando
        //uart_flush_input(confPins.uart);
        //uart_write_bytes(confPins.uart, cmd, strlen(cmd));

        int tam = uart_read_bytes(confPins.uart, resp,max_le_resp, timeout / portTICK_PERIOD_MS);
        resp[tam] = '\0';
        *len = tam;
}


int Sim800l::get_signalStrength(){
    return kpis.signal_strength;
}
int Sim800l::get_bitError(){
    return kpis.bit_errors;
}

void Sim800l::substring(char* original,char* sub,int start,int len){
    for(int i = 0; i <len; i++){
        sub[i] = original[start+i];
    }
    sub[len]='\0'; //apontar o fim do array
}

int Sim800l::findstr(char* strin,const char* busca, int stringlen, int buscaLen, int pos_inicial){

    //se a string de busca for maior que a string original, já vai dar errado
    if (buscaLen > (stringlen-pos_inicial)) {
        return -1;
    }

    // Iterar a string original em busca do match (não precisa ir até o final do array)
    for (int i = pos_inicial; i <= stringlen - buscaLen; i++) {
        // comparar a substring atual com a string de busca
        int result = strncmp(strin + i, busca, buscaLen);
        if ( result == 0) {
            return i;
        }
    }

    // If the needle wasn't found, return false
    return -1; 
}

void Sim800l::clean(char* line, int tam){
    for (int i = 0; i < tam; i++) {
        if(line[i] == '\r' || line[i] == '\n')
        line[i] = ' ';
    }
}