#include "network_events.h"
#include "sys/socket.h"
#include "zephyr/drivers/gpio.h"
#include "zephyr/drivers/uart.h"
#include "zephyr/net/net_ip.h"
#include <errno.h>
#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/net/socket.h>

/******************************************************************************
 Definitions
 ******************************************************************************/

#define SERVER_PORT 12345

#define BUZZER_CMD_SHORT ((char)'b')
#define BUZZER_CMD_LONG ((char)'l')
#define BUZZER_CMD_STOP ((char)'s')

#define NET_CMD_IDENTIFY_AS_DEVICE ((char)'d')
#define NET_CMD_IDENTIFY_AS_STATION ((char)'s')
#define NET_CMD_ACK ((char)'a')
#define NET_CMD_DEVICE_REGISTERED ((char)'r')
#define NET_CMD_ALARM ((char)'\x01')
#define NET_CMD_ARM ((char)'\x02')
#define NET_CMD_DISARM ((char)'\x03')

/******************************************************************************
 Configuration
 ******************************************************************************/

LOG_MODULE_REGISTER(socket_client);

/******************************************************************************
 Devices
 ******************************************************************************/

static const struct gpio_dt_spec blue_led =
    GPIO_DT_SPEC_GET(DT_ALIAS(blue_led), gpios);

static const struct gpio_dt_spec red_led =
    GPIO_DT_SPEC_GET(DT_ALIAS(red_led), gpios);

static const struct gpio_dt_spec yellow_led =
    GPIO_DT_SPEC_GET(DT_ALIAS(yellow_led), gpios);

static const struct device *uart = DEVICE_DT_GET(DT_ALIAS(command_bus));

/******************************************************************************
 Data
 ******************************************************************************/

enum alarm_state {
  ALARM_STATE_DISABLED,
  ALARM_STATE_ARMED,
};

static enum alarm_state alarm_state = ALARM_STATE_DISABLED;
static unsigned server_id = NETWORK_DEVICE_ID_NO_DEVICE;

/******************************************************************************
 Delayed work
 ******************************************************************************/

static void disable_red_led(struct k_work *work) {

  int ret = gpio_pin_set_dt(&red_led, 0);
  if (ret < 0) {
    LOG_ERR("Failed to disable red LED, error %d", ret);
    return;
  }

  LOG_INF("Red LED disabled");
}

K_WORK_DELAYABLE_DEFINE(disable_red_led_work, disable_red_led);

static void disable_blue_led(struct k_work *work) {

  int ret = gpio_pin_set_dt(&blue_led, 0);
  if (ret < 0) {
    LOG_ERR("Failed to disable blue LED, error %d", ret);
    return;
  }

  LOG_INF("Blue LED disabled");
}

K_WORK_DELAYABLE_DEFINE(disable_blue_led_work, disable_blue_led);

static void disable_yellow_led(struct k_work *work) {

  int ret = gpio_pin_set_dt(&yellow_led, 0);
  if (ret < 0) {
    LOG_ERR("Failed to disable yellow LED, error %d", ret);
    return;
  }

  LOG_INF("Yellow LED disabled");
}

K_WORK_DELAYABLE_DEFINE(disable_yellow_led_work, disable_yellow_led);

/******************************************************************************
 Utils
 ******************************************************************************/

static int send_confirmation(int device_id) {
  int ret;

  int fd = network_device_get_fd(device_id);

  if (fd < 0) {
    LOG_ERR("Failed to get file descriptor for device %d", device_id);
    return -1;
  }

  char command = NET_CMD_ACK;
  ret = send(fd, &command, sizeof(command), 0);

  return ret > 0 ? 0 : -1;
}

static int send_dev_registered(void) {
  int ret;

  int fd = network_device_get_fd(server_id);

  if (fd < 0) {
    LOG_ERR("Failed to get file descriptor for device %d", server_id);
    return -1;
  }

  char command = NET_CMD_DEVICE_REGISTERED;
  ret = send(fd, &command, sizeof(command), 0);

  if (ret < 0) {
    LOG_ERR("Failed to send device registered command for server %d, error %d",
            server_id, errno);
    return -1;
  }

  return ret > 0 ? 0 : -1;
}

/******************************************************************************
 Handlers
 ******************************************************************************/

