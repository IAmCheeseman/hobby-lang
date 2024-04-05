var i = 0;
while (i < 3): outer {
  while (true): middle {
    print("middle");
    while (true): inner {
      i += 1;
      print("inner");
      continue outer;
    }
  }
  print("outer");
}

// expect: middle
// expect: inner
// expect: middle
// expect: inner
// expect: middle
// expect: inner

print("ok"); // expect: ok
