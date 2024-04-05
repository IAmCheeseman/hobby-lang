func f(arg) {
  return arg;
}

func g(fn, arg) {
  return f(fn)(arg);
}

func printer(arg) {
  print(arg);
}

g(printer, "hello world"); // expect: hello world
