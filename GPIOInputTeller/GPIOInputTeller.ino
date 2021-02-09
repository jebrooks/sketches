void setup() {

  delay(2000);
  Serial.begin(115200);

  Serial.println("setting pinmodes...");
  pinMode(0, INPUT);
  pinMode(2, INPUT);
  pinMode(4, INPUT);
  pinMode(5, INPUT);
  pinMode(12, INPUT);
  pinMode(13, INPUT);
  pinMode(14, INPUT);

}

void loop() {

  int i = digitalRead(0);
  if (i == 1) {
    Serial.println("pin 0");
    delay(500);
  }

  i = digitalRead(2);
  if (i == 1) {
    Serial.println("pin 2");
    delay(500);
  }

  i = digitalRead(4);
  if (i == 1) {
    Serial.println("pin 4");
    delay(500);
  }

  i = digitalRead(5);
  if (i == 1) {
    Serial.println("pin 5");
    delay(500);
  }

  i = digitalRead(12);
  if (i == 1) {
    Serial.println("pin 1");
    delay(500);
  }

  i = digitalRead(13);
  if (i == 1) {
    Serial.println("pin 13");
    delay(500);
  }

  i = digitalRead(14);
  if (i == 1) {
    Serial.println("pin 14");
    delay(500);
  }


}
