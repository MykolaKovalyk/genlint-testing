

/******************************************************************************
 Globally accessible API pointer definition
 ******************************************************************************/

struct ftest_entity_api;

const struct ftest_entity_api *ftest_entity_api;

/******************************************************************************
 API
 ******************************************************************************/

const struct ftest_entity_api *ftest_entity_api_get(void) {
  return ftest_entity_api;
}