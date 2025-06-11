
#pragma once
#include <stdbool.h>

/******************************************************************************
 Definitions
 ******************************************************************************/

#define NETWORK_DEVICE_ID_NO_DEVICE 0

/******************************************************************************
 API
 ******************************************************************************/

bool network_device_exists(unsigned device_id);

int network_device_get_fd(unsigned device_id);

int network_device_add(unsigned device_id, int fd);

int network_device_enable_handling(unsigned device_id);

int network_device_disable_handling(unsigned device_id);

int network_device_remove(unsigned device_id);

int network_device_notify_reconfig();

int network_device_set_command_error_handler(
    unsigned device_id, void (*handler)(unsigned, unsigned char, int));

int network_device_set_net_error_handler(unsigned device_id,
                                         void (*handler)(unsigned, int));

int network_device_set_handler(int device_id, unsigned char command,
                               int (*handler)(unsigned));