import socket
s = socket.socket()
s.connect(('localhost', 6379))
msg = "ok"
while True:
    # s.sendall(b'*3\r\n$3\r\nset\r\n$4\r\nheya\r\n$4\r\nheya\r\n$2\r\npx\r\n$3\r\n100\r\n')
    # s.sendall(b'*3\r\n$3\r\nset\r\n$4\r\nheya\r\n$4\r\nheya\r\n')
    s.sendall(b'*2\r\n$3\r\nget\r\n$4\r\nheya\r\n')
    print(s.recv(1024))
    msg = input()