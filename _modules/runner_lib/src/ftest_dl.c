#include "ftest_dl.h"
#include <dlfcn.h>
#include <errno.h>
#include <stdio.h>

void *ftest_dl_open_lib(const char *lib_name) {
  void *lib = dlopen(lib_name, RTLD_NOW | RTLD_LOCAL | RTLD_DEEPBIND);

  char *error = dlerror();

  if (error != NULL) {
    printf("Error loading entity library: %s", error);
    return NULL;
  }

  return lib;
}

void *ftest_dl_get_sym(void *lib, const char *sym_name) {
  if (!lib || !sym_name) {
    printf("Invalid library handle or function name\n");
    return NULL;
  }

  void *func = dlsym(lib, sym_name);
  char *error = dlerror();

  if (error != NULL) {
    printf("Error finding function '%s': %s", sym_name, error);
    return NULL;
  }

  return func;
}

void ftest_dl_close_lib(void *lib) {
  if (lib) {
    dlclose(lib);
  }
}