var foo = "variable";

struct Foo {
  func method() {
    print(foo);
  }
}

Foo {}.method(); // expect: variable
