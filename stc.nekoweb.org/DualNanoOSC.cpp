/*
Dual mode eurorack oscillator inspired by Hagiwo's nano oscillator
    # mode 1 : wavetable morphing oscillator
      % smooth morphing from noise-sine-tri-saw-square
      knob 1 & voct in  = pitch
      knob 2 & CV_IN1   = wavetable positon
      knob 3 & CV_IN2   = tbd

    # mode 2 : 2 Operator FM
      % the Donker
      knob 1 & voct in  = pitch
      knob 2 & CV_IN1   = FM ratio
      knob 3 & CV_IN2   = FM depth
  built on the excellent mozzi audio library    
  Mozzi is licensed under the GNU Lesser General Public Licence (LGPL) Version 2.1 or later.
  written by Sam of Homeostatic music - 2025
*/

///////////////////////////////
#include &lt;MozziConfigValues.h&gt;
#define MOZZI_AUDIO_MODE MOZZI_OUTPUT_2PIN_PWM //up to 14 bit output
#define MOZZI_AUDIO_RATE 32768
#define MOZZI_CONTROL_RATE 512  // Hz, powers of 2 are most reliable  //maybe try and increase this to allow for FMing

#include &lt;Mozzi.h&gt;
#include &lt;Oscil.h&gt;
#include &lt;FixMath.h&gt;

#define OSCIL_DITHER_PHASE

// table for FM Oscils to play (2KB)
#include &lt;tables/cos2048_int8.h&gt;  

//custom data with 16 wavetables (16KB)
#include &lt;tables/WaveSTC1024_int8.h&gt;
//list for iterating over names
const int8_t* WAVENAMES[16] = {
  WAVEDATA0, WAVEDATA1, WAVEDATA2, WAVEDATA3, WAVEDATA4, WAVEDATA5, WAVEDATA6, WAVEDATA7, WAVEDATA8, WAVEDATA9, WAVEDATA10, WAVEDATA11, WAVEDATA12, WAVEDATA13, WAVEDATA14, WAVEDATA15
};

// v/oct scaling factor range 1 - 32 (4KB)  {could probably be converted to UFix&lt;5,11&gt; with minimal loss of precision if 2KB needs saving}
//2**voct ist precalculated to avoid using pow()
#include &lt;tables/voctpow1024_float.h&gt;
const static float voctpow[1024] PROGMEM = VOCTPOW_DATA;

// scaling factor for converting voctpow values to fractional values between 0 and 1
const static float INVFACT = 1/31;

// response curve for FM ratio with flat-zones around even ratios, stored as 8 bit fixed point (1KB)
#include &lt;tables/ratiotable1024_UFix3_5.h&gt;
const static UFix&lt;3,5&gt; ratiotable[1024] PROGMEM = RATIOTABLE_DATA;

////////////////////////////////////////////////////////////////////////////////
//IO - pin numbers
const byte KNOB1 = A0;  
const byte KNOB2 = A1;  
const byte KNOB3 = A2;  

const int SWITCH_UP = 12;
const int SWITCH_DOWN = 11;

const byte VOCT_IN = A4;
const byte CV_IN1 = A6;  
const byte CV_IN2 = A7;  

////////////////////////////////////////////////////////////////////////////////
/*
Output over pins 9 & 10 (2 pin PWM defaults)
 D10----[ 499k ]---|
                   |
  D9----[ 3.9k ]---o---[3.9k]---o--------&lt;OUT
                   |            L--|10n|-GND
                   L---|47n|-GND
low pass definitely takes off some high end, but the impoved clarity is nice

the ouput is buffered by a tl072 buffer before being sent out of the module
inputs have clamping diodes to block voltages outside of ~0-5V
*/
////////////////////////////////////////////////////////////////////////////////
//oscilators mode 1,
Oscil &lt;1024, MOZZI_AUDIO_RATE&gt; waveA(WAVEDATA0);
Oscil &lt;1024, MOZZI_AUDIO_RATE&gt; waveB(WAVEDATA1);
  
//oscilators mode 2,
Oscil&lt;COS2048_NUM_CELLS, MOZZI_AUDIO_RATE&gt; aCarrier(COS2048_DATA);
Oscil&lt;COS2048_NUM_CELLS, MOZZI_AUDIO_RATE&gt; aModulator(COS2048_DATA);

//////////////////////////////////////////////////////////////////////////////////////
// variables

// toggle
volatile int mode;

// Mode 1 - Wavetable
UFix&lt;0,8&gt; gainA, gainB;
int voct, freq_knob;
UFix&lt;16,16&gt; freq; 
int wave_knob, wave_cv, wave_sum;
UFix&lt;4,6&gt; wave_pos; //for interpreting the 10bit adc reads as values from [0, 15.99]

