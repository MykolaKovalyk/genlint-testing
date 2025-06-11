
#include "network_events.h"
#include "poll.h"
#include "sys/socket.h"
#include "zephyr/kernel.h"
#include "zephyr/logging/log.h"
#include <errno.h>
#include <stddef.h>
#include <string.h>
#include <sys/_types.h>

/******************************************************************************
 Definitions
 ******************************************************************************/

#define MAX_TRACKED_DEVICES 10
#define COMMAND_HANDLER_COUNT (UCHAR_MAX + 1)

#define POLLFD_RECONF_SOCK_IDX 0

#define UNIX_CMD_RECONFIG ((char)'\r')
#define UNIX_CMD_ACK ((char)'\a')

#define EVENT_HANDLER_POLL_TIMEOUT -1

/******************************************************************************
 Configuration
 ******************************************************************************/

LOG_MODULE_REGISTER(network_events);

/******************************************************************************
 Structures
 ******************************************************************************/

struct network_device {
  unsigned device_id;
  int fd;
  bool handled;
  int (*command_handlers[COMMAND_HANDLER_COUNT])(unsigned);
  void (*net_error_handler)(unsigned device_id, int error_code);
  void (*command_error_handler)(unsigned device_id, unsigned char command,
                                int error_code);
};

/******************************************************************************
 Data
 ******************************************************************************/

static int config_socket;

static struct network_device tracked_devices[MAX_TRACKED_DEVICES];

/******************************************************************************
 Utils
 ******************************************************************************/

static struct network_device *find_free_place() {
  for (unsigned i = 0; i < MAX_TRACKED_DEVICES; i++) {
    struct network_device *place = &tracked_devices[i];

    if (place->device_id == NETWORK_DEVICE_ID_NO_DEVICE) {
      return place;
    }
  }

  return NULL;
}

static struct network_device *network_device_find_by_id(unsigned device_id) {
  for (unsigned i = 0; i < MAX_TRACKED_DEVICES; i++) {
    struct network_device *dev = &tracked_devices[i];

    if (dev->device_id == NETWORK_DEVICE_ID_NO_DEVICE) {
      continue;
    }

    if (dev->device_id == device_id) {
      return dev;
    }
  }

  return NULL;
}

static struct network_device *network_device_find_by_fd(int fd) {
  for (unsigned i = 0; i < MAX_TRACKED_DEVICES; i++) {
    struct network_device *dev = &tracked_devices[i];

    if (dev->device_id == NETWORK_DEVICE_ID_NO_DEVICE) {
      continue;
    }

    if (dev->fd == fd) {
      return dev;
    }
  }

  return NULL;
}

/******************************************************************************
 API
 ******************************************************************************/

bool network_device_exists(unsigned device_id) {
  return network_device_find_by_id(device_id) != NULL;
}

int network_device_get_fd(unsigned device_id) {
  struct network_device *net_dev = network_device_find_by_id(device_id);

  if (net_dev < 0) {
    return -1;
  }

  return net_dev->fd;
}

int network_device_add(unsigned device_id, int fd) {
  if (device_id == NETWORK_DEVICE_ID_NO_DEVICE) {
    return -1;
  }

  struct network_device *dev = network_device_find_by_id(device_id);

  if (dev != NULL) {
    return -1;
  }

  dev = find_free_place();

  if (dev == NULL) {
    return -1;
  }

  dev->device_id = device_id;
  dev->fd = fd;

  memset(dev->command_handlers, 0, sizeof(dev->command_handlers));

  return 0;
}

int network_device_enable_handling(unsigned device_id) {
  struct network_device *dev = network_device_find_by_id(device_id);

  if (dev == NULL) {
    return -1;
  }

  dev->handled = true;
  LOG_INF("Device %d handling enabled", device_id);
  return 0;
}

int network_device_disable_handling(unsigned device_id) {
  struct network_device *dev = network_device_find_by_id(device_id);

  if (dev == NULL) {
    return -1;
  }

  dev->handled = false;
  LOG_INF("Device %d handling disabled", device_id);
  return 0;
}

int network_device_remove(unsigned device_id) {
  struct network_device *dev = network_device_find_by_id(device_id);

  if (dev == NULL) {
    return -1;
  }

  dev->device_id = NETWORK_DEVICE_ID_NO_DEVICE;

  return 0;
}

int network_device_notify_reconfig(void) {

  char command = UNIX_CMD_RECONFIG;
  int ret = send(config_socket, &command, sizeof(command), 0);

  if (ret < 0) {
    return ret;
  }

  char response;
  ret = recv(config_socket, &response, sizeof(response), 0);

  if (ret <= 0 || response != UNIX_CMD_ACK) {
    return -1;
  }

  return 0;
}

int network_device_set_command_error_handler(
    unsigned device_id, void (*handler)(unsigned, unsigned char, int)) {
  struct network_device *dev = network_device_find_by_id(device_id);

  if (dev == NULL) {
    LOG_ERR("Device with id %d not found", device_id);
    return -1;
  }

  dev->command_error_handler = handler;

  LOG_INF("Command error handler set for device %d", device_id);
  return 0;
}

int network_device_set_net_error_handler(unsigned device_id,
                                         void (*handler)(unsigned, int)) {
  struct network_device *dev = network_device_find_by_id(device_id);

  if (dev == NULL) {
    LOG_ERR("Device with id %d not found", device_id);
    return -1;
  }

  dev->net_error_handler = handler;
  LOG_INF("Network error handler set for device %d", device_id);
  return 0;
}

