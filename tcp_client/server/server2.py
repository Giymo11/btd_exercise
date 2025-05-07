import socket
import csv
import time


def chunker(seq, size):
    return (seq[pos:pos + size] for pos in range(0, len(seq), size))


# Define server information
host = '0.0.0.0'  # listen on all available interfaces
port = 9000  # the same port number used in the M5StickC Plus code

# Set up the server socket
s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
s.bind((host, port))
print(f"Connected on {host}:{port}")
s.listen(1)  # accept only one client connection

# Wait for client connection
print(f"Waiting for a client to connect on port {port}...")
conn, addr = s.accept()
print(f"Connected to client at {addr[0]}:{addr[1]}")

# Receive exactly one chunk of data

tup = []
msg=''
termination = 'end'.encode()
st = time.time()
while True:
    data = conn.recv(1024)  # receive up to 1024 bytes of data
    if not data:
        print("no message..")  # connection closed by client
        break
    elif termination in data:
        print("end of transmission..")
        msg= msg + data.decode('utf-8')[:-3]
        break
    else:
        msg = msg + data.decode('utf-8')
        print('chunk:',data.decode('utf-8'))


# get the end time          
et = time.time()        
conn.send("OK".encode('utf-8'))
print(len(msg))
print(msg)
res = [float(idx) for idx in msg.split(', ') if not idx == '']
tup.extend(res)
conn.send("Bye!".encode('utf-8'))
conn.close()
s.close()
a=0
# Edit path if needed!!
with open('./values.csv', 'w',newline = '') as fp:
            writer = csv.writer(fp)
            for group in chunker(tup,3):
                a = a+1
                print(group)
                writer.writerow(group)
print('Done')
# get the execution time
elapsed = et - st
perMessage = elapsed/a
print (a, 'messages in approx.',elapsed,'s')
freq = 1/perMessage
print(freq)