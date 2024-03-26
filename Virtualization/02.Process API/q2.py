import os

def main():
    # open a file
    script_dir = os.path.dirname(os.path.abspath(__file__))
    file_path = os.path.join(script_dir, "test.txt")
    file = open(file_path, "w")

    # call fork() to create child process
    pid = os.fork()

    if pid > 0:
        # parent process
        print("Parent process is writing to the file...")
        file.write("Hello from parent process!\n")
    elif pid == 0:
        # child process
        print("Child process is writing to the file...")
        file.write("Hello from child process!\n")
    else:
        # fork() error
        print("Fork failed")

    # close the file
    file.close()

if __name__ == "__main__":
    main()
