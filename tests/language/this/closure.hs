struct Foo {
  func getClosure() {
    var closure = func() {
      return self.toString();
    };
    return closure;
  }

  func toString() => "Foo";
}

var closure = Foo {}.getClosure();
print(closure()); // expect: Foo
