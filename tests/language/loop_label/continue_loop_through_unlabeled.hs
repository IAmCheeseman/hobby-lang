var i = 0;
while (i < 3): outermost {
  while (true) {
    while (true) {
      while (true) {
        print(i);
        i += 1;
        continue outermost;
      }
    }
  }
}

// expect: 0
// expect: 1
// expect: 2

print("ok"); // expect: ok
