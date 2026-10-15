// Sign language glove - finger flex -> gesture -> phrase
// Portable C++ (no STL), safe on AVR / ESP32 / RP2040

#define THUMB  A0
#define INDEX  A1
#define MIDDLE A2
#define RING   A3
#define PINKY  A6

const int NUM_FINGERS = 5;
const int FINGER_PINS[NUM_FINGERS] = {THUMB, INDEX, MIDDLE, RING, PINKY};

// Per-finger calibration. Each flex sensor has its own range - measure
// yours with the CALIBRATE block below and fill these in.
// FLAT = finger straight, BENT = finger fully curled.
const int FLEX_FLAT[NUM_FINGERS] = {700, 700, 700, 700, 700};
const int FLEX_BENT[NUM_FINGERS] = {900, 900, 900, 900, 900};

const int LEVELS = 3;          // states per finger: 0 (flat) .. 3 (fully bent)
const unsigned long HOLD_MS = 120;  // pose must be steady this long to count

// ---- Gesture table -------------------------------------------------------
// Add a row per gesture: {thumb, index, middle, ring, pinky}, "phrase"
struct Gesture {
  int state[NUM_FINGERS];
  const char* phrase;
};

const Gesture GESTURES[] = {
  {{0, 0, 0, 0, 3}, "Hello!"},
  {{3, 3, 3, 3, 3}, "Yes"},
  {{0, 3, 3, 3, 3}, "Good"},
};
const int NUM_GESTURES = sizeof(GESTURES) / sizeof(GESTURES[0]);

// ---- State ---------------------------------------------------------------
int handState[NUM_FINGERS];
int candidateState[NUM_FINGERS];
int lastStableState[NUM_FINGERS] = {-1, -1, -1, -1, -1};
unsigned long candidateSince = 0;

// ---- Helpers -------------------------------------------------------------
bool sameState(const int a[NUM_FINGERS], const int b[NUM_FINGERS]) {
  for (int i = 0; i < NUM_FINGERS; i++) {
    if (a[i] != b[i]) return false;
  }
  return true;
}

void copyState(int dest[NUM_FINGERS], const int src[NUM_FINGERS]) {
  for (int i = 0; i < NUM_FINGERS; i++) dest[i] = src[i];
}

void readHand(int out[NUM_FINGERS]) {
  for (int i = 0; i < NUM_FINGERS; i++) {
    int raw = analogRead(FINGER_PINS[i]);
    out[i] = constrain(map(raw, FLEX_FLAT[i], FLEX_BENT[i], 0, LEVELS), 0, LEVELS);
  }
  
}

void printState(const int s[NUM_FINGERS]) {
  for (int i = 0; i < NUM_FINGERS; i++) {
    Serial.print(s[i]);
    Serial.print(i < NUM_FINGERS - 1 ? ',' : '\n');
  }
}

// ---- Main ----------------------------------------------------------------
void setup() {
  Serial.begin(9600);
  while (!Serial) { ; }   // harmless on AVR, needed on native-USB boards
  readHand(candidateState);
  candidateSince = millis();
}

void loop() {
  // Uncomment to see raw ADC values while setting FLEX_FLAT / FLEX_BENT:
  for (int i = 0; i < NUM_FINGERS; i++) {
    Serial.print(analogRead(FINGER_PINS[i])); Serial.print('\t');
  }
  Serial.println();
  delay(100);
  return;

  readHand(handState);
  

  // Pose changed -> restart the hold timer. This debounces sensor jitter at
  // the boundary between two levels.
  if (!sameState(handState, candidateState)) {
    copyState(candidateState, handState);
    candidateSince = millis();
    return;
  }

  // Not held long enough yet.
  if (millis() - candidateSince < HOLD_MS) return;

  // Held, but it's the same pose we already reported -> nothing new.
  if (sameState(candidateState, lastStableState)) return;

  copyState(lastStableState, candidateState);

  for (int g = 0; g < NUM_GESTURES; g++) {
    if (sameState(lastStableState, GESTURES[g].state)) {
      Serial.println(GESTURES[g].phrase);
      return;
    }
  }

  // Unrecognised pose - useful while building the gesture table.
  // Serial.print("unknown: "); printState(lastStableState);
}
