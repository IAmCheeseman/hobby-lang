import time


def fib(n):
    if n < 2:
        return n
    return fib(n - 2) + fib(n - 1)


start = time.time()

for i in range(0, 5):
    print(fib(35))
print("Time:", time.time() - start)

