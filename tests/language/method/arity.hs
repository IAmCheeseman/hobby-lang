struct Foo {
  func method0() => "no args";
  func method1(a) => a;
  func method2(a, b) => a + b;
  func method3(a, b, c) => a + b + c;
  func method4(a, b, c, d) => a + b + c + d;
  func method5(a, b, c, d, e) => a + b + c + d + e;
  func method6(a, b, c, d, e, f) => a + b + c + d + e + f;
  func method7(a, b, c, d, e, f, g) => a + b + c + d + e + f + g;
  func method8(a, b, c, d, e, f, g, h) => a + b + c + d + e + f + g + h;
}

var foo = Foo {};
print(foo.method0()); // expect: no args
print(foo.method1(1)); // expect: 1
print(foo.method2(1, 2)); // expect: 3
print(foo.method3(1, 2, 3)); // expect: 6
print(foo.method4(1, 2, 3, 4)); // expect: 10
print(foo.method5(1, 2, 3, 4, 5)); // expect: 15
print(foo.method6(1, 2, 3, 4, 5, 6)); // expect: 21
print(foo.method7(1, 2, 3, 4, 5, 6, 7)); // expect: 28
print(foo.method8(1, 2, 3, 4, 5, 6, 7, 8)); // expect: 36
