
export PKG_CONFIG_PATH=/usr/local/ssl/lib/pkgconfig

1. �ļ�
����ϵͳһ������1��lib, 4����ִ�г���
a. rsa lib
 rsa.c rsa.h ---> librsa.so 
 ��mk_rsa_lib.sh ����
b. client register ����
 cli_reg.c ---> cli_reg
 ��mk_cli_reg.sh ����
c. rsa private/public key pair generator����
 rsa_key_gen.c ---> rsa_key_gen
 ��mk_rsa_key_gen.sh ����
d. server build key ����
 srv_build_key.c ---> srv_build_key.c
  �� mk_srv_build_key.sh ����
e. client verify ���� (���Գ�������)
  cli_verify.c ---> cli_verify
  �� mk_cli_verify.sh ����

2. ������ִ��mk_rsa_lib.sh ����librsa.so�� �����ĳ������Ҫ���Ӵ˿�

3. rsa_key_gen ���� s_pri.key, s_pub.key �����ļ��� s_pub.key��Ҫ���Ƶ��豸�˵�

4. cli_reg ������Ҫ��ȡs_pub.key�ļ��� ���c_device.reg , c_pri.key �ļ�

5. srv_build_key ���� ��Ҫ��ȡ c_device.reg , s_pri.key , ��� c_device.key �ļ�

6. cli_verify ���� ��ȡ c_device.key ���Ƚϵ�ǰ������Ӳ����Ϣ�Ƿ�ƥ�䡣

7. ��������У�����Ҫ��openssl���֧�֡�(apache��ԴЭ��)

8. cli_reg/cli_verify���е�ʱ����Ҫ��rootȨ�ޣ���Ҫ��dmidecode/grep ��ȡӲ����Ϣ��

