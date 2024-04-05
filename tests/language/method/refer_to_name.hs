struct Foo {
  func method() {
    print(method); // expect runtime error: Undefined variable 'method'
  }
}

Foo {}.method();
