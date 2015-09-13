 // Code for controlling a CNC machine (see https://hackaday.io/project/7603-garden-shed-cnc)
 // written by Laurence Shann (laurence.shann@gmail.com)
 
#include <Servo.h>
#include <cnclib.h>

int TICKS_PER_MM = 10; // 8 clicks per turn, 0.8mm per turn
char servo_pin[] = {0,11,12};
char step_pin[] = {1,5,8};
char min_pin[] = {2,6,10};
char max_pin[] = {3,7,9};
char halt_pin = 4;
char move_pins[3][2] = {{A4,A5},{A1,A0},{A3,A2}};
char led_pin = 13;

String response_success = "DONE";

Servo servo[3];
boolean servo_on[3];
boolean click_state[3];
String inputString = "";
boolean stringComplete = false;
vec3 headPosition;
int lastaxis=0;

void setup() 
{
  ZeroHead();
  pinMode(led_pin, OUTPUT);
  pinMode(halt_pin, INPUT_PULLUP);
  for (int axis=0; axis<3; axis++){
    pinMode(step_pin[axis], INPUT_PULLUP);
    pinMode(min_pin[axis], INPUT_PULLUP);
    pinMode(max_pin[axis], INPUT_PULLUP);
    pinMode(move_pins[axis][0], INPUT_PULLUP);
    pinMode(move_pins[axis][1], INPUT_PULLUP);
    servo_on[axis]=false;
  }
  
  Serial.begin(9600);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for Leonardo only
  }
  Serial.println("ready");
  inputString.reserve(200);
  
} 

void loop() 
{ 
    if (Serial.available()){
      char inChar = Serial.read();
      if(inputString.length()>0 || (inChar>='A' && inChar<='Z')) inputString += inChar;
      if (inChar == '\r') {
        stringComplete = true;
      } 
      Serial.write(inChar);
    }
    
    if(stringComplete){
      Serial.println(" ... ");
      Serial.println("arduino received command");
      if(inputString[0]=='G') GCodeCommand();
      else if(inputString[0]=='Z'){ Serial.println("zeroing head"); ZeroHead();}
      else{
        Serial.print("unrecognized command [");
        Serial.print(inputString);
        Serial.println("]");
      }
      Serial.println(response_success);
      inputString="";
      stringComplete = false;
    }
    
    ManualControl();

    boolean state = digitalRead(step_pin[lastaxis]);
    digitalWrite(led_pin, state?HIGH:LOW);
}


boolean checkLimit(int delta, char axis){
    if(delta>0) return digitalRead(min_pin[axis])==LOW;
    else return digitalRead(max_pin[axis])==LOW;
}

boolean checkHalt(){
  return digitalRead(halt_pin)==LOW;
}

boolean checkBtn(char axis, char num){
  return digitalRead(move_pins[axis][num])==LOW;
}


boolean ManualControl(){
  boolean abortSeq = false;
  vec3 delta;
  delta.x=headPosition.x;
  delta.y=headPosition.y;
  delta.z=headPosition.z;
  
  if(checkHalt()) abortSeq=true;
  
  for(int a=0; a<3; a++){
     for(int n=0; n<2; n++){
      if(checkBtn(a,n)){
        abortSeq=true;
        if(a==0) delta.z = headPosition.z+n*2-1;
        if(a==1) delta.x = headPosition.x+n*2-1;
        if(a==2) delta.y = headPosition.y+n*2-1;
      }
     } 
    }
    if(delta.x!=headPosition.x || delta.y!=headPosition.y || delta.z!=headPosition.z) headToTarget(delta);
    return abortSeq;
}

void ZeroHead(){
  headPosition.x=headPosition.y=headPosition.z=0;
}

int vectorAxis(int axis, vec3 vector){
  if(axis==0) return vector.z;
  else if(axis==1) return vector.x;
  else if(axis==2) return vector.y;
  else return -1;
}

void moveHead(int delta, int axis){
  if(axis==0) headPosition.z+=delta;
  if(axis==1) headPosition.x+=delta;
  if(axis==2) headPosition.y+=delta;
}

