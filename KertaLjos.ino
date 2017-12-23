/* 
 *  
 *  Markmið: Búa til lampa úr neopixel hring, sem hefur amk. þrjár stillingar. 
 *  Dimmanlegt hvítt, dimmt rautt, og rólega flöktandi rautt eins og kerti eða eldur.
 * 
 *  Aðferð: Þrjú fylki, r[], g[] og b[], sem innihalda viðeigandi gildi fyrir hvern pixel
 *  
 *  Kerti: random fall sem ákveður +/- gildi á R, sem síðan flöktir til.
 *  
 *  Pottur: A2,
 *  Mode takki: D3, interrupt
 *  Led Datapin: 
 * 
 */

#include <Adafruit_NeoPixel.h>


#define PIN         10     // datapin fyrir led. 6 virkar a pro mini
#define NUMPIX      25    // fjoldi ledda
#define Debounce    400   // debounce a mode-switch takka, ms
#define dimThresh   2     // de-noise throskuldur a pott

// kerti() studlar
#define flickrDiff    0.2      // mesti munur yfirhofud a dimmu og ljosu i flickr (notkun: power*flickrDiff)
#define flickStep     2        // random(0,flickStep), aukning og minnkun i hverju skrefi
#define RGvar         0.5      // hlutfall milli rauds og graens i kerti(), => orange=0.5

#define fDelayLo      50       // hamark og lagmark flickrDelay
#define fDelayHi      200    


Adafruit_NeoPixel pixels = Adafruit_NeoPixel(NUMPIX, PIN, NEO_GRB + NEO_KHZ800);

const byte potPin = 16; // analogpin A2
const byte buttonPin = 3; // intr, pin D3, mode switch takki. Default HIGH, press LOW.

byte LedPower[] = {0, 0, 0, 0, 0};  // fylki fyrir hlaupandi medatal af maelingum
byte power = 0;                     // output power i leddur
byte RedHi, RedLo;

// tima breytur
unsigned long lastRead = 1000;
unsigned long lastSwitch = 1000;
unsigned long lastFlickr = 500;

unsigned long readFreq = 100;    // tidni aflestrar af brightness potti
unsigned long flickrDelay = fDelayLo;

byte cMode = 2;    // 0=hvitt, 1=rautt, 2=kerti.
boolean modeChange = 0;

float angle = 0;

byte R[NUMPIX], G[NUMPIX], B[NUMPIX]; 
int U[NUMPIX];




void kerti(){
// fyrir hvern pixel, halda utan um birtu og hvort hann se ad haekka eda laekka
// birta breytist um random(0,5) i hverju updeit, innan RedHi, RedLo

  RedHi = power;
  RedLo = byte(power * flickrDiff);
  if(RedLo > RedHi) RedLo = 2;

  byte orange = 0;

for(int i=0; i<NUMPIX; i++){
  if(R[i] >= RedHi) U[i] = -1;
  if(R[i] <= RedLo) U[i] = 1;
  randomSeed(analogRead(14));
  R[i] = R[i] + (U[i]*random(0,flickStep));    // random, upp eda nidur eftir atvikum
  R[i] = constrain(R[i], RedLo, RedHi);
  //orange = byte(((R[i]*RGvar)+(U[i]/10))/2);
  pixels.setPixelColor(i, pixels.Color(R[i], 0, 0));
  }
  pixels.show();

  //  Serial.print("R[3]: ");
  //  Serial.println(R[3]);
}



void geraHvitt(){
  for(int i=0;i<NUMPIX;i++){ 
      pixels.setPixelColor(i, pixels.Color(power, power, power)); 
      pixels.show(); 
    }
}

// skoda ad hafa sma graent og blatt med, ef power er fyrir ofan ~200?
void geraRautt(){
  for(int i=0;i<NUMPIX;i++){ 
      pixels.setPixelColor(i, pixels.Color(power, power/4, 0)); 
      pixels.show(); 
    }
}

// breytir milli modes, cMode heldur utan um mode
void swState(){    // cMode, 0=hvitt, 1=rautt, 2=kerti.
   // Debounce, lastSwitch
  if(millis() - lastSwitch >= Debounce){
      if(cMode >= 2){
          cMode = 0;
        }
      else cMode += 1;
  modeChange = 1;
  lastSwitch = millis();
  }
}


// Hlaupandi medaltal af maelingum
void ledPow(byte measured){
  LedPower[0] = measured;
  int sumPow = 0;

  for(int i=4;i>0;i--){
      LedPower[i] = LedPower[i-1];
      sumPow += LedPower[i-1];
  }
  power = byte(sumPow/4);
//  Serial.println(power);

}

void setup() {
  pixels.begin(); // This initializes the NeoPixel library.
  attachInterrupt(1, swState, LOW); // ext.intrp 1 == pinni D3
  pinMode(buttonPin, INPUT);
  pinMode(PIN, OUTPUT);
//  Serial.begin(9600);
  
  // byrja a nulli
  for(int i=0; i<NUMPIX;i++){
    R[i] = 0;
    G[i] = 0;
    B[i] = 0;
    U[i] = 1;
  }
  pixels.show(); 
}
// 

void loop() {

    // --- ## BRIGHTNESS, lesa af potti
    if(millis() - lastRead >= readFreq){  // readFreq, lesa af potti a 200ms fresti
      byte maeling = byte(analogRead(potPin)/4);
      lastRead = millis();
       
      if( ((power - maeling) > dimThresh) || ((maeling - power) > dimThresh) ) {
        ledPow(maeling);
        modeChange = 1;   // breytt astand, uppfaera birtu

        // breyta aflestrar tidni a medan gildin eru olik
        if(maeling != LedPower[4]){
        readFreq = 20;
        }
        else readFreq = 100;
      }
    }

    if(modeChange){  // ef state eda power hefur verid breytt
 //     Serial.print("Mode: ");
 //     Serial.println(cMode); // debug
      switch(cMode){
        case 0:
          geraHvitt();
          modeChange = 0;
          break;
        case 1:
          geraRautt();
          modeChange = 0;
          break;
        case 2:
          kerti();
          modeChange = 0;
          break;
      }

    }
    
//     flickr, lastFlickr
    if(cMode == 2){
      if(millis() - lastFlickr > flickrDelay){
          kerti();
          lastFlickr = millis();
         // flickrDelay = random(0,10) * int(100*sin(angle));
         // angle += 0.1;
          flickrDelay += random(-10,10);
          flickrDelay = constrain(flickrDelay, fDelayLo, fDelayHi);
          // Serial.println(flickrDelay);
      }
    }
    else {delay(1);}

}

/*



Ef stillt a flokkt, flokta utfra styrkleika


# Hvitt, dimmanlegt med 'power'
  - Lesa af potti, breyta ollum rgb gildi

# Rautt, dimmanlegt med 'power'
  - Lesa af potti, laekka

# Kerti, dimmanlegt með 'power'


*/
