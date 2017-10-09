#include <FatReader.h>
#include <SdReader.h>
#include <avr/pgmspace.h>
#include "WaveUtil.h"
#include "WaveHC.h"

#include <Adafruit_NeoPixel.h>

// Which pin on the Arduino is connected to the NeoPixels?
// On a Trinket or Gemma we suggest changing this to 1
#define PIN            7 //neopixles
#define PIN2            9 //neopixles
// How many NeoPixels are attached to the Arduino?
#define NUMPIXELS      30
#define NUMPIXELS2      4
// When we setup the NeoPixel library, we tell it how many pixels, and which pin to use to send signals.
// Note that for older NeoPixel strips you might need to change the third parameter--see the strandtest
// example for more information on possible values.
Adafruit_NeoPixel strip = Adafruit_NeoPixel(NUMPIXELS, PIN, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel strip2 = Adafruit_NeoPixel(NUMPIXELS2, PIN2, NEO_GRB + NEO_KHZ800);
int firstrun = 0;

SdReader card;    // This object holds the information for the card
FatVolume vol;    // This holds the information for the partition on the card
FatReader root;   // This holds the information for the filesystem on the card
FatReader f;      // This holds the information for the file we're play

WaveHC wave;      // This is the only wave (audio) object, since we will only play one at a time

#define DEBOUNCE 5  // button debouncer

// here is where we define the buttons that we'll use. button "1" is the first, button "6" is the 6th, etc
byte buttons[] = {14, 15, 16, 17, 18, 19};
// This handy macro lets us determine how big the array up above is, by checking the size
#define NUMBUTTONS sizeof(buttons)
// we will track if a button is just pressed, just released, or 'pressed' (the current state
volatile byte pressed[NUMBUTTONS], justpressed[NUMBUTTONS], justreleased[NUMBUTTONS];

// this handy function will return the number of bytes currently free in RAM, great for debugging!   
int freeRam(void)
{
  extern int  __bss_end; 
  extern int  *__brkval; 
  int free_memory; 
  if((int)__brkval == 0) {
    free_memory = ((int)&free_memory) - ((int)&__bss_end); 
  }
  else {
    free_memory = ((int)&free_memory) - ((int)__brkval); 
  }
  return free_memory; 
} 

void sdErrorCheck(void)
{
  if (!card.errorCode()) return;
  putstring("\n\rSD I/O error: ");
  Serial.print(card.errorCode(), HEX);
  putstring(", ");
  Serial.println(card.errorData(), HEX);
  while(1);
}

void setup() {
  byte i;
   strip.begin(); // This initializes the NeoPixel library.
   strip2.begin(); // This initializes the NeoPixel library.
  // set up serial port
  Serial.begin(9600);
  putstring_nl("WaveHC with ");
  Serial.print(NUMBUTTONS, DEC);
  putstring_nl("buttons");
  
  putstring("Free RAM: ");       // This can help with debugging, running out of RAM is bad
  Serial.println(freeRam());      // if this is under 150 bytes it may spell trouble!
  
  // Set the output pins for the DAC control. This pins are defined in the library
  pinMode(2, OUTPUT);
  pinMode(3, OUTPUT);
  pinMode(4, OUTPUT);
  pinMode(5, OUTPUT);
 
  // pin13 LED
  pinMode(13, OUTPUT);
 
  // Make input & enable pull-up resistors on switch pins
  for (i=0; i< NUMBUTTONS; i++) {
    pinMode(buttons[i], INPUT);
    digitalWrite(buttons[i], HIGH);
  }
  
  //  if (!card.init(true)) { //play with 4 MHz spi if 8MHz isn't working for you
  if (!card.init()) {         //play with 8 MHz spi (default faster!)  
    putstring_nl("Card init. failed!");  // Something went wrong, lets print out why
    sdErrorCheck();
    while(1);                            // then 'halt' - do nothing!
  }
  
  // enable optimize read - some cards may timeout. Disable if you're having problems
  card.partialBlockRead(true);
 
// Now we will look for a FAT partition!
  uint8_t part;
  for (part = 0; part < 5; part++) {     // we have up to 5 slots to look in
    if (vol.init(card, part)) 
      break;                             // we found one, lets bail
  }
  if (part == 5) {                       // if we ended up not finding one  :(
    putstring_nl("No valid FAT partition!");
    sdErrorCheck();      // Something went wrong, lets print out why
    while(1);                            // then 'halt' - do nothing!
  }
  
  // Lets tell the user about what we found
  putstring("Using partition ");
  Serial.print(part, DEC);
  putstring(", type is FAT");
  Serial.println(vol.fatType(),DEC);     // FAT16 or FAT32?
  
  // Try to open the root directory
  if (!root.openRoot(vol)) {
    putstring_nl("Can't open root dir!"); // Something went wrong,
    while(1);                             // then 'halt' - do nothing!
  }
  
  // Whew! We got past the tough parts.
  putstring_nl("Ready!");
  
  TCCR2A = 0;
  TCCR2B = 1<<CS22 | 1<<CS21 | 1<<CS20;

  //Timer2 Overflow Interrupt Enable
  TIMSK2 |= 1<<TOIE2;

 playfile("squirrel.wav");
}//end setup

SIGNAL(TIMER2_OVF_vect) {
  check_switches();
}


void loop() {
  byte i; 
  if (justpressed[0]) {//Proton Pack switched on
    
    if(firstrun = 1){
      justpressed[0] = 0;
      playfile("Pack.wav");
       Serial.print("Pack.wav");//pack on
       PackonLights();
        strip2.setPixelColor(0,strip2.Color(255,0,0));      
        strip2.setPixelColor(1,strip2.Color(255,0,0));      
        strip2.setPixelColor(2,strip2.Color(255,0,0));
        strip2.setPixelColor(3,strip2.Color(255,05,0));
         strip2.show();
    }
    firstrun = 1;    
    
  }
  else if (justpressed[1]) { //Proton pack fire button pressed

  //if proton pack is turned off play music
    if(digitalRead(14) == 0){//proton pack is on.
      justpressed[1] = 0;
      playfile("nutrona.wav");
      Serial.print("nutrona.wav");
      Firearch();
      TopToBottom();     
    }else{
      Serial.print("play song");
       playfile("Gh02.wav");
      justpressed[1] = 0;
      }
      
      
  }
  else if (justpressed[2]) {
      justpressed[2] = 0;
      playfile("Packstop.wav");
      Serial.print("Packstop.wav");
      strip2.setPixelColor(0,strip2.Color(0,0,0));      
      strip2.setPixelColor(1,strip2.Color(0,0,0));      
      strip2.setPixelColor(2,strip2.Color(0,0,0));
      strip2.setPixelColor(3,strip2.Color(0,0,0));
       strip2.show();
      
  }
  else if (justpressed[3]) {
      justpressed[3] = 0;
      playfile("En04.wav");
  } 
  else if (justpressed[4]) {
      justpressed[4] = 0;
      playfile("SO.WAV");
  } 
  else if (justpressed[5]) {
      justpressed[5] = 0;
      playfile("LA.WAV");
  }
}

/////////////////////////////
//lighting effects
////////////////////////////



int Firearch(){
     uint16_t i;     uint16_t e;     uint16_t t;     uint16_t r;
      for(r=0;r<60;r++){  
         t=27;
         for(i=0;i<10;i++){
            blankstripFAST("00909090");
            for(e=0;e<10;e++){
             strip.setPixelColor(random(t-27,t),strip.Color(0,random(200),random(200)));
             strip.setPixelColor(random(0,45),strip.Color(0,random(200),random(200)));
            }
            t=t+27;
            strip.show();
         }
      }
      blankstripFAST("00909090");
     return 0;
}



int PackonLights(){
     uint16_t i;     uint16_t e;     uint16_t t;     uint16_t r;
      for(r=0;r<25;r++){  
         t=27;
         for(i=0;i<10;i++){
            blankstripFAST("00909090");
            for(e=0;e<10;e++){
             strip2.setPixelColor(random(t-27,t),strip.Color(random(200),0,0));
             strip2.setPixelColor(random(0,45),strip.Color(random(200),0,0));
            }
            t=t+27;
            strip2.show();
         }
      }
      blankstripFAST("00909090");
     return 0;
}



int TopToBottom(){
    
     uint16_t i;
     uint16_t e;
     uint16_t t;
     uint16_t r;
     uint16_t tb;
     
    int NLEDt = 20; // number of LEDs lit at a time
    blankstripFAST("00909090");
    int FadeArray[NLEDt];
    int TTB[] = {270,225,180,135,90,45,0};
    int TopToBottomRepeat = 5;
    
    for(tb=0;tb<TopToBottomRepeat;tb++){ // How many times to repeat TopToBottom
        
    
            for(e=0;e<7;e++){ // how many times to cycle effect
                
                 for(i=0; i<NLEDt; i++) { // light up first set of LEDs and store location
                     
                        int pixNumb = random(TTB[e+1], TTB[e]);
                        strip.setPixelColor(pixNumb,strip.Color(0,0,200)); 
                        //store values in array to fade later
                        FadeArray[i] = pixNumb;
                 }
                 strip.show();
               
                 int fd = 200;
                 int fadespeed = 10; // the higher the number the slower the fade, also the higher the number the lower the fadesteps should be to reach zero/black
                 int fadesteps = 20;
                 for(t=0;t<fadespeed;t++){ // get stored led location and fade to black
                     
                     for(r=0;r<NLEDt;r++){
                         strip.setPixelColor(FadeArray[r],strip.Color(0,0,fd)); //color blue
                     }
                      fd = fd - fadesteps;
                     // delay(10);
                     strip.show();
                 }
                 
                 blankstripFAST("00909090");
             
            }
            
    }
    
    
    
    return 0;
}
// clear the leds
int blankstripFAST(String args){
     uint16_t i;
      for(i=0; i<strip.numPixels(); i++) {
     strip.setPixelColor(i,strip.Color(0,0,0));
      }
      strip.show();
    return 0;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////


// Plays a full file from beginning to end with no pause.
void playcomplete(char *name) {
  // call our helper to find and play this name
  playfile(name);
  while (wave.isplaying) {
  // do nothing while its playing
  }
  // now its done playing
}

void playfile(char *name) {
  // see if the wave object is currently doing something
  if (wave.isplaying) {// already playing something, so stop it!
    wave.stop(); // stop it
  }
  // look in the root directory and open the file
  if (!f.open(root, name)) {
    putstring("Couldn't open file "); Serial.print(name); return;
  }
  // OK read the file and turn it into a wave object
  if (!wave.create(f)) {
    putstring_nl("Not a valid WAV"); return;
  }
  
  // ok time to play! start playback
  wave.play();
}


void check_switches()
{
  static byte previousstate[NUMBUTTONS];
  static byte currentstate[NUMBUTTONS];
  byte index;

  for (index = 0; index < NUMBUTTONS; index++) {
    currentstate[index] = digitalRead(buttons[index]);   // read the button
    
    /*     
    Serial.print(index, DEC);
    Serial.print(": cstate=");
    Serial.print(currentstate[index], DEC);
    Serial.print(", pstate=");
    Serial.print(previousstate[index], DEC);
    Serial.print(", press=");
    */
    
    if (currentstate[index] == previousstate[index]) {
      if ((pressed[index] == LOW) && (currentstate[index] == LOW)) {
          // just pressed
          justpressed[index] = 1;
      }
      else if ((pressed[index] == HIGH) && (currentstate[index] == HIGH)) {
          // just released
          justreleased[index] = 1;
      }
      pressed[index] = !currentstate[index];  // remember, digital HIGH means NOT pressed
    }
    //Serial.println(pressed[index], DEC);
    previousstate[index] = currentstate[index];   // keep a running tally of the buttons
  }
}


