// False and nil are false
print(false || "ok"); // expect: ok
print(nil || "ok"); // expect: ok

// Everything else is true
print(true || "ok"); // expect: true
print(0 || "bad"); // expect: 0
print("ok" || "bad"); // expect: ok
