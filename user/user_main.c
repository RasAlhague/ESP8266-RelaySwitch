#include "user_interface.h"
#include <espconn.h>
#include <mem.h>
#include <gpio.h>
#include <stdlib.h>
#include "osapi.h"

#define DLEVEL 2

char GPIO_URI[] = "/gpio/";
char GPIO_HIGH[] = "/high";
char GPIO_LOW[] = "/low";

typedef struct uri {
    char uri[128];
} URI;

char OK_200[32] = "HTTP/1.1 200 OK";

void process_gprio_uri(URI *);

int ipow(int base, int exp) {
    int result = 1;
    while (exp) {
        if (exp & 1)
            result *= base;
        exp >>= 1;
        base *= base;
    }

    return result;
}

void init_gpios(void) {
    os_printf("Initing GPIO to LOW\n");

    gpio_output_set(0, BIT0, BIT0, 0);
    gpio_output_set(0, BIT2, BIT2, 0);
}

void connect_to_wifi(void) {
    const char ssid[32] = "RA Wi-Fi";
    const char password[32] = "y134679258";

    struct station_config stationConf;

#if DLEVEL > 2
    os_printf("wifi_set_opmode(STATION_MODE)\n");
#endif
    wifi_set_opmode(STATION_MODE);

    os_memcpy(&stationConf.ssid, ssid, 32);
    os_memcpy(&stationConf.password, password, 32);

    wifi_station_set_config(&stationConf);
#if DLEVEL > 2
    os_printf("wifi_station_connect()\n");
#endif
    wifi_station_connect();
}

void http_disconnetcb(void *arg) {
    struct espconn *pespconn = arg;

#if DLEVEL > 2
    os_printf("Disconnecting \n");
#endif

    espconn_disconnect(pespconn);
}

void extract_uri(URI *__dest, char *__response) {
    char *http_str_poiner;
    http_str_poiner = strstr(__response, " HTTP");
#if DLEVEL > 2
    os_printf("Pointer to HTTP str = %s\n\n", http_str_poiner);
#endif

    char *slash_str_poiner;
    slash_str_poiner = strstr(__response, "/");
#if DLEVEL > 2
    os_printf("Pointer to / str = %s\n", slash_str_poiner);
#endif

    size_t p_url_end = (size_t) (http_str_poiner - slash_str_poiner);
    strncpy(__dest->uri, slash_str_poiner, p_url_end);
    __dest->uri[p_url_end] = '\0';
#if DLEVEL > 2
    os_printf("uri = %s\n", __dest->uri);
#endif
}

void http_recvcb(void *arg,
                 char *pdata,
                 unsigned short len) {
    struct espconn *pespconn = arg;
    char response[len];
    URI uri;

    os_memcpy(response, pdata, len);

    extract_uri(&uri, response);

    if (strncmp(uri.uri, GPIO_URI, strlen(GPIO_URI)) == 0) {
        process_gprio_uri(&uri);
    }

    espconn_regist_sentcb(pespconn, http_disconnetcb);
//    espconn_send(pespconn, uri, strlen(uri));
    espconn_send(pespconn, OK_200, strlen(OK_200));
}

void process_gprio_uri(URI *uri) {
    long int gpio_pin;
    uint32 pin_addr;
    gpio_pin = strtol(uri->uri + strlen(GPIO_URI), NULL, 10);
    pin_addr = (uint32) ((ipow(2, gpio_pin % 4)) * ipow(10, gpio_pin / 4));

#if DLEVEL > 2
    os_printf("gpio_pin = %d\n", gpio_pin);
    os_printf("pin_addr = %d\n", pin_addr);
#endif

    if (strstr(uri->uri, GPIO_LOW)) {
        os_printf("Setting pin to LOW with address = %d; URL = %s\n", pin_addr, uri->uri);
        gpio_output_set(0, pin_addr, pin_addr, 0);
    } else if (strstr(uri->uri, GPIO_HIGH)) {
        os_printf("Setting pin to HIGH with address = %d; URL = %s\n", pin_addr, uri->uri);
        gpio_output_set(pin_addr, 0, pin_addr, 0);
    }
}

//This function gets called whenever
void ICACHE_FLASH_ATTR server_connectcb(void *arg) {
    struct espconn *pespconn = (struct espconn *) arg;

#if DLEVEL > 2
    os_printf("server_connectcb\n");
#endif

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
#if DLEVEL > 2
    os_printf("Start listening\n");
#endif
    espconn_accept(pHTTPServer);

    //I don't know what default is, but I always set this.
    espconn_regist_time(pHTTPServer, 2, 0);
}

void user_init(void) {
    uart_div_modify(0, UART_CLK_FREQ / 115200);

    os_printf("SDK version: %s\n", system_get_sdk_version());

    init_gpios();
    connect_to_wifi();
    start_http_server();
}
