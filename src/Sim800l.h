
#ifndef Sim800 // include para checar se já não foi incluido uma vez
#define Sim800
#include "driver/gpio.h"

typedef struct {
    int signal_strength;
    int bit_errors;
} sim800l_kpis;

typedef struct {
    gpio_num_t sim_rx_pin;  
    gpio_num_t sim_tx_pin;  
    gpio_num_t sim_rst_pin; 
    int baud_rate; 
    uint8_t uart;
} ConfPinsSIM800L;

class Sim800l{
    private:
        ConfPinsSIM800L confPins;
        sim800l_kpis kpis;

    public:
        Sim800l(const ConfPinsSIM800L *config);

        bool setup_sim800l();
        bool init();
        bool waitResponse(int maxlen);
        void sendcmd(const char *cmd, char *resp, int* len, int max_le_resp);
        void update_kpis ();
        void substring(char* original,char* sub,int start,int len);
        int findstr(char* strin,const char* busca, int stringlen, int buscaLen, int pos_inicial);
        int get_signalStrength();
        int get_bitError();
        void getNetwork(char* net, int* len);
        void getCCID(char* resp,int* tam);
        bool isRegistered();
        bool configGPRS(char* APN,int tam);
        bool sendCMDWaitOK(char* to_send);
        bool startGPRS();
        bool endGPRS();
        void getIP(char* resp,int* tam);
        void reset();
        int getURL(char* URL,int tam);
        void waitcmd(char *resp, int *len, int max_le_resp, int timeout);
        void readHttpResponse(char *line,int len, int* len_read);
        void clean(char* line, int tam);
        

};

#endif