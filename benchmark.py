import timeit

HL_APP = './bin/hl_release'
BENCHMARK_PATH = 'benchmark/'

ITERATE_COUNT = 5

LANGUAGES = [
    ("hobby", "./bin/hl_release", ".hl"),
    ("lua", "lua", ".lua"),
    ("python", "python3", ".py"),
]

BENCHMARKS = [
    "fib",
]

times = {}


def run_script(language, benchmark):
    global times

    print("---")
    print(language[0].upper() + ":")
    results = []
    path = BENCHMARK_PATH + benchmark + language[2]
    for i in range(ITERATE_COUNT):
        t = timeit.Timer(
            f"run(['{language[1]}', '{path}'])",
            setup="from subprocess import run")
        results.append(t.timeit(1))

    sum = 0
    for i in results:
        sum += i
    times[language[0]] = sum / len(results)


def run_benchmark(benchmark):
    global times

    for i in LANGUAGES:
        run_script(i, benchmark)

    print(f"{benchmark.upper()} RESULTS")
    for i in LANGUAGES:
        print(f"{i[0].title()} average:\t{round(times[i[0]] * 1000)}ms")
    times = {}


run_benchmark("fib")

