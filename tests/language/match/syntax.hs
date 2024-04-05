var x = 0;

match (x) {
  case 0 => print("case 1"); // expect: case 1
  else => print("else");
}

print("ok"); // expect: ok
