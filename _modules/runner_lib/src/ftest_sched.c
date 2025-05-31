#include "ftest_sched.h"
#include "nsi_hw_scheduler.h"
#include "nsi_main_semipublic.h"
#include "nsi_tracing.h"
#include "nsi_utils.h"
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>

/******************************************************************************
 Structures
 ******************************************************************************/

struct ftest_shed_entity_entry {
  struct ftest_shed_entity_config *entity_config;
  uint64_t init_time;
  bool initialized;
};

/******************************************************************************
 Data
 ******************************************************************************/

static size_t ftest_shed_entity_count = 0;
static struct ftest_shed_entity_entry entities[CONFIG_FTEST_SHED_MAX_ENTITIES];

static struct ftest_shed_entity_config runner_entity = {
    .init_func = nsi_init,
    .exec_func = nsi_hws_one_event,
    .find_next_event = nsi_hws_find_next_event,
    .get_next_event_time = nsi_hws_get_next_event_time,
};

/******************************************************************************
 Utils
 ******************************************************************************/

bool ftest_shed_is_valid_entity(struct ftest_shed_entity_config *entity) {
  return entity != NULL && entity->init_func != NULL &&
         entity->exec_func != NULL && entity->find_next_event != NULL &&
         entity->get_next_event_time != NULL;
}

/******************************************************************************
 API
 ******************************************************************************/

int ftest_add_entity_to_schedule(
    struct ftest_shed_entity_config *entity_config) {

  if (ftest_shed_entity_count >= CONFIG_FTEST_SHED_MAX_ENTITIES) {
    errno = ENOMEM;
    return -1;
  }

  if (!ftest_shed_is_valid_entity(entity_config)) {
    errno = EINVAL;
    return -1;
  }

  struct ftest_shed_entity_entry *new_entry =
      &entities[ftest_shed_entity_count++];

  new_entry->entity_config = entity_config;
  new_entry->initialized = false;

  return 0;
}

static struct ftest_shed_entity_entry *
get_next_scheduled_entity_init_if_needed(int argc, char *argv[]) {
  uint64_t next_event_time = NSI_NEVER;
  struct ftest_shed_entity_entry *next_entity = NULL;

  for (size_t i = 0; i < ftest_shed_entity_count; i++) {
    struct ftest_shed_entity_entry *entity = &entities[i];

    if (!entity->initialized) {
      entity->entity_config->init_func(argc, argv);
      entity->init_time = nsi_hws_get_time();
      entity->initialized = true;
    }

    uint64_t event_time =
        entity->init_time + entity->entity_config->get_next_event_time();

    if (next_entity == NULL || event_time < next_event_time) {
      next_entity = entity;
      next_event_time = event_time;
    }
  }

  return next_entity;
}

/******************************************************************************
 Sheduler loop
 ******************************************************************************/

int main(int argc, char *argv[]) {

  int result = ftest_add_entity_to_schedule(&runner_entity);

  if (result < 0) {
    nsi_print_error_and_exit(
        "Catastrophic init failure - unable to shedule the runner.");
  }

  while (true) {
    struct ftest_shed_entity_entry *next_scheduled_entity =
        get_next_scheduled_entity_init_if_needed(argc, argv);

    next_scheduled_entity->entity_config->exec_func();
  }

  NSI_CODE_UNREACHABLE; /* LCOV_EXCL_LINE */
}