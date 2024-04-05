while (true): outer {
  while (true): middle {
    while (true): inner {
      print("inner"); // expect: inner
      break middle;
    }
    print("middle");
  }
  print("outer"); // expect: outer
  break;
}

print("ok"); // expect: ok
