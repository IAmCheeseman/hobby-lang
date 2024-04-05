struct Foo {
  static func new() => Foo {};
  func bar() {
    self = "value"; // expect error line 4
  }
}

Foo:new().bar();
