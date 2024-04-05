struct Foo {
  func method() {
    return "ok";
    print("bad");
  }
}

print(Foo {}.method()); // expect: ok
