var f;

struct Foo {
  func method(param) {
    f = func() {
      print(param);
    };
  }
}

Foo {}.method("param");
f(); // expect: param
