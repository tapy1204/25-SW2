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
#define _DIST_MIN 100      // mm (표시용 가드 레일)
#define _DIST_MAX 300      // mm (표시용 가드 레일)

#define TIMEOUT ((INTERVAL / 2) * 1000.0) // us
#define SCALE (0.001 * 0.5 * SND_VEL)     // duration(us) -> mm

#define _EMA_ALPHA 0.6     // 참고용(표시만), 제어는 median 기반

// ▼ Median 창 크기: 3, 10, 30으로 바꿔가며 테스트하세요.
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

  // 버퍼를 임시 배열로 복사해 정렬 (O(N log N), N<=30이므로 충분히 빠름)
  float tmp[MEDIAN_N];
  // 순환버퍼 -> 선형 배열 복사
  for (int i = 0; i < n; ++i) {
    int idx = (samp_head - n + i);
    if (idx < 0) idx += MEDIAN_N;
    tmp[i] = samples[idx];
  }

  // 단순 삽입정렬(소규모 N에 적합)
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

  // 초기 EMA를 _DIST_MAX로, 샘플 버퍼는 비워둠
  last_sampling_time = millis();
}

void loop() {
  // 주기 제어
  if (millis() < last_sampling_time + INTERVAL) return;

  // 1) 원시 측정
  float dist_raw = USS_measure(PIN_TRIG, PIN_ECHO);

  // 2) 샘플 버퍼에 추가 (범위 필터/직전 유효값 적용 없이 '있는 그대로' 기록)
  samples[samp_head] = dist_raw;
  samp_head = (samp_head + 1) % MEDIAN_N;
  if (samp_count < MEDIAN_N) samp_count++;

  // 3) 중위수 계산
  float dist_median = compute_median();

  // 4) EMA(표시 전용) — 과제 요구 핵심은 아님. 그래프 비교 참고용
  dist_ema = _EMA_ALPHA * dist_raw + (1.0f - _EMA_ALPHA) * dist_ema;

  // 5) 시리얼 출력 (슬라이드 포맷 준수)
  //    그래프 축 폭을 위해 min()으로 상한(+100mm) 클리핑
  Serial.print("Min:");      Serial.print(_DIST_MIN);
  Serial.print(",raw:");     Serial.print(min(dist_raw,   _DIST_MAX + 100));
  Serial.print(",ema:");     Serial.print(min(dist_ema,   _DIST_MAX + 100));
  Serial.print(",median:");  Serial.print(min(dist_median,_DIST_MAX + 100));
  Serial.print(",Max:");     Serial.print(_DIST_MAX);
  Serial.println("");

  // 6) LED 제어는 'median' 기준으로 수행 (중위수 필터만으로 동작 확인 목적)
  if ((dist_median < _DIST_MIN) || (dist_median > _DIST_MAX))
    digitalWrite(PIN_LED, HIGH);  // 범위 밖 -> LED OFF
  else
    digitalWrite(PIN_LED, LOW);   // 범위 내 -> LED ON

  // 7) 주기 갱신
  last_sampling_time += INTERVAL;
}
