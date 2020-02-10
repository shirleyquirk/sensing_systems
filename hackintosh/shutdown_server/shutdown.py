#!/usr/bin/python3
import http.server
import socketserver
import subprocess

PORT = 9999
class Handler(http.server.BaseHTTPRequestHandler):
    def do_GET(s):
        s.send_response(200)
        s.wfile.write(b'Maxwell server page %s' % bytes(s.path,'utf-8'))
        if s.path == "/shutdown":
            s.wfile.write(b': Shutting Down Now')
            subprocess.run(['shutdown','-h','now'])


with socketserver.TCPServer(("",PORT),Handler) as httpd:
    print("serving on port: ",PORT)
    try:
        httpd.serve_forever()
    except KeyboardInterrupt:
        pass
    httpd.server_close()
print("Server closed:")
