print(false && "bad"); // expect: false
print(nil && "bad"); // expect: nil
print(true && "ok"); // expect: ok
print(0 && "ok"); // expect: ok
print("" && "ok"); // expect: ok
