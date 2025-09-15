#define PIN_LED 7
unsigned int count, toggle;
int toggle_state(unsigned int toggle);

void setup(){
  Serial.begin(115200);
  while(!Serial) {}
  pinMode(PIN_LED, OUTPUT);
  count = toggle = 0;
  digitalWrite(PIN_LED, 1);
  delay(1000);
}

void loop(){
    Serial.println(++count);
    if(count <= 10)
    {
      digitalWrite(PIN_LED, 0);
    }
    if(count > 10 && count <= 21)
    {
      toggle = toggle_state(toggle);
      digitalWrite(PIN_LED, toggle);
    }
    if(count > 21)
    {
      while(1){}
    }
    delay(100);
}

int toggle_state(unsigned int toggle){
  return !toggle;
}
