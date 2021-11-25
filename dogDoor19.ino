// dogDoor19  8/7/2021 pm
// Dog Door app on Arduino
// Installed Production Version
//
// removed AUTObuttonRoutine()
//
// =============================================================================
//
//   Note Pins 2 and 3 are designated interrupt pins. Possible uses are:
//   https://www.instructables.com/External-Interrupt-in-arduino/
//
// WIRING:
//   Relay
//   IN2 - IN5     UNO pins 4-7
//   VCC           UNO 5v
//
//   GND           -GND wall wart
//   VCC           no connection
//   JD-VCC        +5v wall wart
// 
//   External power connected to relay GND & JD- in the opto-isolate fashion.
//   https://www.makerspaces.com/wp-content/uploads/2017/02/led-with-switch_bb.jpg
//
//===================================================================================
//
// Steinel motion detector analog pins
// 3.1 volt detection = 220 to 640 value
// (Avg off: 222-225  Avg on: 636-638)
// Note: steinin_val & steinout_val will
//       decrement slowly (85 sec)
const int STEININ = A0;
const int STEINOUT = A1;
int steinin_val = 0;
int steinout_val = 0;
int stein_count = 0;
int OFFcount = 0;   // using counter instead of delay()

// Relay pins for 17 volt motor operation
const int R2 = 4;
const int R3 = 5;
const int R4 = 6;
const int R5 = 7;

// LED light comes on after OFFDOWN pressed
const int OFFLED = 9;

// Magnetic switches
const int BOTTOM = 10;
const int TOP = 11;

// Manual buttons
const int OFFDOWN = 3;  // emergency off gets an interrupt pin
const int ONAUTO = 12;  // restores automatic door after offdown

// Flags, counters
int initialization = 1; // denotes inside setup() segment
int motorRunning = 0;   // no=0/yes=1
int OFFbutton = 0;      // OFF button state.  ('Auto' = 0, 1 = pressed, 2 = active)
int state = 0;          // Door State.  0=idle
int prevState = 0;      // Previous Door State.  0=idle
int stein = 0;          // Steinel switch state. 0=inactive
int prevStein = 0;      // Previous stein State.  0=off
int duration = 0;       // Has motor been running TOO LONG?
unsigned long TOOLONG = 100000;  // about 2 min.

void setup()
{  Serial.begin(9600);

   Serial.println(""); 
   Serial.println(""); 
   Serial.println(""); 
   Serial.println(""); 
   Serial.println(""); 
   Serial.println("");
   Serial.println("BEGIN INITIALIZATION ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^");

   // Relays -- Uses complete Opto-Isolation;
   // Defaults to HIGH (relays off)
   pinMode(R2, INPUT_PULLUP);  // relay #2 (UP)
   pinMode(R2, OUTPUT);
   pinMode(R3, INPUT_PULLUP);  // relay #3 (DOWN)
   pinMode(R3, OUTPUT);
   pinMode(R4, INPUT_PULLUP);  // relay #4 (DOWN)
   pinMode(R4, OUTPUT);
   pinMode(R5, INPUT_PULLUP);  // relay #5 (UP)
   pinMode(R5, OUTPUT);

   pinMode(OFFLED, OUTPUT);    // off/down LED

   pinMode(TOP, INPUT_PULLUP);       // top mag switch
   pinMode(BOTTOM, INPUT_PULLUP);    // bottom mag switch

   // Analog pins look for 3.1 volts
   pinMode(STEININ, INPUT_PULLUP);   // steinel sensor inside
   pinMode(STEINOUT, INPUT_PULLUP);  // steinel sensor outside

   // Manual buttons
   pinMode(ONAUTO, INPUT_PULLUP);    // up/auto manual button
   pinMode(OFFDOWN, INPUT_PULLUP);   // off/down manual button

   steinelInit();

   while (initialization == 1)
   {
      door();

      Serial.print("INIT MR, state, stein: ");
      Serial.print(motorRunning);
      Serial.print(", ");
      Serial.print(state);
      Serial.print(", ");
      Serial.println(stein);

      if (motorRunning == 1)
      {
        if (state == 2)  //at top
        {
          openRelays();
          initialization = 0;     // break out of setup
          break;
        }
      }
      else  // motor not running
      {
          if (state == 2)  //at top, right from the get-go
          {
             openRelays();
             initialization = 0;     // break out of setup
             break;
          }
          
          // door at bottom
          else if (state == -2)
          {   Serial.println("AI1");
              goUpFromBottom();
          }
          
          else if (state != 2)   // door not at top
          {   Serial.println("AI2");
              goUp();
          }
       }  // motor not running
   }  // while

   Serial.println("");
   Serial.println("BEGIN MAIN LOOP #########################################");
}

