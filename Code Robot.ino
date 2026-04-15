// ============================================================
//  LE DALMACHIEN - V3 PARCOURS AVEC GYROSCOPE MPU6050
//  Équipe 24 - MEC2105 - Polytechnique Montréal - H2026
// ============================================================

#include <Servo.h>
#include <Adafruit_NeoPixel.h>
#include <Wire.h>

// ============================================================
//  VARIABLES À CALIBRER
// ============================================================
const float VITESSE_MS = 7000;  // ms par mètre (1 m en 3.5 s) *** CALIBRER ***
const float GYRO_TOLERANCE = 2.0;    // degrés
const float DEG_PAR_SEC = 112.0;     // pour timeout sécurité

// Distances parcours (cm)
const float DIST_1  = VITESSE_MS*0.70;   // montée initiale
const float DIST_3  = VITESSE_MS*0.15;   // avance vers poulet
const float DIST_5  = VITESSE_MS*0.20;   // recul après poulet
const float DIST_9  = VITESSE_MS*1.30;   // traversée
const float DIST_12 = VITESSE_MS*0.65;   // avance finale

// Queue
const int QUEUE_CENTRE      = 90;
const int QUEUE_AMPLITUDE   = 150;
const int QUEUE_DUREE_CYCLE = 1500;
const int QUEUE_NB_CYCLES   = 3;

// LED
const uint8_t LED_R = 0, LED_G = 255, LED_B = 0;
const int LED_ON_MS  = 500;
const int LED_OFF_MS = 1000;
const int LED_NB_CLIGNO = 3;

// ============================================================
//  PINS
// ============================================================
const int IN1 = 2, IN2 = 3, IN3 = 4, IN4 = 7;
const int PIN_QUEUE = 12;
const int PIN_TETE_G = 9;
const int PIN_TETE_D = 10;
const int PIN_LED   = 6;
const int NB_LEDS   = 20;
const int MPU_ADDR  = 0x68;

// Tete
const int TETE_VERTICAL   = 98;
const int TETE_HORIZONTAL = 20;
const int TETE_MS_PAR_DEG = 15;
const int ACOUP_HAUT   = 30;
const int ACOUP_BAS    = 10;
const int ACOUP_VITESSE = 10;  // ms par degre
const int ACOUP_PAUSE  = 50;   // ms entre a-coups
const int ACOUP_NB     = 5;

// ============================================================
//  OBJETS
// ============================================================
Servo servoQueue, servoTeteG, servoTeteD;
Adafruit_NeoPixel strip(NB_LEDS, PIN_LED, NEO_GRB + NEO_KHZ800);
int angleTete = TETE_VERTICAL;
float angleZ = 0.0;
unsigned long lastGyroTime;

// ============================================================
//  MPU6050
// ============================================================
void initMPU() {
  Wire.begin();
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x6B); Wire.write(0);
  Wire.endTransmission(true);
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x1B); Wire.write(0x00); // ±250°/s
  Wire.endTransmission(true);
  delay(100);
  angleZ = 0.0;
  lastGyroTime = millis();
}

float readGyroZ() {
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x47);
  Wire.endTransmission(false);
  Wire.requestFrom(MPU_ADDR, 2, true);
  int16_t raw = (Wire.read() << 8) | Wire.read();
  return raw / 131.0;
}

void updateAngle() {
  unsigned long now = millis();
  float dt = (now - lastGyroTime) / 1000.0;
  lastGyroTime = now;
  angleZ += readGyroZ() * dt;
}

void resetAngle() {
  angleZ = 0.0;
  lastGyroTime = millis();
}

// ============================================================
//  MOTEURS
// ============================================================
void avancer()       { digitalWrite(IN1,HIGH); digitalWrite(IN2,LOW);  digitalWrite(IN3,HIGH); digitalWrite(IN4,LOW);  }
void reculer()       { digitalWrite(IN1,LOW);  digitalWrite(IN2,HIGH); digitalWrite(IN3,LOW);  digitalWrite(IN4,HIGH); }
void tournerDroite() { digitalWrite(IN1,LOW);  digitalWrite(IN2,HIGH); digitalWrite(IN3,HIGH); digitalWrite(IN4,LOW);  }
void tournerGauche() { digitalWrite(IN1,HIGH); digitalWrite(IN2,LOW);  digitalWrite(IN3,LOW);  digitalWrite(IN4,HIGH); }
void stopMotors()    { digitalWrite(IN1,LOW);  digitalWrite(IN2,LOW);  digitalWrite(IN3,LOW);  digitalWrite(IN4,LOW);  }

void go(float ms) {
  if (ms <= 0) return;
  avancer(); delay((int)ms); stopMotors();
}

void back(float ms) {
  if (ms <= 0) return;
  reculer(); delay((int)ms); stopMotors();
}

void rotGyro(bool droite, float degres) {
  if (degres <= 0) return;
  resetAngle();
  unsigned long timeout = (unsigned long)(degres / DEG_PAR_SEC * 1000) * 3;
  droite ? tournerDroite() : tournerGauche();
  unsigned long start = millis();
  while (true) {
    updateAngle();
    if (abs(angleZ) >= degres - GYRO_TOLERANCE) break;
    if (millis() - start > timeout) { Serial.println(F("TIMEOUT!")); break; }
    delay(2);
  }
  stopMotors();
  delay(200);
}

// ============================================================
//  QUEUE
// ============================================================
void mouvQueue(int de, int a, int ms) {
  de = constrain(de,0,180); a = constrain(a,0,180);
  int n = abs(a-de); if (!n) return;
  int d = max(ms/n,1); int step = (de<a)?1:-1;
  for (int p=de; p!=a+step; p+=step) { servoQueue.write(p); delay(d); }
}

