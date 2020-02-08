#ifndef _RCAN_HS_BUS_H
#define _RCAN_HS_BUS_H


#include "rcan_config.h"







//void can_bus_add_node_list(rcan_node* first node);

void rcan_hs_bus_transmit_next(struct can_bus* bus);

void can_bus_activate_all();
void can_bus_loop_all();
void can_bus_reset(struct can_bus *bus);

#endif