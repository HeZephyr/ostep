import os

def main():
    pid = os.fork()
    if pid == 0:
        # This is the child process
        print("Child process executing")
        # os.wait()
        print("Child process exiting")
        exit(123)  # Exiting with status code 123
    else:
        # This is the parent process
        print("Parent process waiting for child to finish")
        # Wait for the child process to finish and get its status
        pid, status = os.wait()
        print("Child process with PID", pid, "finished with status", status)
        # Check if the child process exited normally or was terminated by a signal
        if os.WIFEXITED(status):
            print("Child exited normally")
            print("Exit status of child:", os.WEXITSTATUS(status))
        elif os.WIFSIGNALED(status):
            print("Child terminated by signal")
            print("Terminating signal:", os.WTERMSIG(status))

if __name__ == "__main__":
    main()