void loop()
{   
   //  Door states (state):
   //  2 = door at top
   //  1 = going up
   //  0 = door idle/undetermined
   // -1 = going down
   // -2 = at bottom

   // Steinel states (stein):
   // 0 = both off
   // 1 = at least one on

   // Manual button states
   // flag: 0=off, 1=on
   // OFFDOWN pin, OFFbutton enable
   // ONAUTO pin, OFFbutton disable

   // Save last loop's states
   prevState = state;
   prevStein = stein;

   Serial.println("");
   Serial.print("prevState, prevStein: ");
   Serial.print(prevState);
   Serial.print(", ");
   Serial.println(prevStein);
   
   // Determine current Door state
   door();
   
   // Determine current Steinel state ("running average")
   steinel();

   Serial.print("Current MR, state, stein: ");
   Serial.print(motorRunning);
   Serial.print(", ");
   Serial.print(state);
   Serial.print(", ");
   Serial.println(stein);
   Serial.print("stein vals: ");
   Serial.print(steinin_val);
   Serial.print(", ");
   Serial.println(steinout_val);
   
   // Determine OFF button status
   OFFbuttonRoutine();

   if (OFFbutton == 0)
   {
      // States & Transitions in Order of Likelihood +=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+

      if (stein == 0)  //steinel off
      {
         // S1 door at bottom
         if (state == -2)
         {   Serial.println("S1");
      
             if (motorRunning == 1)
             { 
               openRelays();
             }
         }

         // S2 door at top, start going down
         if (state == 2)
         {   Serial.println("S2");
      
             if (motorRunning == 0)
             { 
                goDownFromTop();
             }
         }
 
      } //steinel off

      else if (stein == 1)  //steinel on
      {
         // S3 door at top, steinel on
         if (state == 2)
         {   Serial.println("S3");
      
             if (motorRunning == 1)
             { 
               openRelays();
             }
         }

         // S4 door at bottom, start going up
         if (state == -2)
         {   Serial.println("S4");
      
             if (motorRunning == 0)
             { 
                 goUpFromBottom();
             }
          }

         // S5 door gets signal to go up in middle of going down (i.e., stein was off, now on). No manual
         if (state == -1)
         {   Serial.println("S5");
      
             if ((prevState == -1) and (prevStein == 0))
             {  
               if (motorRunning == 1)   // in middle of going down...
               { 
                  goUp();
               }
             }
         }
     }  // steinel ON
     
   } // OFFbutton is OFF

   else

   { // OFFbutton is ON
    
      // S6 door not at bottom, Manual Offdown
      if ((state != -2) && (OFFbutton == 1))
      {   Serial.println("S6");
      
          if (state != -1)
          { 
             if (motorRunning == 0)
             { 
                goDown();
             }
          }
       
          OFFbutton = 2;  // OFF button pressed and active
      }

      // S7 door hits bottom after manual offdown pressed and active
      if ((state == -2) && (OFFbutton == 2))
      {   Serial.println("S7");
          if (motorRunning == 1)
          {  
             openRelays();
          }
      }

      // S8  Flash LED when door is actively OFF
      if (OFFbutton == 2)
      {   Serial.println("S8");

          OFFcount = OFFcount + 1;

          if (OFFcount == 1)
          {  digitalWrite(OFFLED, HIGH);   // turn the LED on
          }

          if (OFFcount == 10)
          {  digitalWrite(OFFLED, LOW);    // turn the LED off
          }
          
          if (OFFcount == 15)
          {  OFFcount = 0;
          }
      }

   }  // OFFbutton is ON


   // S9 motor running duration check
   if (motorRunning == 1)
   {   
       duration = duration + 1;
   
      if (duration > TOOLONG)
      {   
          Serial.println("Motor has been running too long.  HALTED.");
          Serial.println("");

          openRelays();
          exit(0);
      }
   }
 
}
//====================================================================

void openRelays()
{
   // Turns motor off.
   // Always run this 1st when going up/down to avoid a short.
   
   // Using self-powered opto-isolation
   // 'HIGH' means 'de-energize relay to OFF'
   digitalWrite(R2, HIGH);
   digitalWrite(R3, HIGH);
   digitalWrite(R4, HIGH);
   digitalWrite(R5, HIGH);

   motorRunning = 0;
   duration = 0;
}

