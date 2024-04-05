struct Foo {
  func method() { }
}
var foo = Foo {};
print(foo.method); // expect: <function method>
