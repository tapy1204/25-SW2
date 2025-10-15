#include <Servo.h>

#define PIN_SERVO 10
#define PIN_TRIG  12
#define PIN_ECHO  13

const int ANGLE_DOWN = 20;   // 차단기 '내림' 각도
const int ANGLE_UP   = 90;   // 차단기 '올림' 각도

const unsigned long MOVING_TIME = 2000;  // 한 번 올림/내림에 걸리는 시간(ms)
unsigned long moveStartTime = 0;
int  motionStartAngle = ANGLE_DOWN; // 현재 모션의 시작각
int  motionStopAngle  = ANGLE_UP;   // 현재 모션의 목표각
int  currentAngle     = ANGLE_DOWN; // 서보에 마지막으로 쓴 각도

const float SND_VEL = 346.0f;       // m/s @ 24℃
const unsigned long USS_TIMEOUT_US = 25000UL; // 25ms (왕복 약 4.3m)
const float SCALE_MM = 0.001f * 0.5f * SND_VEL; // us -> mm 계수

// 샘플링 간격 및 간단한 EMA 필터
const unsigned long USS_INTERVAL_MS = 40;
const float EMA_A = 0.5f;           // EMA 가중치(0~1)
unsigned long lastSampleMs = 0;
float distEma = 1000.0f;            // 초기값(먼 거리)

const int OPEN_THRESH_MM  = 250;   
const int CLOSE_THRESH_MM = 320;  
bool carPresent = false;            // 현재 차량 감지 상태

enum EasingMode { SIGMOID, COSINE_EASE_IN_OUT };
const EasingMode EASE_MODE = SIGMOID;
const float SIGMOID_K = 10.0f;

Servo myServo;

float measureDistanceMM() {
  // trigger 10us HIGH
  digitalWrite(PIN_TRIG, HIGH);
  delayMicroseconds(10);
  digitalWrite(PIN_TRIG, LOW);

  // echo pulse duration (us)
  unsigned long dur = pulseIn(PIN_ECHO, HIGH, USS_TIMEOUT_US);

  return dur * SCALE_MM; 
}

float easeSigmoid(float t) {
  // centered sigmoid (0.5에서 급가속), 정규화
  float x = (t - 0.5f);
  float y = 1.0f / (1.0f + expf(-SIGMOID_K * x)); // ~[0,1], but not exactly
  // y(0)=~σ(-K/2), y(1)=~σ(+K/2) → 0..1로 재정규화
  float y0 = 1.0f / (1.0f + expf(-SIGMOID_K * (-0.5f)));
  float y1 = 1.0f / (1.0f + expf(-SIGMOID_K * (+0.5f)));
  return (y - y0) / (y1 - y0);
}

float easeCosineInOut(float t) {
  // (1 - cos(pi * t)) / 2
  return (1.0f - cosf(3.1415926f * t)) * 0.5f;
}

float applyEasing(float t) {
  if (t <= 0.0f) return 0.0f;
  if (t >= 1.0f) return 1.0f;
  if (EASE_MODE == SIGMOID) return easeSigmoid(t);
  else                      return easeCosineInOut(t);
}

int lerpAngle(int a0, int a1, float u01) {
  float v = a0 + (a1 - a0) * u01;

  if (v < 0) v = 0; 
  if (v > 180) v = 180;
  return (int)(v + 0.5f);
}

void setup() {
  pinMode(PIN_TRIG, OUTPUT);
  pinMode(PIN_ECHO, INPUT);
  digitalWrite(PIN_TRIG, LOW);

  myServo.attach(PIN_SERVO);
  delay(200);

  // 초기 자세 내림
  currentAngle = ANGLE_DOWN;
  myServo.write(currentAngle);
  delay(300);

  motionStartAngle = currentAngle;
  motionStopAngle  = ANGLE_UP;
  moveStartTime    = millis();

  Serial.begin(57600);
  Serial.println("Parking Gate challenge start");
}

void loop() {
  unsigned long now = millis();

  if (now - lastSampleMs >= USS_INTERVAL_MS) {
    lastSampleMs = now;
    float d = measureDistanceMM();
    if (d <= 0.1f) d = distEma; 
    distEma = EMA_A * d + (1.0f - EMA_A) * distEma;

    // Hysteresis 판정
    if (!carPresent && distEma > 0 && distEma < OPEN_THRESH_MM) {
      carPresent = true;
      startNewMotionTo(ANGLE_UP);
    } else if (carPresent && distEma > CLOSE_THRESH_MM) {
      carPresent = false;
      startNewMotionTo(ANGLE_DOWN);
    }

    // 모니터링
    Serial.print("dist(mm):"); Serial.print((int)distEma);
    Serial.print(",car:");     Serial.print(carPresent ? 1 : 0);
    Serial.print(",angle:");   Serial.print(currentAngle);
    Serial.println();
  }

  unsigned long elapsed = now - moveStartTime;
  if (elapsed <= MOVING_TIME) {
    float t = (float)elapsed / (float)MOVING_TIME; // 0..1
    float u = applyEasing(t);                      // eased 0..1
    currentAngle = lerpAngle(motionStartAngle, motionStopAngle, u);
    myServo.write(currentAngle);
  } else {
    // 모션 완료 후 각도 고정
    currentAngle = motionStopAngle;
  }
}

// 새 모션 시작
void startNewMotionTo(int targetAngle) {
  // 현재 진행 중 모션의 즉시 위치를 기준 시작각으로 설정
  unsigned long now = millis();
  unsigned long elapsed = now - moveStartTime;
  float t = (elapsed >= MOVING_TIME) ? 1.0f : (float)elapsed / (float)MOVING_TIME;
  float u = applyEasing(t);
  int   instAngle = lerpAngle(motionStartAngle, motionStopAngle, u);

  motionStartAngle = instAngle;
  motionStopAngle  = targetAngle;
  moveStartTime    = now;
}
