int inPin = 1; //whatever the input pin is
int outPin = 0; //output to tell other hardware if software failed
int heartBeat = 0;   //output value, start LOW
int lastBeat = 0; //to check for changing vals

//wait for X amount of signal changes (heart beats) within a short 
//time period (to make sure power cycles or other transients doesn't 
//accidentally turn it on)
//no idea what these should be
int changeCounter = 0; //just looking for changes
int waitPeriod = 260;
int startTime = 0;
int currTime = 0;
int goal = 5; 
int state = 0;
  
  
void setup() {
  pinMode(outPin, OUTPUT);
  pinMode(inPin, INPUT);
  //START LOW
  digitalWrite(outPin, 0);
}

void loop() {
  switch(state){
    
    //NOT STARTED YET
    case 0:
      currTime = millis();
      //check for change in hearbeat
      heartBeat = digitalRead(inPin);
      if (heartBeat != lastBeat){ changeCounter++; }
      
      lastBeat = heartBeat;
    
      //if we get 5 changes in a span of 260ms continue to healthy state
      if (changeCounter > goal){
        //sets everything up for the next state
        changeCounter = 0;
        waitPeriod = 52;
        startTime = millis();
        goal = 1; 
        state = 1;
      }
      
      //reset counters to continue waiting
      if (currTime - startTime > waitPeriod){
        startTime = millis();
        changeCounter = 0;
      }
      break;
      
    //HEALTHY STATE
    case 1:
      digitalWrite(outPin, 1);
      heartBeat = digitalRead(inPin);
      currTime = millis();
      //check for change in hearbeat
      
      if (heartBeat != lastBeat){ changeCounter++; }
      
      lastBeat = heartBeat;
      
      if (currTime - startTime >= waitPeriod){
        if (changeCounter < goal){
          state = 2;
        }
        startTime = millis();
        changeCounter = 0;
      }
      break;
      
    //DEAD STATE
    case 2:
        digitalWrite(outPin, 0);
        while (1){ 
          //DO NOTHING 
        }
  }
}

