#include "zephyr/drivers/gpio.h"
#include "zephyr/logging/log.h"
#include "zephyr/net/net_ip.h"
#include <stdio.h>
#include <sys/socket.h>
#include <zephyr/kernel.h>

/******************************************************************************
 Definitions
 ******************************************************************************/

#define SERVER_PORT 12345
#define SERVER_ADDRESS "192.169.0.2"

#define NET_CMD_IDENTIFY_AS_DEVICE ((char)'d')
#define NET_CMD_ALARM ((char)'\x01')

/******************************************************************************
 Configuration
 ******************************************************************************/

LOG_MODULE_REGISTER(button_main);

/******************************************************************************
 Devices
 ******************************************************************************/

static const struct gpio_dt_spec button =
    GPIO_DT_SPEC_GET(DT_ALIAS(button_pin), gpios);

/******************************************************************************
 Data
 ******************************************************************************/

static int master_socket = -1;

/******************************************************************************
 Utils
 ******************************************************************************/

static int connect_to_network(void) {
  master_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

  if (master_socket < 0) {
    LOG_ERR("Failed to create socket, error %d: %s", errno, strerror(errno));
    return master_socket;
  }

  struct sockaddr_in server_addr = {
      .sin_family = AF_INET,
      .sin_port = htons(12345), // Replace with your server port
  };

  int ret = net_addr_pton(AF_INET, SERVER_ADDRESS, &server_addr.sin_addr);

  if (ret < 0) {
    LOG_ERR("Failed to convert address, error %d: %s", errno, strerror(errno));
    close(master_socket);
    master_socket = -1;
    return ret;
  }

  ret = connect(master_socket, (struct sockaddr *)&server_addr,
                sizeof(server_addr));

  if (ret < 0) {
    LOG_ERR("Failed to connect to server, error %d: %s", errno,
            strerror(errno));
    close(master_socket);
    master_socket = -1;
    return ret;
  }

  LOG_INF("Connected to server at %s:%d", SERVER_ADDRESS, SERVER_PORT);
  return 0;
}

/******************************************************************************
 Main
 ******************************************************************************/

int main(void) {
  int ret = gpio_pin_configure_dt(&button, GPIO_INPUT);

  if (ret < 0) {
    LOG_ERR("Failed to configure button pin, error %d\n", ret);
    return ret;
  }

  LOG_INF("Button main initialized, waiting for button presses...");

  bool button_state = false;
  bool old_button_state = false;

  while (true) {
    old_button_state = button_state;
    k_msleep(100);

    if (master_socket < 0) {
      ret = connect_to_network();
      if (ret < 0) {
        LOG_ERR("Failed to connect to network, error %d\n", ret);
        continue;
      }

      LOG_INF("Button main initialized, waiting for button presses...");

      char identify_command = NET_CMD_IDENTIFY_AS_DEVICE;
      ret = send(master_socket, &identify_command, sizeof(identify_command), 0);

      if (ret < 0) {
        LOG_ERR("Failed to send identify command, error %d: %s", errno,
                strerror(errno));
        close(master_socket);
        continue;
      }
    }

    button_state = gpio_pin_get_dt(&button);

    if (button_state && !old_button_state) {
      LOG_INF("Button pressed, sending signal to server");

      char signal = NET_CMD_ALARM;
      ret = send(master_socket, &signal, sizeof(signal), 0);

      if (ret < 0) {
        LOG_ERR("Failed to send signal, error %d: %s", errno, strerror(errno));

        close(master_socket);
        master_socket = -1;

        continue;
      }

      LOG_INF("Signal sent successfully");
    }
  }

  return 0;
}