var f;

func f1() {
  var a = "a";
  var f2 = func() {
    var b = "b";
    var f3 = func() {
      var c = "c";
      f = func() {
        print(a);
        print(b);
        print(c);
      };
    };
    f3();
  };
  f2();
}
f1();

f();
// expect: a
// expect: b
// expect: c
