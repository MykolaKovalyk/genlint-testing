#include "ftest_entity_api.h"

static struct ftest_entity_api ftest_entity_api_impl = {
    .initialized = true,
    .device_get_binding = device_get_binding,
};

extern const struct ftest_entity_api *ftest_entity_api;

__attribute__((constructor)) static void ftest_entity_api_initializer(void) {
  ftest_entity_api = &ftest_entity_api_impl;
}