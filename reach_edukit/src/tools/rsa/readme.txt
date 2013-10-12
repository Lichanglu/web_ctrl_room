
export PKG_CONFIG_PATH=/usr/local/ssl/lib/pkgconfig

1. 文件
整个系统一共包括1个lib, 4个可执行程序
a. rsa lib
 rsa.c rsa.h ---> librsa.so 
 由mk_rsa_lib.sh 生成
b. client register 程序
 cli_reg.c ---> cli_reg
 由mk_cli_reg.sh 生成
c. rsa private/public key pair generator程序
 rsa_key_gen.c ---> rsa_key_gen
 由mk_rsa_key_gen.sh 生成
d. server build key 程序
 srv_build_key.c ---> srv_build_key.c
  由 mk_srv_build_key.sh 生成
e. client verify 程序 (测试程序例子)
  cli_verify.c ---> cli_verify
  由 mk_cli_verify.sh 生成

2. 必须先执行mk_rsa_lib.sh 生成librsa.so， 其他的程序均需要链接此库

3. rsa_key_gen 生成 s_pri.key, s_pub.key 两个文件， s_pub.key是要复制到设备端的

4. cli_reg 程序，需要读取s_pub.key文件， 输出c_device.reg , c_pri.key 文件

5. srv_build_key 程序， 需要读取 c_device.reg , s_pri.key , 输出 c_device.key 文件

6. cli_verify 程序， 读取 c_device.key ，比较当前机器的硬件信息是否匹配。

7. 编译和运行，均需要有openssl库的支持。(apache开源协议)

8. cli_reg/cli_verify运行的时候，需要有root权限，需要有dmidecode/grep 读取硬件信息。

