// --------------------------------------
// Global Variables
// --------------------------------------
#include <string.h>
#include <stdio.h>

// --------------------------------------
// Global Variables
// --------------------------------------
float speed = 55.0;
int minimumSecureSpeed = 40;
int maximumSecureSpeed = 70;
int gasState = 0; // 0 = off && 1 = on
int brakeState = 0; // 0 = off && 1 = on
int mixState = 0; // 0 = off && 1 = on
int slope; // 0 = FLAT && 1 = UO && -1 = DOWN
unsigned long previousMillis = 0;
int lamState = 0; // 0 = off && 1 = on
int lit = 0.0;
long selectorDistance = 0; // 10000 - 90000
long distanceDeposit;
float actualDistanceToDeposit = 0.0;
float diffDistance;
int unloadState = 0; // 0 = off && 1 = frenando $$ 2 = parado
int emergence = 0;

// --------------------------------------
// Constant Variables
// --------------------------------------
const long interval = 200;


// --------------------------------------
// Function: check_accel
// --------------------------------------
int check_accel(){
  if(gasState == 1){
    digitalWrite(13, HIGH);
  }else{
    digitalWrite(13, LOW);
  }
  return 0;
}
// --------------------------------------
// Function: check_brake
// --------------------------------------
int check_brake(){
  if(brakeState == 1){
    digitalWrite(12, HIGH);
  }else{
    digitalWrite(12, LOW);
  }
  return 0;
}


// --------------------------------------
// Function: check_mix
// --------------------------------------
int check_mix(){
  if(mixState == 1){
    digitalWrite(11, HIGH);
  }else{
    digitalWrite(11, LOW);
  }
  return 0;
}

// --------------------------------------
// Function: check_slope
// --------------------------------------
int check_slope(){
  if (digitalRead(8) == HIGH) {
    slope = -1;
  } else if (digitalRead(9) == HIGH) {
    slope = 1;
  } else {
    slope = 0;
  }
  return 0;
}

// --------------------------------------
// Function: represent_speed
// --------------------------------------
int represent_speed(){
  float accel = 0.0;
  if(gasState == 1)  {  accel += 0.5; }
  if(brakeState == 1)  {  accel -= 0.5; }
  if(slope == 1)  {  accel -= 0.25; }
  if(slope == -1)  {  accel += 0.25; }
  speed += (accel * 0.1);
  if(speed < minimumSecureSpeed || speed > maximumSecureSpeed){
    return -1;
  }

  analogWrite(10, map(speed, 40, 70, 0, 255));
}

// --------------------------------------
// Function: check_lam
// --------------------------------------
int check_lam(){
  if(lamState == 1){
    digitalWrite(7,HIGH);
  }else{
    digitalWrite(7,LOW);
  }
  return 0;
}

// --------------------------------------
// Function: check_lit
// --------------------------------------
int check_lit(){
  lit = analogRead(A0);
  lit = map(lit, 250, 680, 0, 99);
  return 0;
}

// --------------------------------------
// Function: check_deposit_distance
// --------------------------------------
int check_deposit_distance(){
  selectorDistance = map(analogRead(A1), 500, 1023, 10000, 90000);
  return 0;
}

// --------------------------------------
// Function: display_distance_segment
// --------------------------------------
int display_distance_segment(){
  PORTD = selectorDistance/10000 << 2;  //Rotaciona 2 vezes para esq. o contador e manda para os pinos de saída
  return 0;
}

// --------------------------------------
// Function: validation_distance
// --------------------------------------
int validation_distance(){
  if(digitalRead(6) == HIGH){
    distanceDeposit = selectorDistance;
    unloadState = 1;
  }
  return 0;
}

// --------------------------------------
// Function: display_real_distance_deposit
// --------------------------------------
int display_real_distance_deposit(){

    float t = 0.1;
    float accel = 0.0;
    if(gasState == 1)  {  accel += 0.5; }
    if(brakeState == 1)  {  accel -= 0.5; }
    if(slope == 1)  {  accel -= 0.25; }
    if(slope == -1)  {  accel += 0.25; }
    actualDistanceToDeposit += (speed * t) + (0.5 * accel * pow(t, 2));
    diffDistance = distanceDeposit - actualDistanceToDeposit;

    Serial.println(diffDistance);
    PORTD = (long)diffDistance/10000 << 2;  //Rotaciona 2 vezes para esq. o contador e manda para os pinos de saída
    if(diffDistance <= 0){
       if(speed < 10){
         unloadState = 2;  // to mode stop
       }else{
         unloadState = 0;  // to mode selector
     }
   }

  return 0;
}



// --------------------------------------
// Function: set_speed_zero()
// --------------------------------------
int set_speed_zero(){
  speed = 0;
  return 0;
}

// --------------------------------------
// Function: read_end_stop()
// --------------------------------------
int read_end_stop(){
  if(digitalRead(6) == HIGH){
    unloadState = 0;
  }
  return 0;
}

// --------------------------------------
// Function: check_emergence()
// --------------------------------------
int check_emergence(){
  if(emergence){
    brakeState=1;
    gasState=0;
    lamState=1;
  }
  Serial.println(speed);
  return 0;
}

