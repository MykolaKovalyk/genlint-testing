/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "ftest_gpio_iface.h"
#include "ftest_uart_iface.h"
#include "sys/socket.h"
#include "zephyr/kernel.h"
#include "zephyr/ztest_assert.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/_intsup.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/net/socket.h>
#include <zephyr/ztest.h>

LOG_MODULE_REGISTER(framework_tests);

ZTEST_SUITE(framework_tests, NULL, NULL, NULL, NULL, NULL);

#define FTEST_MASTER_IP "192.169.0.2"
#define FTEST_MASTER_PORT 12345

#define FTEST_BUTTON_PIN 27
#define FTEST_BUZZ_PIN 27

#define BUTTON_GPIO DEVICE_DT_GET(DT_NODELABEL(button_gpio))
#define BUZZER_GPIO DEVICE_DT_GET(DT_NODELABEL(buzz_gpio))
#define MASTER_UART DEVICE_DT_GET(DT_NODELABEL(master_uart))
#define BUZZ_UART DEVICE_DT_GET(DT_NODELABEL(buzz_uart))

#define NET_CMD_DEVICE_REGISTERED ((char)'r')
#define NET_CMD_IDENTIFY_AS_STATION ((char)'s')
#define NET_CMD_ACK ((char)'a')
#define NET_CMD_ALARM ((char)'\x01')
#define NET_CMD_ARM ((char)'\x02')

#define UART_CMD_LONG_BUZZ ((char)'l')
#define UART_CMD_VARYING_BUZZ ((char)'b')

static void send_command(int fd, char command) {
  int ret = send(fd, &command, sizeof(command), 0);
  zassert_true(ret > 0, "Failed to send command, error %d: %s", errno,
               strerror(errno));
}

static void server_connect(int fd) {
  struct sockaddr_in server_addr = {
      .sin_family = AF_INET,
      .sin_port = htons(FTEST_MASTER_PORT),
      .sin_addr.s_addr = inet_addr(FTEST_MASTER_IP),
  };

  int ret = connect(fd, (struct sockaddr *)&server_addr, sizeof(server_addr));
  zassert_true(ret == 0, "Failed to connect to server, error %d: %s", errno,
               strerror(errno));

  LOG_INF("Connected to server at %s:%d", FTEST_MASTER_IP, FTEST_MASTER_PORT);
}

static void receive_command(int fd, char response) {
  char command;
  int ret = recv(fd, &command, sizeof(command), 0);
  zassert_true(ret == 1, "Failed to receive response, error %d: %s", errno,
               strerror(errno));
  zassert_true(command == response, "Expected '%c' response, got '%c'",
               response, command);
}

static void send_uart_command(const struct device *dev, char command) {
  uint32_t size = ftest_uart_emul_put_rx_data(dev, (const uint8_t *)&command,
                                              sizeof(command));
  zassert_true(size == sizeof(command), "Failed to send UART command");
}

static void receive_uart_command(const struct device *dev,
                                 char expected_command) {
  char buffer;
  uint32_t size = 0;

  uint32_t timeout = 1000;
  while (timeout > 0) {
    size = ftest_uart_emul_get_tx_data(dev, &buffer, sizeof(buffer));
    if (size > 0) {
      break;
    }
    k_msleep(10);
    timeout -= 10;
  }

  zassert_true(size == 1u, "Expected to receive 1 byte, got %d bytes", size);
  zassert_true(buffer == expected_command, "Expected '%c' command, got '%c'",
               expected_command, buffer);
}

ZTEST(framework_tests, test_bad_arm_scenario) {
  int fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  zassert_true(fd >= 0, "Failed to create socket, error %d: %s", errno,
               strerror(errno));

  server_connect(fd);

  LOG_INF("Connected to server at %s:%d", FTEST_MASTER_IP, FTEST_MASTER_PORT);

  send_command(fd, NET_CMD_IDENTIFY_AS_STATION);
  receive_command(fd, NET_CMD_ACK);

  send_command(fd, NET_CMD_ARM);
  receive_command(fd, NET_CMD_ACK);

  receive_uart_command(MASTER_UART, UART_CMD_LONG_BUZZ);
  send_uart_command(BUZZ_UART, UART_CMD_LONG_BUZZ);

  receive_uart_command(MASTER_UART, UART_CMD_VARYING_BUZZ);
  send_uart_command(BUZZ_UART, UART_CMD_VARYING_BUZZ);
  
  receive_command(fd, NET_CMD_ALARM);

  k_msleep(100);

  bool pin_state = ftest_gpio_emul_output_get(BUZZER_GPIO, FTEST_BUZZ_PIN);
  zassert_true(pin_state, "Expected buzzer pin to be set, but it is not");

  k_msleep(300);

  pin_state = ftest_gpio_emul_output_get(BUZZER_GPIO, FTEST_BUZZ_PIN);
  zassert_false(pin_state, "Expected buzzer pin to be cleared, but it is not");

  k_msleep(300);

  pin_state = ftest_gpio_emul_output_get(BUZZER_GPIO, FTEST_BUZZ_PIN);
  zassert_true(pin_state, "Expected buzzer pin to be cleared, but it is not");
}
