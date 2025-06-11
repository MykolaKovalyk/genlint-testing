#include "zephyr/devicetree.h"
#include "zephyr/drivers/adc.h"
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

// ADC Configuration
#define ADC_RESOLUTION 12
#define ADC_GAIN ADC_GAIN_1
#define ADC_REFERENCE ADC_REF_INTERNAL
#define ADC_ACQUISITION_TIME ADC_ACQ_TIME_DEFAULT

// Configurable voltage threshold (in millivolts)
#define VOLTAGE_THRESHOLD_MV 2500 // 2.5V threshold - adjust as needed

/******************************************************************************
 Configuration
 ******************************************************************************/

LOG_MODULE_REGISTER(adc_alarm_main);

/******************************************************************************
 Devices
 ******************************************************************************/

static struct adc_dt_spec adc_channel = ADC_DT_SPEC_GET(DT_PATH(zephyr_user));

/******************************************************************************
 Data
 ******************************************************************************/

static int master_socket = -1;
static int16_t adc_sample_buffer[1];

static struct adc_sequence adc_sequence = {
    .buffer = adc_sample_buffer,
    .buffer_size = sizeof(adc_sample_buffer),
    .resolution = ADC_RESOLUTION,
    .oversampling = 0,
    .calibrate = false,
};

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

// Reference voltage for internal reference (typical for emulated ADC)
#define ADC_VREF_MV 3300

/******************************************************************************
 Main
 ******************************************************************************/

int main(void) {
  if (!adc_is_ready_dt(&adc_channel)) {
    LOG_ERR("ADC device not ready");
    return -1;
  }

  int ret = adc_channel_setup_dt(&adc_channel);
  if (ret < 0) {
    LOG_ERR("Failed to setup ADC channel, error %d", ret);
    return ret;
  }

  LOG_INF("ADC alarm initialized, monitoring voltage threshold: %d mV",
          VOLTAGE_THRESHOLD_MV);

  bool alarm_triggered = false;
  bool old_alarm_state = false;

  while (true) {
    old_alarm_state = alarm_triggered;
    k_msleep(100);

    if (master_socket < 0) {
      ret = connect_to_network();
      if (ret < 0) {
        LOG_ERR("Failed to connect to network, error %d\n", ret);
        continue;
      }

      LOG_INF("Connected to network, monitoring ADC voltage...");

      char identify_command = NET_CMD_IDENTIFY_AS_DEVICE;
      ret = send(master_socket, &identify_command, sizeof(identify_command), 0);

      if (ret < 0) {
        LOG_ERR("Failed to send identify command, error %d: %s", errno,
                strerror(errno));
        close(master_socket);
        master_socket = -1;
        continue;
      }
    }

    ret = adc_sequence_init_dt(&adc_channel, &adc_sequence);
    if (ret < 0) {
      LOG_ERR("Failed to initialize ADC sequence, error %d", ret);
      continue;
    }

    // Read ADC value
    ret = adc_read_dt(&adc_channel, &adc_sequence);
    if (ret < 0) {
      LOG_ERR("Failed to read ADC, error %d", ret);
      continue;
    }

    // Convert raw ADC value to millivolts using Zephyr's API
    int32_t voltage_mv = adc_sample_buffer[0];
    ret = adc_raw_to_millivolts(ADC_VREF_MV, ADC_GAIN, ADC_RESOLUTION,
                                &voltage_mv);
    if (ret < 0) {
      LOG_ERR("Failed to convert ADC raw value to millivolts, error %d", ret);
      continue;
    }

    // Check if voltage exceeds threshold
    alarm_triggered = (voltage_mv > VOLTAGE_THRESHOLD_MV);

    // Trigger alarm on rising edge (voltage crosses threshold)
    if (alarm_triggered && !old_alarm_state) {
      LOG_INF(
          "Voltage threshold exceeded (%d mV > %d mV), sending alarm to server",
          voltage_mv, VOLTAGE_THRESHOLD_MV);

      char signal = NET_CMD_ALARM;
      ret = send(master_socket, &signal, sizeof(signal), 0);

      if (ret < 0) {
        LOG_ERR("Failed to send alarm signal, error %d: %s", errno,
                strerror(errno));

        close(master_socket);
        master_socket = -1;

        continue;
      }

      LOG_INF("Alarm signal sent successfully");
    }

    // Optional: Log current voltage periodically for debugging
    static int log_counter = 0;
    if (++log_counter >= 50) { // Log every 5 seconds (50 * 100ms)
      LOG_DBG("Current voltage: %d mV", voltage_mv);
      log_counter = 0;
    }
  }

  return 0;
}