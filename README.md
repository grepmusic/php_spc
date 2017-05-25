# php_spc
a key-value cache php extension for php-fpm child process

There are only two functions to manipulate php_spc cache: spc_set & spc_get;

```php
SET:
bool spc_set(/* strict */ string|bool false $key, /* strict */ string|bool false $value[, $free_old_value=true]) for setting cache

spc_set(string $key, string $value, /* bool or TYPE which can be converted to bool */ $free_old_value=true) 
// store $value at $key, if $key already exists, $free_old_value parameter tells php_spc whether to free old value before storing $value at $key; if $free_old_value is false, php_spc will update $key with $value directly WITHOUT freeing old value(which will cause memory leak, but it is a faster set operation since php_spc need not determine whether $key exists; $free_old_value defaults to true.

spc_set(string $key, false)
// remove $key from php_spc

spc_set(false, false)
// truncate php_spc internal cache, delete all key-values in php_spc

GET:
string spc_get([ /* strict */ string $key ]) for getting value from cache

spc_get(/* strict */ string $key) // get value at $key, if $key is not found, false will be returned

spc_get() // get all key-values from php_spc
```

Installation:
```bash
git clone https://github.com/grepmusic/php_spc && cd php_spc && phpize --clean && phpize && ./configure && make 
&& echo add "'extension=$(pwd)/modules/spc.so'" to your php.ini configuration file and restart php-fpm
```

So I assume you have installed php_spc successfully, you have temporarily set 'pm.max_children = 1' in your php-fpm.conf and restart php-fpm (kill -USR2 $PHP_FPM_MASTER_PID), then you can test it by the following code:

```php
<?php

echo 'spc_get("c") = ' . var_export(spc_get("c"), true) . "\n"; // value will be available since the 2nd request

echo "spc_get() = " . var_export(spc_get(), true) . "\n";

spc_set("a", "1");

spc_set("b", "2");

echo 'spc_get("b") = ' . var_export(spc_get("b"), true) . "\n";

spc_set("b", false); // remove key "b" 

echo 'spc_get("b") = ' . var_export(spc_get("b"), true) . "\n";
spc_set(false, false); // truncate all key-values
echo "spc_get() (after spc_set(false, false)) = " . var_export(spc_get(), true) . "\n";
spc_set("c", "3");
spc_set("d", "4");
echo "spc_get() = " . var_export(spc_get(), true) . "\n";

?>
```
```php
// The first time you will get:
spc_get("c") = false
spc_get() = array (
)
spc_get("b") = '2'
spc_get("b") = false
spc_get() (after spc_set(false, false)) = array (
)
spc_get() = array (
  'c' => '3',
  'd' => '4',
)
```
```php
// The second time you will get:
spc_get("c") = '3'
spc_get() = array (
  'c' => '3',
  'd' => '4',
)
spc_get("b") = '2'
spc_get("b") = false
spc_get() (after spc_set(false, false)) = array (
)
spc_get() = array (
  'c' => '3',
  'd' => '4',
)
```


The php_spc cache will automatically be cleared ether after php-fpm child process died/respawned/restarted or you manually call spc_set(false, false).

The extension is useful especially when you want to store something(such as php configuration file) which will be only changed after deployed again(then php-fpm will be restarted), it sometimes can be an alternative to memcache/redis/file cache, because it is faster than any kind of these cache.

Currently you can store array variable as serialized string(using serialize(), recommanded because it seems it is faster using unserialize()) or json string(using json_encode()).

Thank you very much if you can give me some feedback about php_spc extension.

