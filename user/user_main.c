#include "user_interface.h"
#include <espconn.h>
#include <mem.h>
#include <gpio.h>
#include "osapi.h"

void init_gpios(void) {
    os_printf("Initing GPIO to LOW\n");

    gpio_output_set(0, BIT0, BIT0, 0);
    gpio_output_set(0, BIT2, BIT2, 0);
}

void connect_to_wifi(void) {
    const char ssid[32] = "RA Wi-Fi";
    const char password[32] = "y134679258";

    struct station_config stationConf;

    os_printf("wifi_set_opmode(STATION_MODE)\n");
    wifi_set_opmode(STATION_MODE);

    os_memcpy(&stationConf.ssid, ssid, 32);
    os_memcpy(&stationConf.password, password, 32);

    wifi_station_set_config(&stationConf);
    os_printf("wifi_station_connect()\n");
    wifi_station_connect();
}

void http_disconnetcb(void *arg) {
    struct espconn *pespconn = arg;

    os_printf("Disconnecting \n");
//    os_printf(pespconn->proto.tcp->remote_ip);
//    os_printf("\n");

    espconn_disconnect(pespconn);
}

void http_recvcb(void *arg,
                 char *pdata,
                 unsigned short len) {
    struct espconn *pespconn = arg;
    char response[len];
    char uri[64];

    os_memcpy(response, pdata, len);


    char *http_str_poiner;
    http_str_poiner = strstr(response, " HTTP");
//    os_printf("Pointer to HTTP str = %s\n\n", http_str_poiner);

    char *slash_str_poiner;
    slash_str_poiner = strstr(response, "/");
//    os_printf("Pointer to / str = %s\n", slash_str_poiner);

    size_t p_url_end = (size_t) (http_str_poiner - slash_str_poiner);
    strncpy(uri, slash_str_poiner, p_url_end);
    uri[p_url_end] = '\0';
//    os_printf("uri = %s\n", uri);


    espconn_regist_sentcb(pespconn, http_disconnetcb);
    espconn_send(pespconn, uri, strlen(uri));
}

//This function gets called whenever
void ICACHE_FLASH_ATTR server_connectcb(void *arg) {
    struct espconn *pespconn = (struct espconn *) arg;

    os_printf("server_connectcb\n");

    //Let's register a few callbacks, for when data is received or a disconnect happens.
    espconn_regist_recvcb(pespconn, http_recvcb);
    espconn_regist_disconcb(pespconn, http_disconnetcb);
}

void start_http_server(void) {
    //Allocate an "espconn"
    struct espconn *pHTTPServer = (struct espconn *) os_zalloc(sizeof(struct espconn));
    os_memset(pHTTPServer, 0, sizeof(struct espconn));

    //Initialize the ESPConn
    espconn_create(pHTTPServer);
    pHTTPServer->type = ESPCONN_TCP;
    pHTTPServer->state = ESPCONN_NONE;

    //Make it a TCP conention.
    pHTTPServer->proto.tcp = (esp_tcp *) os_zalloc(sizeof(esp_tcp));
    pHTTPServer->proto.tcp->local_port = 80;

    //"httpserver_connectcb" gets called whenever you get an incoming connetion.
    espconn_regist_connectcb(pHTTPServer, server_connectcb);

    //Start listening!
    os_printf("Start listening\n");
    espconn_accept(pHTTPServer);

    //I don't know what default is, but I always set this.
    espconn_regist_time(pHTTPServer, 2, 0);
}

void user_init(void) {
    uart_div_modify(0, UART_CLK_FREQ / 115200);

    os_printf("SDK version:%s\n", system_get_sdk_version());

    init_gpios();
    connect_to_wifi();
    start_http_server();
}
