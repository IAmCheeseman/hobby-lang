enum AttackType {
  Melee,
  Magic,
  Ranged,
  Env,
}

func getAttackTypeString(type) {
  match (type) {
    case AttackType:Melee => return "Melee";
    case AttackType:Magic => return "Magic";
    case AttackType:Ranged => return "Ranged";
    case AttackType:Env => return "Env";
    else => error("Invalid attack type.");
  }
}

struct Attack {
  var type;
  var damage;
  var piercing;

  static func new(type, damage, piercing) => Attack {
    .type = type,
    .damage = damage,
    .piercing = piercing,
  };

  func displayStats() {
    print("Type:\t\t" .. getAttackTypeString(self.type));
    print("Damage:\t\t" .. toString(self.damage));
    print("Piercing:\t" .. toString(self.piercing * 100) .. "%");
  }
}

var goblinAttack = Attack:new(AttackType:Melee, 12, 0.3);
var wizardAttack = Attack:new(AttackType:Magic, 16, 0.0);

print("-- Goblin --");
goblinAttack.displayStats();
print("\n-- Wizard --");
wizardAttack.displayStats();
