#include "ftest_eth_buf.h"
#include "ringbuffer.h"
#include "ftest_pcap.h"
#include "zephyr/kernel.h"
#include "zephyr/kernel/thread.h"
#include "zephyr/logging/log_core.h"
#include "zephyr/net/ethernet.h"
#include "zephyr/net/net_if.h"
#include <stddef.h>
#include <stdint.h>
#include <zephyr/logging/log.h>

/******************************************************************************
 Definitions
 ******************************************************************************/

#define DT_DRV_COMPAT ftest_eth_inproc

#define ETH_HDR_LEN sizeof(struct net_eth_hdr)
#define FTEST_HDR_LEN (sizeof(struct ftest_eth_hdr))
#define TOTAL_PLD_LEN (FTEST_HDR_LEN + ETH_HDR_LEN + NET_ETH_MTU)
#define NET_BUF_TIMEOUT K_MSEC(100)

/******************************************************************************
 Structures
 ******************************************************************************/

struct ftest_eth_hdr {
  size_t len;
  uint8_t payload[];
} __packed;

struct ftest_eth_inproc_config {
  uint8_t mac[6];
  const char *ip;
  const char *mask;
};

struct ftest_eth_inproc_data {
  uint8_t send_buf[TOTAL_PLD_LEN];
  uint8_t recv_buf[TOTAL_PLD_LEN];
  uint32_t ringbuf_rx_index;
  struct net_linkaddr ll_addr;
  struct k_thread rx_thread;
  struct z_thread_stack_element *rx_stack;
  size_t rx_stack_size;
};

/******************************************************************************
 Module confiugration
 ******************************************************************************/

LOG_MODULE_REGISTER(DT_DRV_COMPAT, LOG_LEVEL_ERR);

/******************************************************************************
 Driver implementation
 ******************************************************************************/

static struct net_pkt *prepare_pkt(struct net_if *iface, uint8_t *payload,
                                   int count, int *status) {

  struct net_pkt *pkt =
      net_pkt_rx_alloc_with_buffer(iface, count, AF_UNSPEC, 0, NET_BUF_TIMEOUT);

  if (!pkt) {
    *status = -ENOMEM;
    return NULL;
  }

  if (net_pkt_write(pkt, payload, count)) {
    net_pkt_unref(pkt);
    *status = -ENOBUFS;
    return NULL;
  }

  *status = 0;

  LOG_DBG("Recv pkt %p len %d", pkt, count);

  return pkt;
}

static void ftest_eth_rx_task(void *iface_ptr, void *unused1, void *unused2) {
  ARG_UNUSED(unused1);
  ARG_UNUSED(unused2);

  struct net_if *iface = iface_ptr;
  const struct device *dev = net_if_get_device(iface);
  struct ftest_eth_inproc_data *data = dev->data;

  LOG_DBG("Starting ZETH RX thread");

  while (true) {
    if (net_if_is_up(iface)) {
      while (rb_available(&ftest_eth_inproc_rb, data->ringbuf_rx_index) > 0) {

        rb_result_t res = rb_read(&ftest_eth_inproc_rb, data->ringbuf_rx_index,
                                  data->recv_buf, TOTAL_PLD_LEN, NULL);
        data->ringbuf_rx_index++;

        if (res != RB_OK && res != RB_OVERRUN) {
          LOG_ERR("Failed to read from ring buffer: %d", res);
          continue;
        }

        if (res == RB_OVERRUN) {
          LOG_WRN("Ring buffer overrun detected, data will be lost");
          continue;
        }

        struct ftest_eth_hdr *ftest_hdr =
            (struct ftest_eth_hdr *)data->recv_buf;
        uint8_t *payload = ftest_hdr->payload;

        int status;
        struct net_pkt *pkt =
            prepare_pkt(iface, payload, ftest_hdr->len, &status);

        if (!pkt) {
          net_pkt_unref(pkt);
          LOG_ERR("Failed to prepare packet: %d", status);
          continue;
        }

        status = net_recv_data(iface, pkt);
        if (status < 0) {
          LOG_ERR("Failed to receive data on iface %p, status %d", iface,
                  status);
          net_pkt_unref(pkt);
          continue;
        }

        LOG_INF("Received pkt %p len %d on iface %p", pkt, ftest_hdr->len,
                iface);

#if CONFIG_FTEST_ETH_OUTPUT_PCAP
        ftest_pcap_write_packet(ftest_hdr->payload, ftest_hdr->len,
                                k_uptime_get());
#endif
      }
      k_yield();
    }

    k_sleep(K_MSEC(10));
  }
}

/******************************************************************************
 Driver API
 ******************************************************************************/