// --------------------------------------
// Function: comm_server
// --------------------------------------
int comm_server()
{
    int i;
    char arg[10];
    char request[10];
    char answer[10];
    int speed_int;
    int speed_dec;

    // while there is enough data for a request
    if (Serial.available() >= 9) {
        // read the request
        i=0;
        while ( (i<9) && (Serial.available() >= (9-i)) ) {
            // read one character
            request[i]=Serial.read();

            // if the new line is positioned wrong
            if ( (i!=8) && (request[i]=='\n') ) {
                // Send error and start all over again
                sprintf(answer,"MSG: ERR\n");
                Serial.print(answer);
                memset(request,'\0', 9);
                i=0;
            } else {
              // read the next character
              i++;
            }
        }
        request[9]='\0';

        // cast the request
        if (0 == strcmp("SPD: REQ\n",request)) {
            // send the answer for speed request
            speed_int = (int)speed;
            speed_dec = ((int)(speed*10)) % 10;
            sprintf(answer,"SPD:%02d.%d\n",speed_int,speed_dec);
            Serial.print(answer);
        } else if (1 == sscanf(request,"GAS: %s\n",arg)) {
             if (0 == strcmp(arg,"SET")) {
                // activar acelerador
                gasState=1;
                strcpy (answer,"GAS:  OK\n");
              } else if (0 == strcmp(arg,"CLR")) {
                // desactivar acelerador
                gasState=0;
                strcpy (answer,"GAS:  OK\n");
              } else {
                // error
                strcpy (answer,"MSG: ERR\n");
              }
        } else if (1 == sscanf(request,"BRK: %s\n",arg)) {
              if (0 == strcmp(arg,"SET")) {
                // activar freno
                brakeState=1;
                strcpy (answer,"BRK:  OK\n");
              } else if (0 == strcmp(arg,"CLR")) {
                // desactivar freno
                brakeState=0;
                strcpy (answer,"BRK:  OK\n");
              } else {
                // error
                strcpy (answer,"MSG: ERR\n");
              }
            } else if (1 == sscanf(request,"MIX: %s\n",arg)) {
              if (0 == strcmp(arg,"SET")) {
                // activar mixer
                mixState=1;
                strcpy (answer,"MIX:  OK\n");
              } else if (0 == strcmp(arg,"CLR")) {
                // desactivar mixer
                mixState=0;
                strcpy (answer,"MIX:  OK\n");
              } else {
                // error
                strcpy (answer,"MSG: ERR\n");
              }

            // peticiones de informacin, devolver algo
        } else if (0 == strcmp(request,"SLP: REQ\n")) {
            // devolver pendiente
            check_slope();
            switch(slope){
              case 0:
                sprintf (answer,"SLP:FLAT\n");
                break;
              case -1:
                sprintf (answer,"SLP:DOWN\n");
                break;
              case 1:
                sprintf (answer,"SLP:  UP\n");
                break;
            }
            Serial.print(slope);
            Serial.print("\n");
        // si no coincide con ninguno, error
        } else if (1 == sscanf(request,"LAM: %s\n",arg)) {
              if (0 == strcmp(arg,"SET")) {
                // activar mixer
                lamState=1;
                strcpy (answer,"MIX:  OK\n");
              } else if (0 == strcmp(arg,"CLR")) {
                // desactivar mixer
                lamState=0;
                strcpy (answer,"MIX:  OK\n");
              } else {
                // error
                strcpy (answer,"MSG: ERR\n");
              }

            // peticiones de informacin, devolver algo
        } else if (0 == strcmp("LIT: REQ\n",request)) {
            sprintf(answer,"SPD: %d\n",lit);
            Serial.print(answer);

        } else if (0 == strcmp(request,"DS:  REQ\n")) {
            // devolver distancia
            sprintf(answer,"DS:%ld\n", (long) diffDistance);
            Serial.print(answer);
        } else if (0 == strcmp(request,"STP: REQ\n")) {
            if(unloadState == 2){
              sprintf(answer,"STP:  GO\n");
            }else{
              sprintf(answer,"STP:STOP\n");
            }
            Serial.print(answer);
        } else if (0 == strcmp(request,"ERR: SET\n")) {
            emergence = 1;
            sprintf(answer,"ERR:  OK\n");
            Serial.print(answer);
        } else {
            // error, send error message
            sprintf(answer,"MSG: ERR\n");
            Serial.print(answer);
        }
    }
    return 0;
}

// --------------------------------------
// Function: setup
// --------------------------------------
void setup() {
    Serial.begin(9600);
    pinMode(13, OUTPUT); // gas
    pinMode(12, OUTPUT); // brake
    pinMode(11, OUTPUT); // mix
    pinMode(10, OUTPUT); // speed
    pinMode(9, INPUT);   // slope up
    pinMode(8, INPUT);   // slope down
    pinMode(7, OUTPUT);   // lam
    pinMode(A0, INPUT);   // lit
    DDRD = B00111100; // Configura os pinos digitais 2, 3, 4 e 5 como saída digital

}

// --------------------------------------
// Function: loop
// --------------------------------------
void loop() {
  unsigned long currentMillis = millis();
  comm_server();
  if(!emergence){
    check_accel();
  }
  check_brake();
  check_mix();
  check_slope();
  represent_speed();
  check_lam();
  check_lit();
  if(unloadState == 0){ // selector mode
    check_deposit_distance();
    display_distance_segment();
    validation_distance();
  }else if(unloadState == 1){  // Modo de acercamiento al deposito
    display_real_distance_deposit();
  }else{  // Modo de parada
    set_speed_zero();
    read_end_stop();
  }
  check_emergence();

  if (currentMillis - previousMillis < interval) {
     delay(interval - (currentMillis - previousMillis));
  }
  previousMillis = currentMillis;

}
