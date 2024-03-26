import os

def main():
    # set initial value
    x = 100
    # call fork(), create child process
    pid = os.fork()
    if pid > 0:
        # parent process
        print(f"Parent process - x: {x}")
    elif pid == 0:
        # child process
        print(f"Child process - x: {x}")
    else:
        # fork() error
        print("Fork failed")

    if pid > 0:
        x += 50
        print(f"Parent process - x after modification: {x}")
    elif pid == 0:
        x -= 50
        print(f"Child process - x after modification: {x}")

if __name__ == "__main__":
    main()