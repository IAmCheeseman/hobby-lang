var f;
while (true) {
    var i = "i";
    f = func() => print(i);
    break;
}

f(); // expect: i
