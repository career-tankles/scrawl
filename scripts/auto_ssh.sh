#!/bin/sh

user=$(id -un)
username=$user
userhome="/home/$username"
id_rsa_pub_file="/home/$username/.ssh/id_rsa.pub"
servers=10.16.29.89,10.125.117.79
host_file=host.txt

sudo test -f "$id_rsa_pub_file" && {
        ./super_remote_cmds.py --cmds="sudo -u $username sh -c \"ssh-keygen -t rsa\""
}

sudo test -f "$id_rsa_pub_file" && {
    ./super_remote_cmds.py --auto_ssh --user=$user --host_file=$host_file --auto_ssh_username=$username --auto_ssh_user_home=$userhome --auto_ssh_rsa_file=$id_rsa_pub_file
    # 验证一下, 没有提示输入密码，则表示成功
    ./super_remote_cmds.py --host_file=host.txt --cmds="ls -l /tmp"

    #./super_remote_cmds.py --user=$user --servers=$servers --auto_ssh --auto_ssh_username=$username --auto_ssh_user_home=$userhome --auto_ssh_rsa_file=$id_rsa_pub_file
}
