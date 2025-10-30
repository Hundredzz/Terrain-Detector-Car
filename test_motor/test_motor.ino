const int AP 5; //A+
const int AM 6; //A-
const int BP 10; //B+
const int BM 11; //B-

void setup() {
  pinMode(AP,OUTPUT);
  pinMode(AM,OUTPUT);
  pinMode(BP,OUTPUT);
  pinMode(BM,OUTPUT);
}

void loop() {
  //forward?
  digitalWrite(AP,HIGH);
  digitalWrite(AM,LOW);
  digitalWrite(BP,LOW);
  digitalWrite(BM,HIGH);
  delay(2000);

  //turn right?
  digitalWrite(AP,HIGH);
  digitalWrite(AM,LOW);
  digitalWrite(BP,HIGH);
  digitalWrite(BM,LOW);
  delay(2000);

  //turn left?
  digitalWrite(AP,LOW);
  digitalWrite(AM,HIGH);
  digitalWrite(BP,LOW);
  digitalWrite(BM,HIGH);
  delay(2000);

  //backward?
  digitalWrite(AP,LOW);
  digitalWrite(AM,HIGH);
  digitalWrite(BP,HIGH);
  digitalWrite(BM,LOW);
  delay(2000);

  //stop
  digitalWrite(AP,LOW);
  digitalWrite(AM,LOW);
  digitalWrite(BP,LOW);
  digitalWrite(BM,LOW);
  delay(2000);
}
