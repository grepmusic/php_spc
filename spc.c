//spc.c

#include "php_spc.h"
// zval** array_copy(zval *zvalue ZEND_FILE_LINE_DC); // in the future I will let spc to store any type of zval, not only string

static zend_function_entry spc_functions[] = {
    ZEND_FE(spc_set, NULL)
    ZEND_FE(spc_get, NULL)
    { NULL, NULL, NULL }
};
static zval* spc_cache_zval = (zval*)0;

//module entry
zend_module_entry spc_module_entry = {
#if ZEND_MODULE_API_NO >= 20010901
     STANDARD_MODULE_HEADER,
#endif
    "spc",
    spc_functions, /* Functions */

#ifdef SPC_DEBUG
    PHP_MINIT(spc), /* MINIT */
    PHP_MSHUTDOWN(spc), /* MSHUTDOWN */
    PHP_RINIT(spc), /* RINIT */
    PHP_RSHUTDOWN(spc), /* RSHUTDOWN */
#else
    NULL, /* MINIT */
    NULL, /* MSHUTDOWN */
    NULL, /* RINIT */
    NULL, /* RSHUTDOWN */
#endif

    NULL, /* MINFO */
#if ZEND_MODULE_API_NO >= 20010901
    "1.0", //这个地方是我们扩展的版本
#endif
    STANDARD_MODULE_PROPERTIES
};
 
#ifdef COMPILE_DL_SPC
ZEND_GET_MODULE(spc)
#endif

#define SPC_ALLOC_ZVAL(arg); \
    (arg) = (zval*) pemalloc(sizeof(zval), 1); \
    INIT_PZVAL(arg);

#define SPC_ARRAY_INIT(arg); \
        Z_TYPE_P(arg) = IS_ARRAY; \
        Z_ARRVAL_P(arg) = (HashTable *) pemalloc(sizeof(HashTable), 1); \
        zend_hash_init(Z_ARRVAL_P(arg), 0, NULL, NULL /*ZVAL_PTR_DTOR*/, 1);

#define SPC_INIT_ZVAL(); if(spc_cache_zval == NULL) { \
            SPC_ALLOC_ZVAL(spc_cache_zval); \
            SPC_ARRAY_INIT(spc_cache_zval); \
    }

#define SPC_ZVAL_STRINGL(z, s, l, duplicate); do { \
                const char *__s=(s); int __l=l; \
                zval *__z = (z); \
                Z_STRLEN_P(__z) = __l; \
                Z_STRVAL_P(__z) = (duplicate?pestrndup(__s, __l, 1):(char*)__s); \
                Z_TYPE_P(__z) = IS_STRING; \
        } while (0)

#define SPC_FREE_ZVAL_STRING(z); \
        pefree(Z_STRVAL_P(z), 1); \
        Z_STRVAL_P(z) = NULL; \
        Z_STRLEN_P(z) = 0;

#ifdef SPC_DEBUG
static void spc_debug_log(char* s) {
    FILE* fp = fopen("/tmp/spc_debug.log", "a");
    fputs(s, fp);
    fputs("\n", fp);
    fclose(fp);
}
#else
#define spc_debug_log(x); 
#endif

#define spc_zval_copy_ctor(zvalue); zval_copy_ctor(zvalue);

static int spc_free_zval_str_func(zval** val TSRMLS_DC) {
    SPC_FREE_ZVAL_STRING(*val);
    return ZEND_HASH_APPLY_KEEP;
}

void spc_cache_truncate() {
    if(spc_cache_zval) {
        zend_hash_apply(Z_ARRVAL_P(spc_cache_zval), spc_free_zval_str_func TSRMLS_CC);
        zend_hash_clean(Z_ARRVAL_P(spc_cache_zval));
        pefree(spc_cache_zval, 1);
        spc_cache_zval = NULL;
    }
}

#ifdef SPC_DEBUG
static char spc_log_buffer[256] = {0};

PHP_MINIT_FUNCTION(spc)
{
    snprintf(spc_log_buffer, 255, "m_init time: %d", time(NULL));
    spc_debug_log(spc_log_buffer);
    return SUCCESS;
}
 
PHP_RINIT_FUNCTION(spc)
{
    snprintf(spc_log_buffer, 255, "r_init time: %d", time(NULL));
    spc_debug_log(spc_log_buffer);
    return SUCCESS;
}

PHP_RSHUTDOWN_FUNCTION(spc)
{
    snprintf(spc_log_buffer, 255, "r_shutdown time: %d\n", time(NULL));
    spc_debug_log(spc_log_buffer);
    return SUCCESS;
}
#endif
 
PHP_MSHUTDOWN_FUNCTION(spc)
{
    spc_debug_log("m_shutdown: cleaning up hash");
    spc_cache_truncate();
    spc_debug_log("m_shutdown: ok\n");
    return SUCCESS;
}