void goDown()
{  
   openRelays();
   
   Serial.println("Going Down; Down Down Down Down");
    
   // 'LOW' means 'energize relay to ON'
   digitalWrite(R2, LOW);
   digitalWrite(R5, LOW);

   motorRunning = 1;
   state = -1;
}

void goDownFromTop()
{  
   openRelays();   // resets duration = 0
   
   Serial.println("Going Down; Down Down Down");

   // 'LOW' means 'energize relay to ON'
   digitalWrite(R2, LOW);
   digitalWrite(R5, LOW);
   
   motorRunning = 1;

   while ((digitalRead(TOP) == LOW) && (digitalRead(BOTTOM) == HIGH))
   {   delay(20);
       Serial.println("STILL AT TOP");

       duration = duration + 1;
       if (duration > 100)
       {  Serial.println("");
          Serial.println("goDown taking too long.  HALTED HALTED HALTED.");
          openRelays();
          exit(0);
       }
   }
   
   state = -1;
}

void goUp()
{  
   openRelays();
   
   Serial.println("Going Up; Up Up Up Up");
    
   // 'LOW' means 'energize relay to ON'
   digitalWrite(R3, LOW);
   digitalWrite(R4, LOW);

   motorRunning = 1;
   state = 1;
}

void goUpFromBottom()
{  
   openRelays();   // resets duration = 0
   
   Serial.println("Going Up from bottom; Up Up Up");

   // 'LOW' means 'energize relay to ON'
   digitalWrite(R3, LOW);
   digitalWrite(R4, LOW);
   
   motorRunning = 1;

   while ((digitalRead(BOTTOM) == LOW) && (digitalRead(TOP) == HIGH))
   {   delay(20);
       Serial.println("STILL AT BOTTOM");

       duration = duration + 1;
       if (duration > 100)
       {  Serial.println("");
          Serial.println("goUp taking too long.  HALTED HALTED HALTED.");
          openRelays();
          exit(0);
       }
   }
   
   state = 1;
}

void steinel()
{
   stein_count = stein_count + 1;

   if (stein_count > 10)  // smooth the curve
   {
       // Analog values should be near 
       // 220 (inactive) or 640 (active)
        steinin_val = analogRead(STEININ);
        steinout_val = analogRead(STEINOUT);
       
       if ((steinin_val > 500) || (steinout_val > 500))
       {  
          if (prevStein == 0)
          {  // notify that it's changed
             Serial.println("");
             Serial.print("Steinels now ACTIVE. In, out: ");
             Serial.print(steinin_val);
             Serial.print(", ");
             Serial.println(steinout_val);
          }
         
          stein = 1;
       }
 
       if ((steinin_val < 400) && (steinout_val < 400))
       {   
          if (prevStein == 1)
          {   // notify that it's changed
             Serial.println("");
             Serial.print("Steinels now Inactive. In, out: ");
             Serial.print(steinin_val);
             Serial.print(", ");
             Serial.println(steinout_val);
          }
         
          stein = 0;
       }
       
      stein_count = 0;
   }
}

void door()
{  
   // return resting door state:
   //  2 = door resting at top
   //       1 = going up
   //       0 = undetermined
   //      -1 = going down
   // -2 = door resting at bottom

   if ((digitalRead(TOP) == LOW) && (digitalRead(BOTTOM) == HIGH))
   {  if (prevState != 2)
      { Serial.println("door() says 'Now at TOP'");
      }
      
      state = 2;   // at top
   }
  
   if ((digitalRead(BOTTOM) == LOW) && (digitalRead(TOP) == HIGH))
   {  if (prevState != -2)
      { Serial.println("door() says 'Now at BOTTOM'");
      }
      
      state = -2;   // at bottom
   }
}

void OFFbuttonRoutine()
{  
   if (digitalRead(OFFDOWN) == LOW)
   {  OFFbutton = 1;
   }
   
   if (digitalRead(ONAUTO) == LOW)
   {  OFFbutton = 0;
      digitalWrite(OFFLED, LOW);    // turn the LED off
   }
}

void steinelInit()
{
    steinin_val = analogRead(STEININ);
    steinout_val = analogRead(STEINOUT);
       
    if ((steinin_val > 500) || (steinout_val > 500))
    {   stein = 1;
    }
 
    if ((steinin_val < 400) && (steinout_val < 400))
    {   stein = 0;
    }
}
