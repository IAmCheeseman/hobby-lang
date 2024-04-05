func f() {
  while(true) {
    var i = "i";
    return func() { print(i); };
  }
}

var h = f();
h(); // expect: i
