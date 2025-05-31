#pragma once

void *ftest_dl_open_lib(const char *lib_name);

void *ftest_dl_get_sym(void *lib, const char *sym_name);

void ftest_dl_close_lib(void *lib);