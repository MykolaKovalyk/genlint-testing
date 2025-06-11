#include "zephyr/logging/log.h"
#include <limits.h>
#include <string.h>
#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/kernel.h>

/******************************************************************************
 Definitions
 ******************************************************************************/

#define COMMAND_HANDLER_COUNT (UCHAR_MAX + 1)

/******************************************************************************
 Configuration
 ******************************************************************************/

LOG_MODULE_REGISTER(command_processor);

/******************************************************************************
 Data
 ******************************************************************************/

static const struct device *uart = DEVICE_DT_GET(DT_ALIAS(command_bus));

static int (*command_handlers[COMMAND_HANDLER_COUNT])() = {0};

/******************************************************************************
 API
 ******************************************************************************/

void command_processor_register_handler(unsigned char command,
                                        int (*handler)()) {
  command_handlers[command] = handler;
}

/******************************************************************************
 UART Handler Thread
 ******************************************************************************/

static void uart_handler(void) {
  if (!device_is_ready(uart)) {
    LOG_ERR("UART device not ready");
    return;
  }

  LOG_INF("UART command listener started");

  while (1) {
    unsigned char command;
    int res = uart_poll_in(uart, &command);

    if (res < 0) {
      k_msleep(10);
      continue;
    }

    int (*handler)() = command_handlers[command];

    if (handler == NULL) {
      LOG_ERR("Command handler for '%c' not registered", command);
      continue;
    }

    LOG_INF("Executing command handler for '%c'", command);
    res = handler();

    if (res < 0) {
      LOG_ERR("Command handler for '%c' failed with error %d", command, res);
    }
  }
}

K_THREAD_DEFINE(uart_handling_thread, 1024, uart_handler, NULL, NULL, NULL,
                K_PRIO_COOP(5), 0, 0);