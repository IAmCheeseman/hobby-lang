struct Foo {
  func this() {
    return Foo;
  }
}

print(Foo {}.this()); // expect: <Foo>
