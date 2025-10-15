// 09_challenge_median.ino
// 도전과제: 중위수 필터만으로 동작 (직전 유효값 유지 로직 제거)

// Arduino pin assignment
#define PIN_LED  9
#define PIN_TRIG 12
#define PIN_ECHO 13

// configurable parameters
#define SND_VEL 346.0      // m/s @ 24°C
#define INTERVAL 50        // ms
#define PULSE_DURATION 10  // us
#define _DIST_MIN 100      // mm
#define _DIST_MAX 300      // mm

#define TIMEOUT ((INTERVAL / 2) * 1000.0) // us
#define SCALE (0.001 * 0.5 * SND_VEL)     // duration(us) -> mm

#define _EMA_ALPHA 0.6     

#define MEDIAN_N 10

// global variables
unsigned long last_sampling_time = 0; // ms
float dist_ema = _DIST_MAX;           // EMA 표시용

// Median 필터용 순환 버퍼
float samples[MEDIAN_N];
int   samp_count = 0;     // 현재 채워진 개수 (최대 MEDIAN_N)
int   samp_head  = 0;     // 다음 기록 위치(원형)

// 유틸: 현재 버퍼의 중위수 계산
//  - 개수가 홀수: 중앙값
//  - 개수가 짝수: 중앙 두 값의 평균
float compute_median() {
  int n = samp_count;
  if (n == 0) return 0.0;

  // 버퍼를 임시 배열로 복사해 정렬
  float tmp[MEDIAN_N];
  // 순환버퍼 -> 선형 배열 복사
  for (int i = 0; i < n; ++i) {
    int idx = (samp_head - n + i);
    if (idx < 0) idx += MEDIAN_N;
    tmp[i] = samples[idx];
  }

  // 단순 삽입정렬
  for (int i = 1; i < n; ++i) {
    float key = tmp[i];
    int j = i - 1;
    while (j >= 0 && tmp[j] > key) {
      tmp[j + 1] = tmp[j];
      --j;
    }
    tmp[j + 1] = key;
  }

  if (n & 1) {
    return tmp[n / 2];
  } else {
    return 0.5f * (tmp[n / 2 - 1] + tmp[n / 2]);
  }
}

// 초음파 거리측정 (mm)
float USS_measure(int TRIG, int ECHO)
{
  digitalWrite(TRIG, HIGH);
  delayMicroseconds(PULSE_DURATION);
  digitalWrite(TRIG, LOW);

  // pulseIn은 왕복 시간(us). 편도거리 환산을 위해 0.5 * SND_VEL 계수 포함된 SCALE 사용
  unsigned long dur = pulseIn(ECHO, HIGH, TIMEOUT);
  return dur * SCALE; // mm
}

void setup() {
  pinMode(PIN_LED, OUTPUT);
  pinMode(PIN_TRIG, OUTPUT);
  pinMode(PIN_ECHO, INPUT);
  digitalWrite(PIN_TRIG, LOW);

  Serial.begin(57600);

  last_sampling_time = millis();
}

void loop() {
  // 주기 제어
  if (millis() < last_sampling_time + INTERVAL) return;

  float dist_raw = USS_measure(PIN_TRIG, PIN_ECHO);

  // 샘플 버퍼에 추가
  samples[samp_head] = dist_raw;
  samp_head = (samp_head + 1) % MEDIAN_N;
  if (samp_count < MEDIAN_N) samp_count++;

  // 중위수 계산
  float dist_median = compute_median();
  
  dist_ema = _EMA_ALPHA * dist_raw + (1.0f - _EMA_ALPHA) * dist_ema;

  // 시리얼 출력
  // 그래프 축 폭을 위해 min()으로 상한(+100mm) 클리핑
  Serial.print("Min:");      Serial.print(_DIST_MIN);
  Serial.print(",raw:");     Serial.print(min(dist_raw,   _DIST_MAX + 100));
  Serial.print(",ema:");     Serial.print(min(dist_ema,   _DIST_MAX + 100));
  Serial.print(",median:");  Serial.print(min(dist_median,_DIST_MAX + 100));
  Serial.print(",Max:");     Serial.print(_DIST_MAX);
  Serial.println("");
  
  if ((dist_median < _DIST_MIN) || (dist_median > _DIST_MAX))
    digitalWrite(PIN_LED, HIGH);  // 범위 밖 -> LED OFF
  else
    digitalWrite(PIN_LED, LOW);   // 범위 내 -> LED ON
  
  last_sampling_time += INTERVAL;
}
