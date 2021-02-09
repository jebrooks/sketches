void setup() {

  delay(6000);
  Serial.begin(115200);

  Serial.println("setting pinmodes...");
  pinMode(0, OUTPUT);
  pinMode(2, OUTPUT);
  pinMode(4, OUTPUT);
  pinMode(5, OUTPUT);
  pinMode(12, OUTPUT);
  pinMode(13, OUTPUT);
  pinMode(14, OUTPUT);

}

void loop() {

  Serial.println("pin 0");
  digitalWrite(0, HIGH);
  delay(1000);
  digitalWrite(0, LOW);

  Serial.println("pin 2");
  digitalWrite(2, HIGH);
  delay(1000);
  digitalWrite(2, LOW);

  Serial.println("pin 4");
  digitalWrite(4, HIGH);
  delay(1000);
  digitalWrite(4, LOW);

  Serial.println("pin 5");
  digitalWrite(5, HIGH);
  delay(1000);
  digitalWrite(5, LOW);

  Serial.println("pin 12");
  digitalWrite(12, HIGH);
  delay(1000);
  digitalWrite(12, LOW);

  Serial.println("pin 13");
  digitalWrite(13, HIGH);
  delay(1000);
  digitalWrite(13, LOW);

  Serial.println("pin 14");
  digitalWrite(14, HIGH);
  delay(1000);
  digitalWrite(14, LOW);

}
