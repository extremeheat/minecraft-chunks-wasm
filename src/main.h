#ifdef WEBASSEMBLY
__attribute__((import_module("env"), import_name("externalFunction"))) void externalFunction(void);
#endif
