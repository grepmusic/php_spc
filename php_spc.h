//php_spc.h
#ifndef SPC_H
#define SPC_H
 
//加载config.h，如果配置了的话
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
 
//加载php头文件
#include "php.h"
#define phpext_spc_ptr &spc_module_entry
extern zend_module_entry spc_module_entry;
 
#endif

// whether to debug this extension(log file is at /tmp/spc_debug.log), uncomment this line to debug
// #define SPC_DEBUG

PHP_FUNCTION(spc_set);
PHP_FUNCTION(spc_get);

#ifdef SPC_DEBUG
PHP_MINIT_FUNCTION(spc);
PHP_RINIT_FUNCTION(spc);
PHP_RSHUTDOWN_FUNCTION(spc);
#endif

PHP_MSHUTDOWN_FUNCTION(spc);
