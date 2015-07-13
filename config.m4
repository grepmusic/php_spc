PHP_ARG_ENABLE(spc,
    [Whether to enable the "spc" extension],
    [  enable-spc        Enable "spc" extension support])

if test $PHP_SPC != "no"; then
    PHP_SUBST(SPC_SHARED_LIBADD)
    PHP_NEW_EXTENSION(spc, spc.c, $ext_shared)
fi
