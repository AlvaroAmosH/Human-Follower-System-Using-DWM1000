// used for driving motors
int PWM1, PWM2, ratio;

//  Left Motor from front view
const int IN1 = 11;
const int IN2 = 10;
int ForwardPWMLeft = 8;
int ReversePWMLeft = 9;

//  Right Motor from front view
const int IN3 = 7;
const int IN4 = 6;
int ForwardPWMRight = 4;
int ReversePWMRight = 5;

// setup
float tolerance = 10.0; //cm
float distanceFromUser = 150.0;

//Max & Min speeds (from 0 - 200)
int maxPWM, minPWM = 50;

//Hybrid proportions
int propmax = 6;
int propmin = 1;

//all in meters
float dist_b = 0;
float dist_c = 0;
float dist_bc = 0.25;

float s = 0;
float dist = 0;
float x1 = 0;
float x2 = 0;
float x = 0;

////for fuzzy logic
float isMember;
//y
float isClosest;
float isClose;
float isEnough;
float isFar;
float isFarthest;
//x
float isLeft;
float isLeftish;
float isStraight;
float isRightish;
float isRight;

float Y_final, X_final, nY[5], nX[5];
float Min[25], out;
int angle;

bool IS_DEBUG = true;

void setup() {
  Serial.begin(115200);
  Serial1.begin(9600);
  Serial2.begin(9600);
}

void loop() {
  getSerialInput();
  getSerialDistanceInput();
}

void getSerialDistanceInput() {
  String inStr = "";
  bool rcv = false;
  while (Serial1.available() > 0) {
    char  inChar =  (char) Serial1.read();
    if (rcv && inChar != '#' && inChar != '\r' && inChar != '\n') {
      inStr += inChar;
    }
    if (inChar == '#') {
      rcv = true;
    }
    if (inChar == '\r' || inChar == '\n') {
      rcv = false;
      break;
    }
  }
  if (inStr != "") {
    inStr.toLowerCase();
    inStr.trim();
    inStr.replace("#", "");
    if (IS_DEBUG) {
      Serial.print("Data from port 1:");
      Serial.println(inStr);
    }
    if (inStr.toFloat() != 0) {
      dist_b = inStr.toFloat();
    }
  }


  String inStr2 = "";
  bool rcv2 = false;
  while (Serial2.available() > 0) {
    char  inChar =  (char) Serial2.read();
    if (rcv2 && inChar != '#' && inChar != '\r' && inChar != '\n') {
      inStr2 += inChar;
    }
    if (inChar == '#') {
      rcv2 = true;
    }
    if (inChar == '\r' || inChar == '\n') {
      rcv2 = false;
      break;
    }
  }
  if (inStr2 != "") {
    inStr2.toLowerCase();
    inStr2.trim();
    inStr2.replace("#", "");
    if (IS_DEBUG) {
      Serial.print("Data from port 2:");
      Serial.println(inStr2);
    }
    if (inStr2.toFloat() != 0) {
      dist_c = inStr2.toFloat();
    }
  }

  if (dist_b > 0 && dist_c > 0) {
    //Heron's Formula
    s = (dist_b + dist_c + dist_bc) / 2;

    dist = (sqrt(s * (s - dist_b) * (s - dist_c) * (s - dist_bc)) * 2) / dist_bc ;
    //THIS IS THE Y.
    if (isnan(dist) != 1) {
      x1 = sqrt(abs(pow(dist,2) - pow(dist_b,2)));
      if (dist < dist_b) {
        x1 = -x1;
      }
      x2 = sqrt(abs(pow(dist,2) - pow(dist_c,2)));
      if (dist > dist_c) {
        x2 = -x2;
      }
      x = x1 - x2;
      if (x2 > x1) {
        x = x + dist_bc/2;
      }
      else if (x1 > x2) {
        x = x - dist_bc/2;
      }
      
      if (IS_DEBUG) {
        Serial.print("Distance : y = "); Serial.print(dist); 
        Serial.print(", x1 = "); Serial.print(x1);  
        Serial.print(", x2 = "); Serial.print(x2); 
        Serial.print(", x = "); Serial.println(x); 
      }
      else {
        Serial.println("Distance#y=" + String(dist) + ",x1=" + String(x1) + ",x2=" + String(x2));
      }
    }

    //CONNECT TO MOTOR DRIVER
      fuzzification();
      defuzzification();
      drivingMotors();
    //CONNECT TO MOTOR DRIVER
  }
  delay(1000);
}


