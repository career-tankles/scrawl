#!/usr/bin/env python
#coding=utf8

import os, sys, time
import pexpect 
import getopt, getpass
import imp

N0_TIMEOUT = pexpect.TIMEOUT
N1_EOF = pexpect.EOF
N2_password = '.ssword:*'
N3_are_you_sure_to_continue = 'Are you sure you want to continue connecting (yes/no)?'
N4_passwd_try_again = 'Permission denied, please try again'
N5_rm_regular_file = 'remove write-protected regular empty file*?'
N6_rm_dir_makesure = 'descend into write-protected directory*?'
N7_rsa_enter_key = '.*Enter file in which to save the key.*'
N8_rsa_enter_passphrase = '.*Enter passphrase.*'
N9_rsa_enter_same_passphrase = 'Enter same passphrase again'
N10_rsa_overwrite = 'Overwrite (y/n)?'

def super_execute(cmd, timeout=600):
    global debug_mode
    global password
    if cmd == None or len(cmd) <= 0:
        return
    #print 'cmd: %s' % cmd
    print '\n\n\033[32;49;1m %s \033[0m' % (cmd)
    child = pexpect.spawn(cmd)
    child.logfile_read = sys.stdout
    if debug_mode:
        child.logfile_send = sys.stdout
    while True:
        i = child.expect([N0_TIMEOUT, N1_EOF, N2_password, N3_are_you_sure_to_continue, N4_passwd_try_again,
            N5_rm_regular_file, N6_rm_dir_makesure, N7_rsa_enter_key, N8_rsa_enter_passphrase, N9_rsa_enter_same_passphrase, N10_rsa_overwrite,
            ], timeout=timeout)
        if i == 0:
            print 'TIMEOUT'
            break
        elif i == 1:
            break
        elif i == 2:
            if password == '' or password == None:
                password = getpass.getpass('')
            child.sendline(password)  
        elif i == 4:
            password = getpass.getpass('')
            child.sendline(password)  
        elif i == 3 or i == 5 or i == 6:
            child.sendline('yes')
        elif i == 7 or i == 8 or i == 9:
            child.sendline('')
        elif i == 10:
            child.sendline('n') # not overwrite, use it

    #print child.before
    print 'remote execute finished!'
    return True

# 登录到目标机器，执行指定的脚本
def remote_execute_cmd_file(host, cmd_file, username=None):
    if username == None:
        username = getpass.getuser() # default login user
    rsync_cmd_file = 'sudo rsync -auvlz %s %s@%s:/tmp/%s' % (cmd_file, username, host, cmd_file)
    remote_exec_cmd = 'ssh -t %s@%s "sudo chmod +x /tmp/%s; sudo /tmp/%s; "' % (username, host, cmd_file, cmd_file)

    ret = super_execute(rsync_cmd_file)
    if ret:
        ret = super_execute(remote_exec_cmd)
#        if ret:
#            print 'remote execute successful!'
#        else:
#            print 'remote execute failed!'
#    else:
#        print 'rsync %s to %s failed!' % (cmd_file, host)

"""
    # sudo sh -c "cmds"
    #ssh -t -p $port $user@$ip 'cmds'
"""

# s1,s2,s3
def parse_host(host_str):
    hosts = host_str.split(',')
    return hosts
# one host each line
def load_hosts(host_file):
    hosts = []
    for line in file(host_file):
        line = line.strip()
        if line.startswith('#'):
            continue
        if len(line) <= 0:
            continue
        host = line
        hosts.append(host)
    return hosts

def usage():
    usage_ = '''
       %s [options]

            options:
                -h --help
                --servers=host1,host2,...
                --host_file=host.txt
                --cmd_file=cmd.sh
                --user=wangfengliang
                --passwd
                --rsync_src_dir_or_file
                --rsync_dst_dir
            samples:
                # 执行指定的命令or命令文件
                ./super_remote_cmds.py --cmds="ls -l "
                ./super_remote_cmds.py --cmd_file=cmd.sh

                # 在127.0.0.1和127.0.0.2机器上执行命令or命令文件
                ./super_remote_cmds.py --servers=10.16.29.89,10.125.117.79 --cmds="ls -l /da1/msearch" 
                ./super_remote_cmds.py --servers=127.0.0.1,127.0.0.2 --cmd_file=cmd.sh

                # 在hosts.txt中指定的机器上，执行命令or命令文件
                ./super_remote_cmds.py --host_file=host.txt  --cmds="ls -l /da1/msearch" 
                ./super_remote_cmds.py --host_file=host.txt --cmd_file=cmd.sh

                # 同步文件or文件夹
                ./super_remote_cmds.py --rsync_src_dir_or_file=a.txt --rsync_dst_dir=/tmp/
                ./super_remote_cmds.py --servers=10.16.29.89,10.125.117.79 --rsync_src_dir_or_file=a.txt --rsync_dst_dir=/tmp/
                ./super_remote_cmds.py --host_file=host.txt --rsync_src_dir_or_file=a.txt --rsync_dst_dir=/tmp/
                
                 # 注意下面2个的区别(test is directory)
                ./super_remote_cmds.py --rsync_src_dir_or_file=test --rsync_dst_dir=/tmp/  # 同步test文件夹到/tmp/下
                ./super_remote_cmds.py --rsync_src_dir_or_file=test/ --rsync_dst_dir=/tmp/ # 同步test文件夹下的文件到/tmp/下
    
          ''' % (sys.argv[0])
    print usage_

