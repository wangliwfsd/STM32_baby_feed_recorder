#ifndef APP_ETHERNETIF_H
#define APP_ETHERNETIF_H

#include "lwip/err.h"
#include "lwip/netif.h"
#include "cmsis_os.h"

err_t ethernetif_init(struct netif *netif);
void ethernet_link_thread(void const *argument);

#endif