static int trigger_alarm(unsigned device_id) {
  int ret;

  if (alarm_state == ALARM_STATE_DISABLED) {
    LOG_WRN("Ignoring alarm, as device %d is disarmed for device", device_id);
    return -1;
  }

  LOG_INF("Alarm triggered for device %d", device_id);

  ret = gpio_pin_set_dt(&red_led, 1);

  if (ret < 0) {
    LOG_ERR("Failed to enable red LED, error %d", ret);
    return ret;
  }

  uart_poll_out(uart, BUZZER_CMD_SHORT);

  int server_fd = network_device_get_fd(server_id);

  if (server_fd < 0) {
    LOG_ERR("Failed to get file descriptor for server %d", server_id);
    return -1;
  }

  char command = NET_CMD_ALARM;
  ret = send(server_fd, &command, sizeof(command), 0);

  if (ret < 0) {
    LOG_ERR("Failed to send alarm command for device %d, error %d", device_id,
            errno);
    return ret;
  }

  return 0;
}

static int disarm_system(unsigned device_id) {
  int ret;

  LOG_INF("Disarming system for device %d", device_id);
  alarm_state = ALARM_STATE_DISABLED;

  ret = gpio_pin_set_dt(&yellow_led, 0);
  if (ret < 0) {
    LOG_ERR("Failed to enable yellow LED, error %d", ret);
    return ret;
  }

  ret = send_confirmation(device_id);
  if (ret < 0) {
    LOG_ERR("Failed to send confirmation for device %d, error %d", device_id,
            ret);
    return ret;
  }

  uart_poll_out(uart, BUZZER_CMD_STOP);
  return -1;
}

static int arm_system(unsigned device_id) {
  int ret;

  LOG_INF("Arming system for device %d", device_id);
  alarm_state = ALARM_STATE_ARMED;

  ret = gpio_pin_set_dt(&yellow_led, 1);
  if (ret < 0) {
    LOG_ERR("Failed to enable yellow LED, error %d", ret);
    return ret;
  }

  uart_poll_out(uart, BUZZER_CMD_LONG);

  ret = send_confirmation(device_id);
  if (ret < 0) {
    LOG_ERR("Failed to send confirmation for device %d, error %d", device_id,
            ret);
    return ret;
  }

  return 0;
}

static int identify_as_device(unsigned device_id) {
  int ret;

  LOG_INF("Identifying as device for device %d", device_id);

  ret = gpio_pin_set_dt(&blue_led, 1);

  if (ret < 0) {
    LOG_ERR("Failed to enable blue LED, error %d", ret);
    return ret;
  }

  ret = k_work_schedule(&disable_blue_led_work, K_MSEC(300));

  if (ret < 0) {
    LOG_ERR("Failed to schedule work to disable yellow LED, error %d", ret);
    return ret;
  }

  ret = network_device_set_handler(device_id, NET_CMD_ALARM, trigger_alarm);

  if (ret < 0) {
    LOG_ERR("Failed to set handler for alarm command, error %d", ret);
    return ret;
  }

  ret = send_dev_registered();
  if (ret < 0) {
    LOG_ERR("Failed to send device registered for device %d, error %d",
            device_id, ret);
    return ret;
  }

  return 0;
}

static int identify_as_station(unsigned device_id) {
  int ret;

  LOG_INF("Identifying as station for device %d", device_id);

  if (server_id != NETWORK_DEVICE_ID_NO_DEVICE) {
    LOG_ERR("Server already identified, cannot identify as station");
    return -1;
  }

  server_id = device_id;

  ret = gpio_pin_set_dt(&blue_led, 1);

  if (ret < 0) {
    LOG_ERR("Failed to enable blue LED, error %d", ret);
    return ret;
  }

  ret = k_work_schedule(&disable_blue_led_work, K_MSEC(300));

  if (ret < 0) {
    LOG_ERR("Failed to schedule work to disable blue LED, error %d", ret);
    return ret;
  }

  ret = network_device_set_handler(device_id, NET_CMD_ALARM, trigger_alarm);

  if (ret < 0) {
    LOG_ERR("Failed to set handler for alarm command, error %d", ret);
    return ret;
  }

  ret = network_device_set_handler(device_id, NET_CMD_ARM, arm_system);
  if (ret < 0) {
    LOG_ERR("Failed to set handler for arm command, error %d", ret);
    return ret;
  }

  ret = network_device_set_handler(device_id, NET_CMD_DISARM, disarm_system);
  if (ret < 0) {
    LOG_ERR("Failed to set handler for disarm command, error %d", ret);
    return ret;
  }

  ret = send_confirmation(device_id);
  if (ret < 0) {
    LOG_ERR("Failed to send confirmation for device %d, error %d", device_id,
            ret);
    return ret;
  }

  return 0;
}

