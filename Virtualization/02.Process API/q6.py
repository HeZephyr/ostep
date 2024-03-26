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
        # Use waitpid() instead of wait()
        try:
            pid, status = os.waitpid(pid, 0)
            print("Child process with PID", pid, "finished with status", status)
            if os.WIFEXITED(status):
                print("Child exited normally")
                print("Exit status of child:", os.WEXITSTATUS(status))
            elif os.WIFSIGNALED(status):
                print("Child terminated by signal")
                print("Terminating signal:", os.WTERMSIG(status))
        except OSError as e:
            print(f"Error in parent process: {e}")

if __name__ == "__main__":
    main()