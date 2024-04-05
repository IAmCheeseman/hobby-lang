var done = false;
while(!done) {
  var f = func() {
    continue; // expect error
  };
  done = true;
}