password = None
debug_mode = False
if __name__ == "__main__":

    host_file = None
    cmd_file = None
    servers = None
    cmds = None
    rsync_src_dir_or_file = None
    rsync_dst_dir = None
    auto_ssh_mode = False
    auto_ssh_mode = False
    auto_ssh_rsa_file = None
    auto_ssh_username = None
    auto_ssh_user_home = None

    username = getpass.getuser() # default login user
    print '\033[32;49;1m username: %s \033[0m' % (username)

    try:
        opts, args = getopt.getopt(sys.argv[1:], 'h', ['help', 'host_file=', 'servers=', 'user=', 'passwd', 'cmd_file=', 'cmds=', 'debug', 'rsync_src_dir_or_file=', 'rsync_dst_dir=', 'passwd_nosecure=', 'auto_ssh', 'auto_ssh_rsa_file=', 'auto_ssh_username=', 'auto_ssh_user_home='])
        for o, a in opts:
            if o in ('-h', '--help'):
                usage()
                sys.exit(0)
            elif o in ('--host_file'):
                host_file = a
            elif o in ('--servers'):
                servers = a
            elif o in ('--cmds'):
                cmds= a
            elif o in ('--cmd_file'):
                cmd_file = a
            elif o in ('--rsync_src_dir_or_file'):
                rsync_src_dir_or_file = a
            elif o in ('--rsync_dst_dir'):
                rsync_dst_dir = a
            elif o in ('-u', '--user'):
                username = a
            elif o in ('--debug'):
                debug_mode = True
            elif o in ('--auto_ssh'):
                auto_ssh_mode = True
            elif o in ('--auto_ssh_rsa_file'):
                auto_ssh_rsa_file = a
            elif o in ('--auto_ssh_username'):
                auto_ssh_username = a
            elif o in ('--auto_ssh_user_home'):
                auto_ssh_user_home = a
            elif o in ('-p', '--passwd'):
                password = getpass.getpass('password: ')
            elif o in ('--passwd_nosecure'):
                password = a
    except getopt.GetoptError, e:
        print '\033[32;49;1m username: Exception: %s \033[0m' % (str(e))
        sys.exit(1)

    # 在指定的目标机器上执行cmds命令
    if host_file and cmds:
        hosts = load_hosts(host_file)
        for host in hosts:
            remote_cmds = '''ssh -t %s "sh -c '%s'"''' % (host, cmds)
            ret = super_execute(remote_cmds)
#            if ret:
#                print 'execute successfully!!!'
#            else:
#                print 'Error: execute failed!!!'
    elif servers and cmds:
        hosts = parse_host(servers)
        for host in hosts:
            remote_cmds = '''ssh -t %s "sh -c '%s'"''' % (host, cmds)
            ret = super_execute(remote_cmds)
#            if ret:
#                print 'execute successfully!!!'
#            else:
#                print 'Error: execute failed!!!'

    # 执行指定shell命令
    elif cmds:
        remote_cmds = '''sh -c \'%s\'''' % cmds
        ret = super_execute(remote_cmds)
