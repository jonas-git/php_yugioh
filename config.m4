dnl $Id$
dnl config.m4 for extension yugioh

PHP_ARG_ENABLE(yugioh,
  [whether to enable yugioh support],
  [  --enable-yugioh           Enable yugioh support])

if test "$PHP_YUGIOH" != "no"; then
  CFLAGS="-lsqlite3 -Os"
  PHP_NEW_EXTENSION(yugioh,
    php_yugioh.c \
    yugioh.c \
    util.c \
    dice.c \
    replay_reader.c \
    lzma/Alloc.c \
    lzma/LzmaDec.c,
    $ext_shared)
fi
