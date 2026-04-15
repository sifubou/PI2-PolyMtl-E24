# Le Dalmachien 🐕‍🦺

Prototype de chien robot autonome conçu dans le cadre du cours **MEC2105 · Projet Intégrateur II** (Polytechnique Montréal, Hiver 2026).

**Équipe 24 · Groupe 02**
Adam Bouaddi · Rose Mélançon · Liliane Leclerc-Ko · Miguel Azoury · Ryan Kangue Essomé Simeu · William Lalonde

## Mission
Parcourir une piste de compétition, déposer un jouet poulet dans la zone cible, s'immobiliser dans deux cercles rouges successifs avec battement de queue et clignotement LED, puis exécuter une rotation complète dans le cercle BONUS.

## Matériel
- Microcontrôleur : Arduino Uno
- Moteurs : 2 × DC TT bleu, 110 tr/min (haut couple)
- Servomoteurs : 3 × MG90S (2 tête, 1 queue)
- Gyroscope : MPU6050 (asservissement des rotations)
- Alimentation : 2 × piles Li-ion 18650 (3500 mAh)
- LED : anneau NeoPixel 20 × WS2812

## Bibliothèques Arduino requises
- `Servo.h` (standard)
- `Adafruit_NeoPixel`
- `Wire.h` (standard, pour I²C du MPU6050)

## Brochage
| Fonction | Pin Arduino |
|---|---|
| Moteur IN1, IN2, IN3, IN4 | 2, 3, 4, 7 |
| Servo queue | 12 |
| Servo tête gauche / droite | 9 / 10 |
| LED NeoPixel | 6 |
| MPU6050 (I²C SDA/SCL) | A4 / A5 |

## Calibration
- `VITESSE_MS = 7000` ms/m (à calibrer sur la piste réelle)
- `DEG_PAR_SEC = 112` °/s (régulation gyroscopique)

## Fichiers
- `arduino/V3.ino` — programme du parcours complet (parcours obligatoire + BONUS)

## Licence
MIT
