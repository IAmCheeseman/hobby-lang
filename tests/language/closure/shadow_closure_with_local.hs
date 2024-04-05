{
  var foo = "closure";
  var f = func() {
    {
      print(foo); // expect: closure
      var foo = "shadow";
      print(foo); // expect: shadow
    }
    print(foo); // expect: closure
  };
  f();
}
