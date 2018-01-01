/* Copyright 1989 Carnegie Mellon University */

const char *cl_arg(int n);
boolean cl_init(const char* av[], int ac);
long cl_int_option(const char *name, long deflt);
const char *cl_option(const char *name);
boolean cl_switch(const char *name);
boolean cl_syntax(const char *name);
