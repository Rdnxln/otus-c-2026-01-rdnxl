Ход выполнения задания:

По условию ДЗ необходимо оставить самый минимум протоколов:
http, https, telnet

Скачал последнюю версию (на момент выполнения ДЗ):
https://curl.se/download/curl-8.19.0.tar.gz

Извлек содержимое архива
tar xf https://curl.se/download/curl-8.19.0.tar.gz

Попробовал сконфигурировать с настройками по-умолчанию
./configure
чтобы понимать, какой функционал прицельно отключать,
а что, возможно, будет необходимо доустановить.

На следующей итерации исключил libpsl
./configure --without-libpsl

Получил предупреждение, что для поддержки https, необходимо выбрать бэкенд
~~~~~~~
configure: error: select TLS backend(s) or disable TLS with --without-ssl.

Select from these:

  --with-amissl
  --with-gnutls
  --with-mbedtls
  --with-openssl (also works for AWS-LC, BoringSSL and LibreSSL)
  --with-rustls
  --with-schannel
  --with-wolfssl
~~~~~~~

Я выбрал openssl
./configure --with-openssl --without-libpsl

После безошибочной отработки ./configure

Запустил сборку
make -j$(nproc)

Запустил на выполнение пока еще "много-фичастый" curl
./src/curl --version

Появилась возможность проверить, какие протоколы есть, и убирать их прицельно:
./src/curl --version | grep -E '^Protocols\:'
Protocols: dict file ftp ftps gopher gophers http https imap imaps ipfs ipns mqtt mqtts pop3 pop3s rtsp smb smbs smtp smtps telnet tftp ws wss

Теперь предстояла работа с флагами --without и --disable

В итоге для моей системы получилась следующая команда конфигурации:
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

make clean
make -j$(nproc)
./src/curl --version

Вывод в консоли...

  CC       ../lib/curlx/libcurltool_la-wait.lo
  CC       ../lib/curlx/libcurltool_la-warnless.lo
  CC       ../lib/curlx/libcurltool_la-winapi.lo
  CC       toolx/curl-tool_time.o
  CC       curl-tool_hugehelp.o
  CC       curl-tool_ca_embed.o
  CCLD     curlinfo
  CCLD     libcurltool.la
  CCLD     curl
make[1]: выход из каталога «/home/astra/coding/otus/otus-c-2026-01-rdnxl/HW04/curl-8.19.0/src»
Making all in scripts
make[1]: вход в каталог «/home/astra/coding/otus/otus-c-2026-01-rdnxl/HW04/curl-8.19.0/scripts»
make[1]: Цель «all» не требует выполнения команд.
make[1]: выход из каталога «/home/astra/coding/otus/otus-c-2026-01-rdnxl/HW04/curl-8.19.0/scripts»
make[1]: вход в каталог «/home/astra/coding/otus/otus-c-2026-01-rdnxl/HW04/curl-8.19.0»
make[1]: Цель «all-am» не требует выполнения команд.
make[1]: выход из каталога «/home/astra/coding/otus/otus-c-2026-01-rdnxl/HW04/curl-8.19.0»
curl 8.19.0 (x86_64-pc-linux-gnu) libcurl/8.19.0 OpenSSL/3.4.0
Release-Date: 2026-03-11
Protocols: http https telnet
Features: alt-svc AsynchDNS HSTS HTTPS-proxy IPv6 Largefile NTLM SSL threadsafe TLS-SRP UnixSockets
~/coding/otus/otus-c-2026-01-rdnxl/HW04


