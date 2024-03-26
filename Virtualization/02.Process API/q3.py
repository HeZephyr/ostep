import os
import time

def main():
    pid = os.fork()
    
    if pid == 0:
        # Child process
        print("hello")
    else:
        # Parent process
        # Sleep for a short time to allow the child process to execute first
        time.sleep(0.1)
        print("goodbye")

if __name__ == "__main__":
    main()
