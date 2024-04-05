var f;

{
    var a = "a";
    f = func() {
        print(a);
        print(a);
    };
}

f();
// expect: a
// expect: a
