var f;

func foo(param) {
  f = func() {
    print(param);
  };
}
foo("param");

f(); // expect: param
