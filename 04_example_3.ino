#define PIN_LED 13
unsigned int count, toggle;
int toggle_state(unsigned int toggle);

void setup(){
  Serial.begin(115200);
  while(!Serial){
    ;
  }
  pinMode(PIN_LED, OUTPUT);
  Serial.println("Hello World!\n");
  count = toggle = 0;
  digitalWrite(PIN_LED, toggle);
}

void loop(){
  Serial.println(++count);
  toggle = toggle_state(toggle);
  digitalWrite(PIN_LED, toggle);
  delay(1000);
}

int toggle_state(unsigned int toggle){
  return !toggle;
}
