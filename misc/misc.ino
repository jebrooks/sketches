//sonofff use DOUT

int state;

String configNames[] = {"State Topic", "Command Topic", "Min  Open Distance"};
buildForm() {
  String form = Configurator.createForm(configNames[]);
}

handleFormResponse() {
  
}

void setup() {

  
  Srtring configValues = Configurator.getValues(configNames[]);
  
  config[0] = "test";

  delay(6000);
  Serial.begin(115200);

  Serial.println("setting pinmodes...");

  pinMode(12, INPUT);
  state = digitalRead(12);

}

void loop() {

  int lastState = state;
  state = digitalRead(12);
  if (state != lastState) {
    Serial.print("state changed to ");
    Serial.println(state);
  }

}
