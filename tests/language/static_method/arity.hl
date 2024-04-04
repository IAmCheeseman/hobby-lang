struct Foo {
  static func method0() => "no args";
  static func method1(a) => a;
  static func method2(a, b) => a + b;
  static func method3(a, b, c) => a + b + c;
  static func method4(a, b, c, d) => a + b + c + d;
  static func method5(a, b, c, d, e) => a + b + c + d + e;
  static func method6(a, b, c, d, e, f) => a + b + c + d + e + f;
  static func method7(a, b, c, d, e, f, g) => a + b + c + d + e + f + g;
  static func method8(a, b, c, d, e, f, g, h) => a + b + c + d + e + f + g + h;
}

print(Foo:method0()); // expect: no args
print(Foo:method1(1)); // expect: 1
print(Foo:method2(1, 2)); // expect: 3
print(Foo:method3(1, 2, 3)); // expect: 6
print(Foo:method4(1, 2, 3, 4)); // expect: 10
print(Foo:method5(1, 2, 3, 4, 5)); // expect: 15
print(Foo:method6(1, 2, 3, 4, 5, 6)); // expect: 21
print(Foo:method7(1, 2, 3, 4, 5, 6, 7)); // expect: 28
print(Foo:method8(1, 2, 3, 4, 5, 6, 7, 8)); // expect: 36
