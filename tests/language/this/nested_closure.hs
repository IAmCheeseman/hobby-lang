struct Foo {
  func getClosure() {
    var f = func() {
      var g = func() {
        var h = func() {
          return self.toString();
        };
        return h;
      };
      return g;
    };
    return f;
  }

  func toString() => "Foo";
}

var closure = Foo {}.getClosure();
print(closure()()()); // expect: Foo
