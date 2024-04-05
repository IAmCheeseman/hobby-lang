var f;
while(f == nil) {
  var i = "i";
  f = func() => print(i);
  continue;
}

f();
// expect: i
