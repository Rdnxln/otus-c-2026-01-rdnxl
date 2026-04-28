#!/bin/bash

# если отсутствует архив с исходниками
# качаем его, используя wget (curl'а как буд-то бы у нас еще нет)
[ ! -f "curl-8.19.0.tar.gz" ] &&
  wget https://curl.se/download/curl-8.19.0.tar.gz

# развернем рядом содержимое архива
tar xf curl-8.19.0.tar.gz || { logger -s ' архив поврежден'; exit 1; }

# попробуем сконфигурировать, собрать и запустить
pushd ./curl-8.19.0

./configure \
 --with-openssl \
 --without-libpsl \
 --enable-http \
 --enable-https \
 --enable-telnet \
 --disable-dict \
 --disable-file \
 --disable-ftp \
 --disable-ftps \
 --disable-gopher \
 --disable-gophers \
 --disable-imap \
 --disable-imaps \
 --disable-ipfs \
 --disable-ipns \
 --disable-mqtt \
 --disable-mqtts \
 --disable-pop3 \
 --disable-pop3s \
 --disable-rtsp \
 --disable-smb \
 --disable-smbs \
 --disable-smtp \
 --disable-smtps \
 --disable-tftp \
 --disable-websockets

# в этот момент мы можем словить ошибки конфигурации
# если код возврата последней выполненой команды 0
# (а это был ./configure), то критических ошибок не было
# и можно провести компиляцию (сборку)
[ $? -eq 0 ] &&
{
  make clean
  make -j$(nproc)
  ./src/curl --version
}

popd
