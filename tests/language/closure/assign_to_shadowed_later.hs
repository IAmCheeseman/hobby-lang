var a = "global";

{
  var assign = func() {
    a = "assigned";
  };

  var a = "inner";
  assign();
  print(a); // expect: inner
}

print(a); // expect: assigned