/******************************************************************************
 Error handling
 ******************************************************************************/

static void handle_net_error(unsigned device_id, int error_code) {
  LOG_ERR("Network error for device %d: %d", device_id, error_code);

  if (alarm_state == ALARM_STATE_ARMED) {
    trigger_alarm(device_id);
  }

  network_device_remove(device_id);
}

static void handle_command_error(unsigned device_id, unsigned char command,
                                 int error_code) {
  LOG_ERR("Command error for device %d, command '%c': %d", device_id, command,
          error_code);

  if (alarm_state == ALARM_STATE_ARMED) {
    trigger_alarm(device_id);
  }

  network_device_remove(device_id);
}

/******************************************************************************
 Main
 ******************************************************************************/

int main(void) {
  /** configure led pins */
  gpio_pin_configure_dt(&blue_led, GPIO_OUTPUT_ACTIVE);
  gpio_pin_configure_dt(&red_led, GPIO_OUTPUT_ACTIVE);
  gpio_pin_configure_dt(&yellow_led, GPIO_OUTPUT_ACTIVE);

  if (!device_is_ready(uart)) {
    LOG_ERR("UART device not ready");
    return -ENODEV;
  }

  int server_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

  if (server_socket < 0) {
    LOG_ERR("Failed to create socket, error %d: %s", errno, strerror(errno));
    return -errno;
  }

  struct sockaddr_in server_addr = {
      .sin_family = AF_INET,
      .sin_port = htons(SERVER_PORT),
      .sin_addr.s_addr = htonl(INADDR_ANY),
  };

  int ret =
      bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr));

  if (ret < 0) {
    LOG_ERR("Failed to bind socket, error %d: %s", errno, strerror(errno));
    close(server_socket);
    return -errno;
  }

  ret = listen(server_socket, 5);
  if (ret < 0) {
    LOG_ERR("Failed to listen on socket, error %d: %s", errno, strerror(errno));
    close(server_socket);
    return -errno;
  }

  unsigned device_id_counter = 1;

  while (true) {
    ret = accept(server_socket, NULL, NULL);

    if (ret < 0) {
      LOG_ERR("Failed to accept connection, error %d: %s", errno,
              strerror(errno));
      continue;
    }

    LOG_INF("Accepted connection on socket %d", ret);

    unsigned device_id = device_id_counter++;

    if (alarm_state == ALARM_STATE_ARMED) {
      LOG_INF("Alarm is armed, triggering alarm for new connection");
      ret = trigger_alarm(device_id_counter);

      if (ret < 0) {
        LOG_ERR("Failed to trigger alarm for new connection, error %d", ret);
      }

      close(ret);
      continue;
    }

    ret = network_device_add(device_id, ret);
    if (ret < 0) {
      LOG_ERR("Failed to add network device with id %d, error %d: %s",
              device_id, errno, strerror(errno));
      close(ret);
      continue;
    }

    ret = network_device_set_command_error_handler(device_id,
                                                   handle_command_error);
    if (ret < 0) {
      LOG_ERR("Failed to set command error handler for device %d, error %d",
              device_id, ret);
      network_device_remove(device_id);
      close(ret);
      continue;
    }

    ret = network_device_set_net_error_handler(device_id, handle_net_error);
    if (ret < 0) {
      LOG_ERR("Failed to set network error handler for device %d, error %d",
              device_id, ret);
      network_device_remove(device_id);
      close(ret);
      continue;
    }

    LOG_INF("Network device with id %d added successfully", device_id);

    ret = network_device_set_handler(device_id, NET_CMD_IDENTIFY_AS_DEVICE,
                                     identify_as_device);
    if (ret < 0) {
      LOG_ERR("Failed to set identify as device handler, error %d", ret);
      network_device_remove(device_id);
      close(ret);
      continue;
    }

    ret = network_device_set_handler(device_id, NET_CMD_IDENTIFY_AS_STATION,
                                     identify_as_station);

    if (ret < 0) {
      LOG_ERR("Failed to set identify as station handler, error %d", ret);
      network_device_remove(device_id);
      close(ret);
      continue;
    }

    LOG_INF("Network device with id %d is ready to handle commands", device_id);
    network_device_enable_handling(device_id);

    ret = network_device_notify_reconfig();
    if (ret < 0) {
      LOG_ERR("Failed to notify reconfiguration, error %d: %s", errno,
              strerror(errno));
      network_device_remove(device_id);
      close(ret);
      continue;
    }
  }
}