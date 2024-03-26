import os

def main():
    pid = os.fork()
    if pid == 0:
        # This is the child process
        print("Child process executing /bin/ls using execl()")
        os.execl("/bin/ls", "ls", "-l")  # execl() variant
        print("This line should not be reached")  # This line will not be executed if execl() is successful

        # Uncomment each block to test other variants of exec functions
        # print("Child process executing /bin/ls using execle()")
        # os.execle("/bin/ls", "ls", "-l", os.environ)  # execle() variant
        
        # print("Child process executing /bin/ls using execlp()")
        # os.execlp("ls", "ls", "-l")  # execlp() variant

        # print("Child process executing /bin/ls using execv()")
        # os.execv("/bin/ls", ["ls", "-l"])  # execv() variant
        
        # print("Child process executing /bin/ls using execvp()")
        # os.execvp("ls", ["ls", "-l"])  # execvp() variant
        
        # print("Child process executing /bin/ls using execvpe()")
        # os.execvpe("ls", ["ls", "-l"], os.environ)  # execvpe() variant
    else:
        # This is the parent process
        print("Parent process continuing execution")

if __name__ == "__main__":
    main()
