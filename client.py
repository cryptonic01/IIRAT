# -*- coding: utf-8 -*-
# @Author: amaneureka
# @Date:   2017-04-01 22:39:50
# @Last Modified by:   amaneureka
# @Last Modified time: 2017-04-16 11:07:24

import sys
import socket
import select
import time
import configparser

def start_client():
    server_config = config['Server']

    HOST = server_config['HOST']
    PORT = server_config.getint('PORT')
    BUFFER_SIZE = server_config.getint('BUFFER_SIZE')

    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    s.settimeout(2)

    try:
        s.connect((HOST, PORT))
    except Exception as error:
        print 'Unable to connect', error
        return

    print 'Server Connected :)'

    SOCKET_LIST = [sys.stdin, s]
    length = 0
    output = sys.stdout
    while True:

        ready_to_read, _, _ = select.select(SOCKET_LIST, [], [])

        for sock in ready_to_read:

            if sock == s:

                data = sock.recv(BUFFER_SIZE)
                if not data:
                    print '\ndisconnected from the server'
                    return

                if data[:3] == 'RSP':
                    data = data[10:]
                output.write(data)
                output.flush()
                if output != sys.stdout:
                    sys.stdout.write('.')
                    sys.stdout.flush()

            else:

                msg = sys.stdin.readline().strip()
                if msg[:2] == 'PF':
                    output = open('data/scr_%s.jpg' % str(time.strftime("%Y%m%d-%H%M%S")), 'w')
                elif msg[:2] == 'SF':
                    output = sys.stdout
                else:
                    s.send(msg)

if __name__ == '__main__':
    config = configparser.ConfigParser()
    config.read('config.ini')
    sys.exit(start_client())
