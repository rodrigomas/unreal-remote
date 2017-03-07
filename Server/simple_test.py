'''
Created on Mar 4, 2017

@author: rodri
'''

import socket

def main():
    
    
    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    server_address = ('127.0.0.1', 10000)
    
    print('connecting to %s port %s' % server_address)
    
    sock.connect(server_address)
    
    try:     
        
        while(True):       
            #strsss = input("data>")
            strsss = raw_input("data>")
                    
            if(strsss == "exit"):
                break
                
            b = bytearray()
            b.extend(map(ord, strsss))
                    
            sock.send(b)
            
            data = None
            
            try:
                sock.settimeout(0.1)
            
                data = sock.recv(1000)
            except:
                pass
            
            if( not (data == None) and len(data) > 0):
                print('received "%s"' % data)    
    finally:
        sock.close()

if __name__ == '__main__':
    main()