void batQueue() {
  int g = constrain(QUEUE_CENTRE - QUEUE_AMPLITUDE/2, 0, 180);
  int d = constrain(QUEUE_CENTRE + QUEUE_AMPLITUDE/2, 0, 180);
  servoQueue.write(QUEUE_CENTRE); delay(200);
  for (int i=0; i<QUEUE_NB_CYCLES; i++) {
    mouvQueue(QUEUE_CENTRE, g, QUEUE_DUREE_CYCLE/4);
    mouvQueue(g, d, QUEUE_DUREE_CYCLE/2);
    mouvQueue(d, QUEUE_CENTRE, QUEUE_DUREE_CYCLE/4);
  }
  servoQueue.write(QUEUE_CENTRE);
}

// ============================================================
//  TETE
// ============================================================
void ecrireTete(int angle) {
  servoTeteG.write(angle);
  servoTeteD.write(180 - angle);
}

void bougerTete(int cible) {
  if (cible == angleTete) return;
  int step = (cible > angleTete) ? 1 : -1;
  for (int a = angleTete; a != cible; a += step) {
    ecrireTete(a);
    delay(TETE_MS_PAR_DEG);
  }
  ecrireTete(cible);
  angleTete = cible;
}

void aCoupsTete() {
  for (int i = 0; i < ACOUP_NB; i++) {
    int step = (ACOUP_HAUT > ACOUP_BAS) ? 1 : -1;
    for (int a = ACOUP_BAS; a != ACOUP_HAUT; a += step) {
      ecrireTete(a); delay(ACOUP_VITESSE);
    }
    ecrireTete(ACOUP_HAUT);
    delay(ACOUP_PAUSE);
    for (int a = ACOUP_HAUT; a != ACOUP_BAS; a -= step) {
      ecrireTete(a); delay(ACOUP_VITESSE);
    }
    ecrireTete(ACOUP_BAS);
    delay(ACOUP_PAUSE);
  }
  angleTete = ACOUP_BAS;
}

void sequenceDepot() {
  Serial.println(F("  Tete: descente"));
  bougerTete(TETE_HORIZONTAL);
  delay(2000);
  //Serial.println(F("  Tete: a-coups"));
  //aCoupsTete();
  //delay(1000);
  Serial.println(F("  Tete: remontee"));
  bougerTete(TETE_VERTICAL);
}

// ============================================================
//  LED
// ============================================================
void ledOn()  { for(int i=0;i<NB_LEDS;i++) strip.setPixelColor(i,strip.Color(LED_R,LED_G,LED_B)); strip.show(); }
void ledOff() { strip.clear(); strip.show(); }
void ledBlink(int n) { for(int i=0;i<n;i++){ ledOn(); delay(LED_ON_MS); ledOff(); delay(LED_OFF_MS); } }

// ============================================================
//  PARCOURS
// ============================================================
void setup() {
  Serial.begin(9600);
  pinMode(IN1,OUTPUT); pinMode(IN2,OUTPUT);
  pinMode(IN3,OUTPUT); pinMode(IN4,OUTPUT);
  stopMotors();

  servoQueue.attach(PIN_QUEUE);
  servoQueue.write(QUEUE_CENTRE);
  servoTeteG.attach(PIN_TETE_G);
  servoTeteD.attach(PIN_TETE_D);
  ecrireTete(TETE_VERTICAL);
  strip.begin(); strip.show(); ledOff();
  initMPU();

  Serial.println(F("=== DALMACHIEN V3 PRET ==="));
  delay(2000);

  // 1. Avancer 30 cm
  Serial.println(F("1: Avancer 30cm"));
  go(DIST_1);
  delay(300);

  // 2. Tourner 45° anti-horaire
  Serial.println(F("2: Rot 45 gauche"));
  rotGyro(false, 45);
  delay(300);

  // 3. Avancer 15 cm
  Serial.println(F("3: Avancer 15cm"));
  go(DIST_3);
  delay(300);

  // 4. Depot poulet (tete baisse + a-coups + remonte)
  Serial.println(F("4: Depot poulet"));
  sequenceDepot();

  // 5. Reculer 15 cm
  Serial.println(F("5: Reculer 15cm"));
  back(DIST_5);
  delay(300);

  // 6. Tourner 90° anti-horaire
  Serial.println(F("6: Rot 90 gauche"));
  rotGyro(false, 90);
  delay(300);

  // 7. LED clignote 3x
  Serial.println(F("7: LED x3"));
  ledBlink(LED_NB_CLIGNO);
  delay(300);

  // 9. Tourner 45° horaire
  Serial.println(F("9: Rot 45 droite"));
  rotGyro(true, 55);
  delay(300);

  // 10. Avancer 70 cm
  Serial.println(F("10: Avancer 70cm"));
  go(DIST_9);
  delay(300);

  // 11. Tourner 90° anti-horaire
  Serial.println(F("11: Rot 90 gauche"));
  rotGyro(false, 125);
  delay(300);

  Serial.println(F("13: Queue bat"));
  batQueue();
  delay(25000);

  // FIN DU PARCOURS OBLIGATOIRE

    // 8. Tourner 45° horaire
  Serial.println(F("8: Rot 45 droite"));
  rotGyro(true, 50);

  
  delay(300);

  // 12. Avancer 30 cm
  Serial.println(F("12: Avancer 30cm"));
  go(DIST_12);
  delay(300);


  // 14. Rotation 360° sur soi-même
  Serial.println(F("14: Rot 360"));
  rotGyro(true, 270);
  delay(300);

  // FIN
  stopMotors();
  servoQueue.write(QUEUE_CENTRE);
  ledOff();
  Serial.println(F("=== FIN PARCOURS ==="));
}

void loop() {}
