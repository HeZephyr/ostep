import os

def main():
    # create pipe
    r, w = os.pipe()

    # create the first child process
    pid1 = os.fork()
    if pid1 == 0:
        # first child process
        # close read end of the pipe
        os.close(r)
        # duplicate the write end of the pipe to the standard output
        os.dup2(w, 1)
        # execute a command or program (here, as an example, executing the echo command)
        os.execlp("echo", "echo", "Hello, world!")
    else:
        # create the second child process
        pid2 = os.fork()
        if pid2 == 0:
            # second child process
            # close write end of the pipe
            os.close(w)
            # duplicate the read end of the pipe to the standard input
            os.dup2(r, 0)
            print("From Child Process:")
            # execute a command or program (here, as an example, executing the cat command)
            os.execlp("cat", "cat")
        else:
            # parent process
            # close both ends of the pipe
            os.close(r)
            os.close(w)
            # wait for both child processes to finish
            os.waitpid(pid1, 0)
            os.waitpid(pid2, 0)

if __name__ == "__main__":
    main()
