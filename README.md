# HobbyScript

A dynamically typed programming language designed for use in game development.

## Features
- No OOP
- Designed by me
- Works with Swift syntax highlighting

### Hello, world!
```swift
print("Hello, world!");
```

### Something more substantial
Here's a simple timer, that works given a delta time:
```swift
struct Timer {
  var totalTime;
  var timeLeft;

  static func new(time) {
    return Timer {
      .totalTime = time,
      .timeLeft = 0,
    };
  }

  func start() => self.timeLeft = self.totalTime;
  func step(delta) => self.timeLeft -= delta;
  func isOver() => self.timeLeft < 0;
}

var timer =- Timer:new(15);
timer.start();

loop {
  timer.step(1);
  print(timer.timeLeft);

  if (timer.isOver()) {
    break;
  }
}
```
