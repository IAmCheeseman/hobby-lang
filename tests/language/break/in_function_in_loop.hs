var done = false;

while (!done) {
  var f = func() {
    break; // expect error
  };
  done = true;
}