void getSerialInput() {
  String inStr = "";
  while (Serial.available() > 0) {
    inStr = Serial.readString();
    inStr.toLowerCase();
    inStr.trim();
  }
  if (inStr != "") {
    if (inStr.startsWith("debug")) {
      inStr.replace("debug ", "");
      if (inStr.toInt() > 0) {
        IS_DEBUG = true;
        Serial.println("Debugging enabled");
        delay(1000);
      }
      else {
        IS_DEBUG = false;
        Serial.println("Debugging disabled");
      }
    }
  }
  inStr = "";
}

void fuzzification() {
  //y
  isMember = 0;
  memberification(1, dist, 0.5, 0.5, 0.75);
  isClosest = isMember;
  memberification(2, dist, 0.5, 0.75, 1.00);
  isClose = isMember;
  memberification(2, dist, 0.75, 1.00, 1.25);
  isEnough = isMember;
  memberification(2, dist, 1.00, 1.25, 1.5);
  isFar = isMember;
  memberification(3, dist, 1.25, 1.5, 1.5);
  isFarthest = isMember;

  //x
  isMember = 0;
  memberification(1, x, -0.6, -0.3, -0.12);
  isLeft = isMember;
  memberification(2, x, -0.3, -0.12, 0.0);
  isLeftish = isMember;
  memberification(2, x, -0.12, 0.0, 0.12);
  isStraight = isMember;
  memberification(2, x, 0.0, 0.12, 0.3);
  isRightish = isMember;
  memberification(3, x, 0.12, 0.3, 0.6);
  isRight = isMember;
}

void memberification(int type, float value, float A, float B, float C) {
  switch (type) {
    case 1: 
      if (value <= B) isMember = 1;
      if ((value > B) && (value < C)) isMember = (C - value) / (C - B);
      if (value >= C) isMember = 0;
      break;
    case 2:
      if ((value <= A) || (value >= C)) isMember = 0;
      if ((value > A) && (value < B)) isMember = (value - A) / (B - A);
      if ((value > B) && (value < C)) isMember = (C - value) / (C - B);
      if (value == B) isMember = 1;
      break;
    case 3:
      if (value <= A) isMember = 0;
      if ((value > A) && (value < B)) isMember = (value - A) / (B - A);
      if (value >= B) isMember = 1;
      break;
  }
}

int fuzzy_set[5][5] = {
  //Left|Leftish|Straight|Rightish|Right
  {100, 90, 90, 90, 100}, //Farthest
  {90,  70, 60, 70, 90},  //Far
  {90,  40, 0,  40, 90},  //Enough
  {80,  60, 0,  60, 80},  //Close
  {80,  60, 0,  60, 80}   //Closest
};

void defuzzification() {
  float num = 0, denum = 0, center_of_area = 0;
  nY[5] = {};
  nX[5] = {};

  for (int set = 0; set < 25;) {
    for (int i = 0; i < 5; i++) {
      for (int j = 0; j < 5; j++) {
        float data_Y[5] = {isFarthest, isFar, isEnough, isClose, isClosest};
        nY[i] = data_Y[i];
        float data_X[5] = {isLeft, isLeftish, isStraight, isRightish, isRight};
        nX[j] = data_X[j];

        Y_final = max(nY[i], Y_final);
        X_final = max(nX[j], X_final);

        // Center of Area Method
        Min[set] = min(nY[i], nX[j]);
        num += Min[set] * fuzzy_set[i][j];
        denum += Min[set];
        delay(5);
        set++;
      }
    }
  }
  center_of_area = num / denum;
  out = center_of_area;
  ifs();
}

