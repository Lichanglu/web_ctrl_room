(1) install openssl
1. tar xvfz openssl-1.0.0.f.tar.gz
2. cd openssl-1.0.0.f
3. ./config
4. make
5. sudo make install

install dir: /usr/local/ssl


(2)make rsa

change dir to rsa dir

export PKG_CONFIG_PATH=/usr/local/ssl/lib/pkgconfig

./mk_all
