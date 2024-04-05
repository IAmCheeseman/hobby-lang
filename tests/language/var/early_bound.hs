var a = "outer";
{
  var foo = func() {
    print(a);
  };

  foo(); // expect: outer
  var a = "inner";
  foo(); // expect: outer
}