void stopHead(){
  for(int axis=0; axis<3; axis++){
    servo[axis].detach();
    servo_on[axis]=false;
  }
}

boolean headToTarget(vec3 target){
  for(int axis=0; axis<3; axis++){
    click_state[axis] = digitalRead(step_pin[axis]);
  }
  int idleAxes=0;
  do{
    idleAxes=0;
    for(int axis=0; axis<3; axis++){
      int delta = vectorAxis(axis,target) - vectorAxis(axis, headPosition);
      if(!checkLimit(delta, axis)) delta=0;
      
      if(delta==0 && servo_on[axis]){ servo[axis].detach(); servo_on[axis]=false;}
      else if (delta!=0 && !servo_on[axis]){ servo[axis].attach(servo_pin[axis]); servo_on[axis]=true;}
      
      if(delta!=0)
      {
        servo[axis].write(90+delta*90);
        boolean state = digitalRead(step_pin[axis]);
        digitalWrite(led_pin, state?HIGH:LOW);
        lastaxis=axis;
        if(state!=click_state[axis])
        {
          moveHead( (delta>0)?1:-1 ,axis);
        }
      }else{
        idleAxes++; 
      }
    }
    delay(1);
    while (Serial.available()){
      if(Serial.read() == 'X'){
        return false;
      }
    }
  }while(idleAxes<3 && !checkHalt());
  
  return true;
}

void GCodeCommand(){
  int len = inputString.length();
  char buf[len];
  inputString.toCharArray(buf,len);
  gCode c = cnclib::codeFromLine(buf, TICKS_PER_MM);
  lineCoords line;
  line.from = headPosition;
  line.to.x = (c.x==cnclib::UNUSED_AXIS)?headPosition.x:-c.x;
  line.to.y = (c.y==cnclib::UNUSED_AXIS)?headPosition.y:-c.y;
  line.to.z = (c.z==cnclib::UNUSED_AXIS)?headPosition.z:c.z;
  line.arc.x = (c.i==cnclib::UNUSED_AXIS)?headPosition.x:-c.i;
  line.arc.y = (c.j==cnclib::UNUSED_AXIS)?headPosition.y:-c.j;
  line.arc.z = (c.k==cnclib::UNUSED_AXIS)?headPosition.z:c.k;
  if(c.num==2) line.arcDirection = 1;
  if(c.num==3) line.arcDirection = -1;
  
  Serial.println("");
  Serial.println(c.num);
  if(c.num==1) Serial.println("--straight line---");
  if(c.num==2 || c.num==3) Serial.println("--curved line---");
  Serial.print("from x: "); Serial.print(line.from.x);
  Serial.print(" y: "); Serial.print(line.from.y);
  Serial.print(" z: "); Serial.println(line.from.z);
  Serial.print("to x: "); Serial.print(line.to.x);
  Serial.print(" y: "); Serial.print(line.to.y);
  Serial.print(" z: "); Serial.println(line.to.z);
  if(c.num==2 || c.num==3){
    Serial.print("center x: "); Serial.print(line.arc.x);
    Serial.print(" y: "); Serial.print(line.arc.y);
    Serial.print(" z: "); Serial.println(line.arc.z);
  }
  
  if(c.num==1) StraightLine(line);
  if(c.num==2 || c.num==3) CurvedLine(line);
}

void CurvedLine(lineCoords line){
  vec3 target;
  int steps = cnclib::curve_numSteps(line);
  Serial.print("steps ");
  Serial.println(steps);
  for(int s=1; s<=steps; s++){
  //while(true){
    target = cnclib::addVec3(headPosition, cnclib::curve_delta(line, headPosition));
    if (!headToTarget(target)) break;
    if(ManualControl()) break;
  }
  stopHead();
}

void StraightLine(lineCoords line){
  vec3 target;
  int steps = cnclib::line_numSteps(line);
  Serial.print("steps ");
  Serial.println(steps);
  for(int s=1; s<=steps; s++){
    target = cnclib::addVec3(headPosition, cnclib::line_delta(line, s, steps));
    if (!headToTarget(target)) break;
    if(ManualControl()) break;
  }
  stopHead();
}