void ifs() {
       if (Y_final == isFarthest && X_final == isLeft) angle = -45;
  else if (Y_final == isFarthest && X_final == isLeftish) angle = -30;
  else if (Y_final == isFarthest && X_final == isStraight) angle = 0;
  else if (Y_final == isFarthest && X_final == isRightish) angle = 30;
  else if (Y_final == isFarthest && X_final == isRight) angle = 45;

  else if (Y_final == isFar && X_final == isLeft) angle = -60;
  else if (Y_final == isFar && X_final == isLeftish) angle = -45; 
  else if (Y_final == isFar && X_final == isStraight) angle = 0;
  else if (Y_final == isFar && X_final == isRightish) angle = 45;
  else if (Y_final == isFar && X_final == isRight) angle = 60;

  else if (Y_final == isEnough && X_final == isLeft) angle = -90;
  else if (Y_final == isEnough && X_final == isLeftish) angle = -90;
  else if (Y_final == isEnough && X_final == isStraight) angle = -180;
  else if (Y_final == isEnough && X_final == isRightish) angle = 90;
  else if (Y_final == isEnough && X_final == isRight) angle = 90;

  else if (Y_final == isClose && X_final == isLeft) angle = -120;
  else if (Y_final == isClose && X_final == isLeftish) angle = -135;
  else if (Y_final == isClose && X_final == isStraight) angle = -180;
  else if (Y_final == isClose && X_final == isRightish) angle = 135;
  else if (Y_final == isClose && X_final == isRight) angle = 120;

  else if (Y_final == isClosest && X_final == isLeft) angle = -135;
  else if (Y_final == isClosest && X_final == isLeftish) angle = -150;
  else if (Y_final == isClosest && X_final == isStraight) angle = -180;
  else if (Y_final == isClosest && X_final == isRightish) angle = 150;
  else if (Y_final == isClosest && X_final == isRight) angle = 135;
}

void drivingMotors() {
  maxPWM = map(out, 0, 100, 50, 200);
  bool forward;
  if (abs(angle) <= 90) {
    ratio = map(abs(angle), 0, 90, propmin, propmax);
    forward = 1;
  }
  else {
    forward = 0;
  }

  if (angle == 0) { //front
    PWM1 = maxPWM;
    PWM2 = PWM1;
    analogWrite(ForwardPWMLeft, PWM1);
    analogWrite(ReversePWMLeft, 0);
    analogWrite(ForwardPWMRight, PWM2);
    analogWrite(ReversePWMRight, 0);
  }
  else if (angle == -180) { //stop
    analogWrite(ForwardPWMLeft, 0);
    analogWrite(ReversePWMLeft, 0);
    analogWrite(ForwardPWMRight, 0);
    analogWrite(ReversePWMRight, 0); 
  }
  else if (angle < 0 && forward) { //left forward
    PWM1 = maxPWM;
    PWM2 = max(PWM1/ratio, minPWM);
    analogWrite(ForwardPWMLeft,PWM1);
    analogWrite(ReversePWMLeft,0);
    analogWrite(ForwardPWMRight,PWM2);
    analogWrite(ReversePWMRight,0);
  }
  else if (angle < 0 && !forward) { //left stop
    PWM1 = maxPWM;
    PWM2 = PWM1;
    analogWrite(ForwardPWMLeft,PWM1);
    analogWrite(ReversePWMLeft,0);
    analogWrite(ForwardPWMRight,0);
    analogWrite(ReversePWMRight,PWM2);
  }
  else if (angle > 0 && forward) { //right forward
    PWM2 = maxPWM;
    PWM1 = max(PWM2/ratio, minPWM);
    analogWrite(ForwardPWMLeft,PWM1);
    analogWrite(ReversePWMLeft,0);
    analogWrite(ForwardPWMRight,PWM2);
    analogWrite(ReversePWMRight,0);
  }
  else if (angle > 0 && !forward) { //right stop
    PWM2 = maxPWM;
    PWM1 = PWM2;
    analogWrite(ForwardPWMLeft,0);
    analogWrite(ReversePWMLeft,PWM1);
    analogWrite(ForwardPWMRight,PWM2);
    analogWrite(ReversePWMRight,0);
  }
}