ZEND_FUNCTION(spc_set)
{
    zval* key;
    zval* value;
    zval* value_copy = NULL;
    zval** ppold_value = NULL;

    zend_bool free_old_value = 1;
    char to_remove = 0;

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "zz|b", &key, &value, &free_old_value) == FAILURE) {
        RETURN_FALSE;
    }

    spc_debug_log("set: start ...");
    if(Z_TYPE_P(value) == IS_BOOL && ! Z_LVAL_P(value)) {
        to_remove = 1;
        spc_debug_log("set: to_remove is 1");
    }

    SPC_INIT_ZVAL();
    
    if(Z_TYPE_P(key) != IS_STRING) {
        if(Z_TYPE_P(key) == IS_BOOL && ! Z_LVAL_P(key)) {
            spc_debug_log("set: [WARN] key is false, so truncate entire hash");
            spc_cache_truncate();
            RETURN_TRUE;
        }
        php_error_docref(NULL TSRMLS_CC, E_ERROR,"spc_set(/* strict */ string|bool false $key, /* strict */ string|bool false $value[, $free_old_value=true]): $key must be type of string or FALSE(to truncate entire internal hash)");
       RETURN_FALSE;
    }

    if(! to_remove && Z_TYPE_P(value) != IS_STRING) {
        php_error_docref(NULL TSRMLS_CC, E_ERROR,"bool spc_set(/* strict */ string|bool false $key, /* strict */ string|bool false $value[, $free_old_value=true]): value must be type of string or FALSE(to remove an element corresponding to $key)");
       RETURN_FALSE;
    }

    if((free_old_value || to_remove) && zend_hash_find(Z_ARRVAL_P(spc_cache_zval), Z_STRVAL_P(key), Z_STRLEN_P(key) + 1, (void**)&ppold_value) != FAILURE) {
        if(ppold_value) {
           if(free_old_value) {
               SPC_FREE_ZVAL_STRING(*ppold_value);
               // pefree(*ppold_value, 1); // maybe double free, no need here
               spc_debug_log("set: free old value");
           }
           zend_hash_del(Z_ARRVAL_P(spc_cache_zval), Z_STRVAL_P(key), Z_STRLEN_P(key) + 1);
		   // spc_debug_log( Z_STRVAL_P(*ppold_value) ); // zend_hash_del already free memory, if uncomment this line, will cause segment fault
           spc_debug_log("set: del old value from hash");
           if(to_remove) {
               spc_debug_log("set: [WARN] value is false, so remove it");
               RETURN_FALSE;
           }
        }
    }

    SPC_ALLOC_ZVAL(value_copy);
    SPC_ZVAL_STRINGL(value_copy, Z_STRVAL_P(value), Z_STRLEN_P(value), 1);
    
    if(zend_hash_update(Z_ARRVAL_P(spc_cache_zval), Z_STRVAL_P(key), Z_STRLEN_P(key) + 1, &value_copy, sizeof(zval*), NULL) == FAILURE) {
        spc_debug_log("set: update failed");
        RETURN_FALSE;
    }
    spc_debug_log("set: update ok");

    RETURN_TRUE;
}

ZEND_FUNCTION(spc_get)
{
    zval* key = NULL;
    zval** ppvalue = NULL;

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "|z!", &key) == FAILURE) {
        RETURN_FALSE;
    }

    SPC_INIT_ZVAL();
    if(! key) {
        *return_value = *spc_cache_zval;
        spc_zval_copy_ctor(return_value);
        spc_debug_log("get: get all");
        return;
    }

    if(Z_TYPE_P(key) != IS_STRING) {
        php_error_docref(NULL TSRMLS_CC, E_ERROR,"spc_get([ /* strict */ string $key ]): $key must be type of string");
       RETURN_FALSE;
    }

    spc_debug_log("get: start ...");

    if(zend_hash_find(Z_ARRVAL_P(spc_cache_zval), Z_STRVAL_P(key), Z_STRLEN_P(key) + 1, (void**)&ppvalue) == FAILURE) {
        spc_debug_log("get: not found");
        RETURN_FALSE;
    }
    if(! ppvalue || ! *ppvalue) {
        spc_debug_log("get: found but value is invalid");
        RETURN_FALSE;
    }
    *return_value = **ppvalue;
    spc_zval_copy_ctor(return_value);
    spc_debug_log("get: ok");
}

/*

zval** array_copy(zval *zvalue ZEND_FILE_LINE_DC) {
    char k[2] = { 'k', '\0' };
    HashTable* orig_ht = NULL;
    // ALLOC_HASHTABLE_REL(orig_ht);
    orig_ht = (HashTable *) pemalloc(sizeof(HashTable), 1);
    zend_hash_init(orig_ht, 1, NULL, /* ZVAL_PTR_DTOR * / NULL, 1);
    FILE *fp=fopen("/tmp/req_log","a+");//请确保文件可写，否则apache会莫名崩溃
    if(zend_hash_update(orig_ht, k, 2, zvalue, sizeof(zval*), NULL) == FAILURE) {
        fprintf(fp,"array_copy (update failed): %d\n",time(NULL));
        fclose(fp);
        return 0;
    }

    zval *tmpzval = NULL;
    HashTable* tmp_ht = NULL;
    TSRMLS_FETCH();
    tmp_ht = (HashTable *) pemalloc(sizeof(HashTable), 1);
    zend_hash_init(tmp_ht, zend_hash_num_elements(orig_ht), NULL, NULL /* ZVAL_PTR_DTOR * /, 1); // persistent hash
    zend_hash_copy(tmp_ht, orig_ht, /*(copy_ctor_func_t) zval_add_ref* / NULL, (void *) &tmpzval, sizeof(zval *));
    tmp_ht->nNextFreeElement = orig_ht->nNextFreeElement;

    zval** ret = NULL;
    if(zend_hash_find(tmp_ht, k, 2, (void**)&ret) == FAILURE) {
        fprintf(fp,"array_copy (not found): %d\n",time(NULL));
        fclose(fp);
        return 0;
    }
    if(! ret) {
        fprintf(fp,"ret is null: %d\n",time(NULL));
        fclose(fp);
        return 0;
    }
        fprintf(fp,"ret is at %p\n", *ret);
        fclose(fp);

    zend_hash_del(orig_ht, k, 2);
    return ret;
}
*/

