void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  //Serial0.begin(115200);
  //Serial1.begin(115200);
}

void loop() {
  // put your main code here, to run repeatedly:
  Serial.print("Serial");
  //Serial0.print("Serial0");
  //Serial1.print("Serial1");
  delay(2000);
}
