func f() {
  return "ok";
  print("bad");
}

print(f()); // expect: ok
