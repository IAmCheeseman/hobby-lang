struct Foo {
  func bar() => self;
  func baz() => "baz";
}

print(Foo {}.bar().baz()); // expect: baz
