#ifndef APP_LWIPOPTS_H
#define APP_LWIPOPTS_H

#define NO_SYS                          0
#define MEM_ALIGNMENT                   4
#define MEM_SIZE                        (20 * 1024)
#define MEMP_NUM_PBUF                   12
#define MEMP_NUM_UDP_PCB                6
#define MEMP_NUM_TCP_PCB                4
#define MEMP_NUM_TCP_PCB_LISTEN         2
#define MEMP_NUM_TCP_SEG                16
#define MEMP_NUM_SYS_TIMEOUT            12
#define PBUF_POOL_SIZE                  12
#define PBUF_POOL_BUFSIZE               1524
#define LWIP_IPV4                       1
#define LWIP_IPV6                       0
#define LWIP_TCP                        1
#define LWIP_UDP                        1
#define LWIP_ICMP                       1
#define LWIP_DHCP                       1
#define LWIP_DNS                        1
#define LWIP_NETCONN                    1
#define LWIP_SOCKET                     0
#define LWIP_NETIF_API                  1
#define LWIP_NETIF_LINK_CALLBACK        1
#define LWIP_STATS                      0

#define CHECKSUM_GEN_IP                 0
#define CHECKSUM_GEN_UDP                0
#define CHECKSUM_GEN_TCP                0
#define CHECKSUM_CHECK_IP               0
#define CHECKSUM_CHECK_UDP              0
#define CHECKSUM_CHECK_TCP              0
#define CHECKSUM_GEN_ICMP               0

#define TCPIP_THREAD_NAME               "TCP/IP"
#define TCPIP_THREAD_STACKSIZE          1024
#define TCPIP_THREAD_PRIO               osPriorityHigh
#define TCPIP_MBOX_SIZE                 8
#define DEFAULT_UDP_RECVMBOX_SIZE       8
#define DEFAULT_TCP_RECVMBOX_SIZE       8
#define DEFAULT_ACCEPTMBOX_SIZE         4
#define DEFAULT_THREAD_STACKSIZE        512

#define LWIP_SNTP                       1
#define SNTP_SERVER_DNS                 1
#define SNTP_UPDATE_DELAY               21600000UL
void APP_SNTP_SetUnixTime(unsigned long seconds);
#define SNTP_SET_SYSTEM_TIME(sec)       APP_SNTP_SetUnixTime((unsigned long)(sec))

#endif
