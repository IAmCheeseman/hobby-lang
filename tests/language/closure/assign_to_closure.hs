var f;
var g;

{
  var local = "local";
  f = func() {
    print(local);
    local = "after f";
    print(local);
  };

  g = func() {
    print(local);
    local = "after g";
    print(local);
  };
}

f();
// expect: local
// expect: after f

g();
// expect: after f
// expect: after g