#        if ret:
#            print 'execute successfully!!!'
#        else:
#            print 'Error: execute failed!!!'

    # 指定hosts和脚本文件，到目标机器执行
    elif host_file and cmd_file:
        hosts = load_hosts(host_file)
        for host in hosts:
            remote_execute_cmd_file(host, cmd_file, username)
    elif servers and cmd_file:
        hosts = parse_host(servers)
        for host in hosts:
            remote_execute_cmd_file(host, cmd_file, username)

    # 只指定了脚本文件，则执行脚本文件
    elif cmd_file:
        rsync_cmd_file = 'sudo rsync -auvlz %s /tmp/%s' % (cmd_file, cmd_file)
        remote_exec_cmd = 'sudo sh -c "chmod +x /tmp/%s"' % (cmd_file)
        remote_exec_cmd2 = 'sudo sh -c "/tmp/%s";' % (cmd_file)
        ret = super_execute(rsync_cmd_file)
        ret = super_execute(remote_exec_cmd)
        ret = super_execute(remote_exec_cmd2)

    # rsync同步文件
    elif rsync_src_dir_or_file and rsync_dst_dir:
        if host_file or servers:
            host = []
            if host_file:
                hosts = load_hosts(host_file)
            elif servers:
                hosts = parse_host(servers)
            for host in hosts:
                #rsync_cmd = 'rsync -avlz %s %s@%s:%s' % (rsync_src_dir_or_file, username, host, rsync_dst_dir)
                rsync_cmd = 'sudo rsync -avlz %s %s@%s:%s' % (rsync_src_dir_or_file, username, host, rsync_dst_dir)
                ret = super_execute(rsync_cmd)
#                if ret:
#                    print 'execute successfully!!!'
#                else:
#                    print 'Error: execute failed!!!'
        else:
            #rsync_cmd = 'rsync -avlz %s %s' % (rsync_src_dir_or_file, rsync_dst_dir)
            rsync_cmd = 'sudo rsync -avlz %s %s' % (rsync_src_dir_or_file, rsync_dst_dir)
            ret = super_execute(rsync_cmd)
#            if ret:
#                print 'execute successfully!!!'
#            else:
#                print 'Error: execute failed!!!'
    elif auto_ssh_mode:
        if not auto_ssh_rsa_file:
            print 'auto_ssh must given id_rsa.pub file'
            sys.exit(1)
        if not auto_ssh_username:
            auto_ssh_username = username
        if not auto_ssh_user_home:
            auto_ssh_user_home = '/home/%s' % auto_ssh_username 

        rsa_pub_content = ''
        try: 
            rsa_pub_content = file(auto_ssh_rsa_file).read()
        except Exception, e:
            #print 'read file: %s failed, %s' % (auto_ssh_rsa_file, str(e))
            print '\033[32;49;1m read file: %s failed, %s \033[0m' % (auto_ssh_rsa_file, str(e))
            # 提升到root权限
            if len(rsa_pub_content) <= 0 and os.geteuid():
                args = [sys.executable] + sys.argv
                # 下面两种写法，一种使用su，一种使用sudo，都可以
                #os.execlp('su', 'su', '-c', ' '.join(args))
                os.execlp('sudo', 'sudo', *args)
                try:
                    rsa_pub_content = file(auto_ssh_rsa_file).read()
                except Exception, e:
                    print 'sudo read file failed, %s' % str(e)
            elif len(rsa_pub_content) > 0:
                print 'RSA-DATA: %s' % rsa_pub_content

        if len(rsa_pub_content) <= 0:
            print 'read id_rsa.pub conent is empty, exit'
            sys.exit(2)
        cmd_file = 'auto_ssh_remote_cmd.sh'
        remote_cmd_file_content = r'''
#!/bin/sh

username="%s"
userhome="%s"
#id_rsa_pub="/tmp/id_rsa.pub"
id_rsa_pub_content="%s"

sudo mkdir -p $userhome/.ssh
sudo touch $userhome/.ssh/authorized_keys
#sudo sh -c "cat $id_rsa_pub >> $userhome/.ssh/authorized_keys"
sudo sh -c "echo \"$id_rsa_pub_content\" >> $userhome/.ssh/authorized_keys"

sudo chmod 755 $userhome
sudo chmod 700 $userhome/.ssh
sudo chmod 600 $userhome/.ssh/authorized_keys
sudo chown -R $username.$username $userhome/.ssh

        ''' % (auto_ssh_username, auto_ssh_user_home, rsa_pub_content)
        file(cmd_file, 'w').write(remote_cmd_file_content)

        if host_file or servers:
            host = []
            if host_file:
                hosts = load_hosts(host_file)
            elif servers:
                hosts = parse_host(servers)
            for host in hosts:
                remote_execute_cmd_file(host, cmd_file, username)
        else:
            remote_exec_cmd = 'sudo sh /tmp/%s' % (cmd_file)
            ret = super_execute(remote_exec_cmd)
#            if ret:
#                print 'execute successfully!!!'
#            else:
#                print 'Error: execute failed!!!'
        cmd_file = None
    else:
        usage()