int network_device_set_handler(int device_id, unsigned char command,
                               int (*handler)(unsigned)) {

  struct network_device *dev = network_device_find_by_id(device_id);

  if (dev == NULL) {
    return -1;
  }

  dev->command_handlers[command] = handler;
  return 0;
}

/******************************************************************************
 Handler thread
 ******************************************************************************/

static size_t get_pollfd_from_traced_devices(int local_config_socket,
                                             struct pollfd *pollfds) {
  size_t pollfd_count = 1;

  pollfds[POLLFD_RECONF_SOCK_IDX].fd = local_config_socket;
  pollfds[POLLFD_RECONF_SOCK_IDX].events = POLLIN;

  for (unsigned i = 0; i < MAX_TRACKED_DEVICES; i++) {
    struct network_device *dev = &tracked_devices[i];

    if (dev->device_id == NETWORK_DEVICE_ID_NO_DEVICE) {
      continue;
    }

    pollfds[pollfd_count].fd = dev->fd;
    pollfds[pollfd_count].events = POLLIN;
    pollfd_count++;
  }

  return pollfd_count;
}

void handle_socket_commands(struct pollfd *pollfds, size_t num_fds) {
  int ret;

  for (unsigned i = 0; i < num_fds; i++) {
    if (i == POLLFD_RECONF_SOCK_IDX) {
      continue;
    }

    struct pollfd *pfd = &pollfds[i];
    struct network_device *net_dev = network_device_find_by_fd(pfd->fd);

    if (net_dev == NULL) {
      LOG_ERR("Failure finding device with fd %d", pfd->fd);
      continue;
    }

    if (pfd->revents & (POLLERR | POLLHUP | POLLNVAL)) {
      // get the error from the socket
      int error_code;
      socklen_t error_code_len = sizeof(error_code);
      ret = getsockopt(pfd->fd, SOL_SOCKET, SO_ERROR, &error_code,
                       &error_code_len);

      if (ret < 0) {
        LOG_ERR("Failed to get socket error for fd %d, device_id %d: %s",
                pfd->fd, net_dev->device_id, strerror(errno));
      }

      LOG_ERR(
          "Device with id %d encountered error during poll, removing it: %s",
          net_dev->device_id, strerror(error_code));
      if (net_dev->net_error_handler != NULL) {
        net_dev->net_error_handler(net_dev->device_id, error_code);
      }

      ret = network_device_remove(net_dev->device_id);
      continue;
    }

    if (pfd->revents & POLLIN && net_dev->handled) {
      unsigned char command = 0;
      ret = recv(pfd->fd, &command, sizeof command, 0);

      if (ret < 0) {
        LOG_ERR("Failure receiving data from socket %d, device_id %d",
                net_dev->fd, net_dev->device_id);
        if (net_dev->net_error_handler != NULL) {
          net_dev->net_error_handler(net_dev->device_id, errno);
        }
        continue;
      }

      int (*handler)(unsigned) = net_dev->command_handlers[command];

      if (handler == NULL) {
        LOG_ERR("Attermpting to call a command '%c' for which a handler was "
                "not registered.");
        continue;
      }

      LOG_INF("Executing handler for command %c, device %d", command,
              net_dev->device_id);
      ret = handler(net_dev->device_id);

      if (ret < 0) {
        LOG_ERR("Command handler '%c' failed while executing for fd %d, "
                "device_id %d",
                command, net_dev->fd, net_dev->device_id);
        if (net_dev->command_error_handler != NULL) {
          net_dev->command_error_handler(net_dev->device_id, command, ret);
        }
      }
    }
  }
}

static void event_handler(void *unused0, void *unused1, void *unused2) {
  int config_sockpair[2];

  int ret = socketpair(AF_UNIX, SOCK_STREAM, 0, config_sockpair);

  if (ret < 0) {
    LOG_ERR("Failed to initialize configuration pair of sockets");
    return;
  }

  config_socket = config_sockpair[0];
  int local_config_socket = config_sockpair[1];

  LOG_INF("Event handler thread initialized");

  while (true) {
    struct pollfd pollfd_sockets[MAX_TRACKED_DEVICES];
    size_t num_fds =
        get_pollfd_from_traced_devices(local_config_socket, pollfd_sockets);

    ret = poll(pollfd_sockets, num_fds, EVENT_HANDLER_POLL_TIMEOUT);

    if (ret <= 0) {
      LOG_ERR("Unexpected error occurred at poll, %s", strerror(errno));
      return;
    }

    if (pollfd_sockets[POLLFD_RECONF_SOCK_IDX].revents &
        (POLLERR | POLLHUP | POLLNVAL)) {
      LOG_ERR("Fatal command handler error, error on socket");
      return;
    }

    if (pollfd_sockets[POLLFD_RECONF_SOCK_IDX].revents & POLLIN) {

      char command;
      ret = recv(pollfd_sockets[POLLFD_RECONF_SOCK_IDX].fd, &command,
                 sizeof command, 0);

      if (ret < 0) {
        LOG_ERR("Failed to receive a reconfig command");
        continue;
      }

      if (command != UNIX_CMD_RECONFIG) {
        LOG_ERR("Unrecognized config command: %c", command);
      }

      LOG_INF("Reconfigure command received");

      char response = UNIX_CMD_ACK;
      ret = send(pollfd_sockets[POLLFD_RECONF_SOCK_IDX].fd, &response,
                 sizeof(response), 0);

      if (ret < 0) {
        LOG_ERR("Failed to respond to a reconfig command");
      }

      continue;
    }

    handle_socket_commands(pollfd_sockets, num_fds);

    k_msleep(10);
  }
}

K_THREAD_DEFINE(device_event_handler, 2048, event_handler, NULL, NULL, NULL,
                K_PRIO_COOP(10), 0, 0);