// Mode 2 - FM
UFix&lt;0, 16&gt; mod_index;
UFix&lt;8, 16&gt; deviation;  // 8 so that we do not exceed 32bits in updateAudio
UFix&lt;4, 7&gt; mod_to_carrier_ratio;  // each 10 bit read (knob & CV_IN) will be interpreted as UFix&lt;3,7&gt;, these will be added to make a single UFix&lt;4,7&gt; (range 0 - 15.99)
UFix&lt;0,2&gt; offset = 0.25; //minimum ratio
UFix&lt;3,7&gt; ratio, cv_ratio;  //values from [0, 7.99]
UFix&lt;0,10&gt; depth, cv_depth, knob_depth; //10 bit numbers from [0,1[
UFix&lt;16, 16&gt; carrier_freq, mod_freq;  

//////////////////////////////////////////////////////////////////////////////////////
// function for setting wavetable position and crossfade gain in mode 1
void setWave(){
  //set waves
  int wave_int = wave_pos.asInt();
  waveA.setTable(WAVENAMES[constrain((wave_int-1),0,15)]);
  waveB.setTable(WAVENAMES[wave_int]);
  //set gains   -   linear crossfade between waves
  gainB = wave_pos.asFloat()-wave_int; //fractional part of wave_pos
  gainA = 1-gainB.asFloat();

}

void setFreqs(UFix&lt;16,16&gt; freq) {
  if (mode ==  1){
    waveA.setFreq(freq);
    waveB.setFreq(freq);
  }
  if (mode == 2){
    mod_freq = UFix&lt;14,16&gt;(freq) * mod_to_carrier_ratio;
    // this deviation factor is used within the AudioOutput calculation
    deviation = ((UFix&lt;16,0&gt;(mod_freq)) * depth).sR&lt;8&gt;();  // the sR here cheaply divides the deviation by 256.
    aCarrier.setFreq(freq);
    aModulator.setFreq(mod_freq);
  }  
}

//////////////////////////////////////////////////////////////////////////////////////
void setup() {
  // pinMode(SWITCH_DOWN, INPUT_PULLUP); //currently unused
  pinMode(12, INPUT_PULLUP);
  pinMode(LED_BUILTIN, OUTPUT);
  startMozzi();
  // Serial.begin(9600);
}

void updateControl(){ 
  //determine mode
  mode = digitalRead(SWITCH_UP) +1;
  if (mode == 1){//Wavetable
    digitalWrite(LED_BUILTIN, HIGH); // LED indicator
    //read and set wavetable position
    wave_knob = mozziAnalogRead(KNOB2);
    wave_cv = mozziAnalogRead(CV_IN1);
    wave_sum = constrain(wave_knob+wave_cv, 0, 1023);
    wave_pos = UFix&lt;4,6&gt;::fromRaw(wave_sum); //
    setWave();
  }
  if (mode==2){//FM
    digitalWrite(LED_BUILTIN, LOW); // LED indicator
    //set ratio
    //knob
    ratio = UFix&lt;3,5&gt;::fromRaw(pgm_read_byte(&(ratiotable[mozziAnalogRead(KNOB2)])));
    //CV in
    cv_ratio = UFix&lt;3,7&gt;::fromRaw((mozziAnalogRead(CV_IN1)));
    mod_to_carrier_ratio = ratio + cv_ratio + offset; 
  }
  //KNOB3 & CVIN2 as VCA/FM depth 
  //knob
  knob_depth = UFix&lt;0,10&gt;::fromRaw(mozziAnalogRead(KNOB3)); // Interpret 10bit read as value in [0, 1] 
  //CV in
  cv_depth = UFix&lt;0,10&gt;::fromRaw(mozziAnalogRead(CV_IN2));
  depth = min((cv_depth + knob_depth).asFloat(), 0.99);
  //setting freq, same in both modes
  freq_knob = 32 + (mozziAnalogRead(KNOB1)&gt;&gt;1); //Range 32 hz ~C1 - 544 hz ~C#5            # without rightshift 1055hz ~C6
  //voct scaling
  voct = mozziAnalogRead(VOCT_IN);
  freq = UFix&lt;16,16&gt;(freq_knob * (pgm_read_float(&(voctpow[voct])))); // V/oct apply
  setFreqs(freq);
}

AudioOutput updateAudio(){
  switch (mode){
    case 1://Wavetable
       return MonoOutput(SCALE_AUDIO(
                (depth*(
                    (gainA * UFix&lt;8,0&gt;::fromRaw(waveA.next()))
                    +(gainB * UFix&lt;8,0&gt;::fromRaw(waveB.next()))
                  ) 
                ).asRaw(),   17) // 16+1 bit headroom scaling
        );
        break;
    case 2://FM
      auto modulation = (deviation * toSFraction(aModulator.next())); //modulation scaling
      return MonoOutput::from8Bit(aCarrier.phMod(modulation));        //FM is really phase-mod
      break;
    default: //should never occur
      return 0;
  }
}

void loop() {
  audioHook();
}



