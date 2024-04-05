var f;

{
  var local = "local";
  f = func() {
    print(local);
  };
}

f(); // expect: local
