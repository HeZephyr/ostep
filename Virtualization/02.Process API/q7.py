import os

def main():
    pid = os.fork()
    if pid == 0:
        # This is the child process
        # Close standard output (STDOUT FILENO)
        os.close(1)
        # Try to print output using printf()
        os.system("printf 'Child process: Hello, world!'")
    else:
        # This is the parent process
        print("Parent process waiting for child to finish")
        # Wait for the child process to finish
        os.wait()
        print("Child process finished")

if __name__ == "__main__":
    main()
