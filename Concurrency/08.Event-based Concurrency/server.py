import socket
import datetime

def handle_connection(client_socket):
    # 获取当前时间并格式化为字符串
    current_time = datetime.datetime.now().strftime('%Y-%m-%d %H:%M:%S')
    
    # 将当前时间发送给客户端
    client_socket.send(current_time.encode())
    
    # 关闭客户端套接字
    client_socket.close()

def main():
    # 服务器主机和端口
    host = '127.0.0.1'
    port = 8080

    # 创建TCP套接字
    server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    
    # 绑定主机和端口
    server_socket.bind((host, port))
    
    # 监听连接，最大连接数为5
    server_socket.listen(5)

    # 打印服务器信息
    print(f"Server listening on {host}:{port}")

    while True:
        # 接受客户端连接
        client_socket, addr = server_socket.accept()
        
        # 打印连接信息
        print(f"Connection from {addr}")
        
        # 处理连接
        handle_connection(client_socket)

    # 关闭服务器套接字
    server_socket.close()

if __name__ == "__main__":
    main()
