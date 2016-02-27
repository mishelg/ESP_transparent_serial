#include "ets_sys.h"
#include "osapi.h"
#include "gpio.h"
#include "os_type.h"
#include "user_config.h"
#include "user_interface.h"
#include "uart.h"
#include "gpio.h"
#include "c_types.h"
#include "espconn.h"
#include "mem.h"
#define UART0   0
#define user_procTaskPrio        0
#define user_procTaskQueueLen    1
extern UartDevice UartDev;
struct espconn conn;
os_event_t    user_procTaskQueue[user_procTaskQueueLen];
static void loop(os_event_t *events);

//Main code function
static void ICACHE_FLASH_ATTR
loop(os_event_t *events)
{
    os_delay_us(10000);
    system_os_post(user_procTaskPrio, 0, 0 );
}

static void ICACHE_FLASH_ATTR networkConnectedCb(void *arg);
static void ICACHE_FLASH_ATTR networkDisconCb(void *arg);
static void ICACHE_FLASH_ATTR networkReconCb(void *arg, sint8 err);
static void ICACHE_FLASH_ATTR networkRecvCb(void *arg, char *data, unsigned short len);
static void ICACHE_FLASH_ATTR networkSentCb(void *arg);
void ICACHE_FLASH_ATTR network_init();

LOCAL os_timer_t network_timer;
LOCAL os_timer_t read_timer;

static void ICACHE_FLASH_ATTR networkSentCb(void *arg) {
}
static void ICACHE_FLASH_ATTR networkRecvCb(void *arg, char *data, unsigned short len) {
  struct espconn *conn=(struct espconn *)arg;
  if(data[0]=='R' && data[1]=='S'&& data[2]=='T' && data[3]=='!' ){
	gpio_output_set(0, BIT2, BIT2, 0);
	os_delay_us(10);
	gpio_output_set(BIT2, 0, BIT2, 0);
  }
  uart0_tx_buffer(data,len);
}

static void ICACHE_FLASH_ATTR networkConnectedCb(void *arg) {
  struct espconn *conn=(struct espconn *)arg;
  espconn_regist_recvcb(conn, networkRecvCb);
}

static void ICACHE_FLASH_ATTR networkReconCb(void *arg, sint8 err) {
  network_init();
}

static void ICACHE_FLASH_ATTR networkDisconCb(void *arg) {
	network_init();
}


void ICACHE_FLASH_ATTR network_start() {
  static struct espconn *conn_pointer = &conn;
  static ip_addr_t ip;
  static ip_addr_t *ip_pointer = &ip;
  static esp_tcp tcp;
 
  IP4_ADDR(&ip, (uint8) IP1, (uint8) IP2, (uint8) IP3, (uint8) IP4);
  
  conn.type=ESPCONN_TCP;
  conn.state=ESPCONN_NONE;
  conn.proto.tcp=&tcp;
  conn.proto.tcp->local_port=espconn_port();
  conn.proto.tcp->remote_port=PORT_NUMBER;
  os_memcpy(conn_pointer->proto.tcp->remote_ip, &ip_pointer->addr, 4);
  espconn_regist_connectcb(&conn, networkConnectedCb);
  espconn_regist_disconcb(&conn, networkDisconCb);
  espconn_regist_reconcb(&conn, networkReconCb);
  espconn_regist_recvcb(&conn, networkRecvCb);
  espconn_connect(&conn);
}
void ICACHE_FLASH_ATTR read_buffer(void) {
  struct espconn *conn_pointer = (struct espconn*) &conn;
  uint8* temp;
  RcvMsgBuff *pRxBuff = &(UartDev.rcv_buff);
  temp = (pRxBuff->pRcvMsgBuff);
  espconn_sent(conn_pointer,temp,pRxBuff->pWritePos-temp);
  pRxBuff->pWritePos = pRxBuff->pRcvMsgBuff;
  pRxBuff->BuffState = EMPTY;
 }

void ICACHE_FLASH_ATTR network_check_ip(void) {
  struct ip_info ipconfig;
  os_timer_disarm(&network_timer);
  wifi_get_ip_info(STATION_IF, &ipconfig);
  if (wifi_station_get_connect_status() == STATION_GOT_IP && ipconfig.ip.addr != 0) {
    network_start();
  } else {
    os_printf("No ip found\n\r");
    os_timer_setfn(&network_timer, (os_timer_func_t *)network_check_ip, NULL);
    os_timer_arm(&network_timer, 1000, 0);
  }
}

void ICACHE_FLASH_ATTR rx_buffer_check(void* arg) {
  os_timer_disarm(&read_timer);
  if (UartDev.rcv_buff.BuffState != EMPTY) {
    read_buffer();
    os_timer_setfn(&read_timer, (os_timer_func_t *)rx_buffer_check, NULL);
    os_timer_arm(&read_timer, 25, 0);
    } else {
    os_timer_setfn(&read_timer, (os_timer_func_t *)rx_buffer_check, NULL);
    os_timer_arm(&read_timer, 25, 0);}
}

void ICACHE_FLASH_ATTR network_init() {
  os_timer_disarm(&network_timer);
  os_timer_setfn(&network_timer, (os_timer_func_t *)network_check_ip, NULL);
  os_timer_arm(&network_timer, 1000, 0);
}


void ICACHE_FLASH_ATTR receive_buffer_init() {
  os_timer_disarm(&read_timer);
  os_timer_setfn(&read_timer, (os_timer_func_t *)rx_buffer_check, NULL);
  os_timer_arm(&read_timer, 25, 0);
}
//Init function 
void ICACHE_FLASH_ATTR user_init() {

    uart_init(BIT_RATE_115200,BIT_RATE_115200);
    gpio_init();
    PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO2_U, FUNC_GPIO2);
	gpio_output_set(BIT2, 0, BIT2, 0);
    char ssid[32] = SSID;
    char password[64] = SSID_PASSWORD;
    struct station_config stationConf;

    //Set station mode
    wifi_set_opmode( 0x1 );

    //Set ap settings
    os_memcpy(&stationConf.ssid, ssid, 32);
    os_memcpy(&stationConf.password, password, 64);
    wifi_station_set_config(&stationConf);

    //Start os task
    system_os_task(loop, user_procTaskPrio,user_procTaskQueue, user_procTaskQueueLen);

    system_os_post(user_procTaskPrio, 0, 0 );

    network_init();
    
    receive_buffer_init();
    
    uart0_tx_buffer("Hi Mishel!\n",10);
}
