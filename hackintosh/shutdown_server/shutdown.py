#!/usr/bin/python3
import http.server
import socketserver
import subprocess
import time

def flash():
    subprocess.run(['bash','-c','echo -en /top/enable\\\\x00\\\\x00 > /dev/udp/192.168.8.143/8888'])
    time.sleep(1)
    subprocess.run(['bash','-c','echo -en /top/disable\\\\x00\\\\x00 > /dev/udp/192.168.8.143/8888'])

PORT = 9999
class Handler(http.server.BaseHTTPRequestHandler):
    def do_GET(s):
        s.send_response(200)
        s.wfile.write(b'Maxwell server page %s' % bytes(s.path,'utf-8'))
        if s.path == "/shutdown":
            s.wfile.write(b': Shutting Down Now')
            subprocess.run('ssh pi@venus \"sudo shutdown now\"',shell=True)
            subprocess.run('ssh pi@mars \"sudo shutdown now\"',shell=True)
            subprocess.run('ssh pi@jupiter \"sudo shutdown now\"',shell=True)
            subprocess.run(['shutdown','-h','now'])
        if s.path == "/restart":
            s.wfile.write(b': Restarting')
            subprocess.run(['shutdown','-r','now'])  
        if s.path == "/reset_pyramid":
            s.wfile.write(b': Resetting Pyramid\n')
            ret=subprocess.run('ssh pi@raspberrypi \"python3 ~/reset.py\"',shell=True,stdout=subprocess.PIPE)
            s.wfile.write(ret.stdout)
            time.sleep(1)
            flash()
        if s.path == "/flash_top":
            flash()
with socketserver.TCPServer(("",PORT),Handler) as httpd:
    print("serving on port: ",PORT)
    try:
        httpd.serve_forever()
    except KeyboardInterrupt:
        pass
    httpd.server_close()
print("Server closed:")