static void ftest_eth_iface_init(struct net_if *iface) {
  const struct device *dev = net_if_get_device(iface);
  struct ftest_eth_inproc_data *data = dev->data;
  const struct ftest_eth_inproc_config *config = dev->config;

  ethernet_init(iface);

  if (!rb_is_valid(&ftest_eth_inproc_rb)) {
    rb_result_t res =
        rb_init(&ftest_eth_inproc_rb, ftest_eth_inproc_rb_buffer,
                sizeof(ftest_eth_inproc_rb_buffer), TOTAL_PLD_LEN);

    if (res != RB_OK) {
      LOG_ERR("Failed to initialize eth ring buffer: %d", res);
      return;
    }
  }

  data->ringbuf_rx_index = rb_get_write_index(&ftest_eth_inproc_rb);

  int res = net_linkaddr_set(&data->ll_addr, config->mac, sizeof(config->mac));
  if (res < 0) {
    LOG_ERR("Failed to convert link address: %d", res);
    return;
  }

  res = net_if_set_link_addr(iface, data->ll_addr.addr, data->ll_addr.len,
                             NET_LINK_ETHERNET);
  if (res < 0) {
    LOG_ERR("Failed to set link address on iface %p: %d", iface, res);
    return;
  }

  struct in_addr addr;
  struct in_addr netmask;

  res = net_addr_pton(AF_INET, config->ip, &addr);

  if (res < 0) {
    NET_ERR("Invalid address: %s", config->ip);
    return;
  }

  struct net_if_addr *addr_res =
      net_if_ipv4_addr_add(iface, &addr, NET_ADDR_MANUAL, 0);

  if (addr_res == NULL) {
    NET_ERR("Failed to add IPv4 address: %s", config->ip);
    return;
  }

  res = net_addr_pton(AF_INET, config->mask, &netmask);

  if (res < 0) {
    NET_ERR("Invalid netmask: %s", config->mask);
    return;
  }

  bool mask_res = net_if_ipv4_set_netmask_by_addr(iface, &addr, &netmask);

  if (!mask_res) {
    NET_ERR("Failed to set netmask: %s", config->mask);
    return;
  }

  k_tid_t tid = k_thread_create(&data->rx_thread, data->rx_stack,
                                data->rx_stack_size, ftest_eth_rx_task, iface,
                                NULL, NULL, K_PRIO_COOP(14), 0, K_NO_WAIT);

  if (tid == NULL) {
    LOG_ERR("Failed to create RX thread");
    return;
  }

  LOG_INF("Eth interface %p initialized with MAC "
          "%02x:%02x:%02x:%02x:%02x:%02x, address %s, netmask %s",
          iface, config->mac[0], config->mac[1], config->mac[2], config->mac[3],
          config->mac[4], config->mac[5], config->ip, config->mask);
}

static int ftest_eth_iface_send(const struct device *dev, struct net_pkt *pkt) {
  struct ftest_eth_inproc_data *data = dev->data;
  int count = net_pkt_get_len(pkt);
  int ret;

  struct ftest_eth_hdr *ftest_hdr = (struct ftest_eth_hdr *)data->send_buf;
  ftest_hdr->len = count;

  ret = net_pkt_read(pkt, ftest_hdr->payload, count);
  if (ret) {
    LOG_ERR("Cannot retrieve pkt %p data (%d)", pkt, ret);
    return ret;
  }

  ret = rb_write(&ftest_eth_inproc_rb, data->send_buf, TOTAL_PLD_LEN);
  if (ret < 0) {
    LOG_ERR("Cannot send pkt %p (%d)", pkt, ret);
  }

  LOG_INF("Sent pkt %p len %d", pkt, count);

#if CONFIG_FTEST_ETH_OUTPUT_PCAP
  ftest_pcap_write_packet(ftest_hdr->payload, ftest_hdr->len, k_uptime_get());
#endif

  return ret < 0 ? ret : 0;
}

static int ftest_eth_iface_set_config(const struct device *dev,
                                      enum ethernet_config_type cfg_type,
                                      const struct ethernet_config *cfg) {
  return 0;
}

static enum ethernet_hw_caps
ftest_eth_iface_get_capabilities(const struct device *dev) {
  return 0;
}

/******************************************************************************
 Driver registration
 ******************************************************************************/

static struct ethernet_api ftest_eth_iface_api = {
    .iface_api.init = ftest_eth_iface_init,
    .send = ftest_eth_iface_send,
    .set_config = ftest_eth_iface_set_config,
    .get_capabilities = ftest_eth_iface_get_capabilities,
};

#define FTEST_ETH_INPROC_INIT(inst)                                            \
  K_KERNEL_STACK_DEFINE(ftest_eth_rx_thread_stack_##inst,                      \
                        CONFIG_ARCH_POSIX_RECOMMENDED_STACK_SIZE);             \
                                                                               \
  static struct ftest_eth_inproc_data ftest_eth_inproc_data_##inst = {         \
      .rx_stack = ftest_eth_rx_thread_stack_##inst,                            \
      .rx_stack_size =                                                         \
          K_KERNEL_STACK_SIZEOF(ftest_eth_rx_thread_stack_##inst),             \
  };                                                                           \
                                                                               \
  static const struct ftest_eth_inproc_config ftest_eth_inproc_config_##inst = \
      {                                                                        \
          .mac = DT_PROP(DT_DRV_INST(inst), mac),                              \
          .ip = DT_PROP(DT_DRV_INST(inst), ip),                                \
          .mask = DT_PROP(DT_DRV_INST(inst), mask),                            \
  };                                                                           \
                                                                               \
  ETH_NET_DEVICE_DT_INST_DEFINE(                                               \
      inst, NULL, NULL, &ftest_eth_inproc_data_##inst,                         \
      &ftest_eth_inproc_config_##inst, CONFIG_ETH_INIT_PRIORITY,               \
      &ftest_eth_iface_api, NET_ETH_MTU);

DT_INST_FOREACH_STATUS_OKAY(FTEST_ETH_INPROC_INIT)