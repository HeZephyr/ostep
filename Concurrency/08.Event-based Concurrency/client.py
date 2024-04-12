import socket

def main():
    # 服务器主机和端口
    host = '127.0.0.1'
    port = 8080

    # 创建TCP套接字
    client_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)

    # 连接到服务器
    client_socket.connect((host, port))

    # 接收服务器发送的当前时间
    current_time = client_socket.recv(1024).decode()

    # 打印当前时间
    print(f"Current Time: {current_time}")

    # 关闭客户端套接字
    client_socket.close()

if __name__ == "__main__":
    main()
