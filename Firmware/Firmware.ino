#include "TimerOne.h"
#include "CommandParser.h"

typedef CommandParser<> MyCommandParser;
MyCommandParser parser;

#define HEART_BEAT 13

#define MOTOR1_P 11
#define MOTOR1_D 10

#define MOTOR2_P 9
#define MOTOR2_D 8

#define MOTOR3_P 7
#define MOTOR3_D 6

#define MOTOR4_P 5
#define MOTOR4_D 4

#define MOTOR5_T 3

int heart_value = 0;
bool heart_pulse = false;

int motor_P[4] = { MOTOR1_P, MOTOR2_P, MOTOR3_P, MOTOR4_P };
int motor_D[4] = { MOTOR1_D, MOTOR2_D, MOTOR3_D, MOTOR4_D };

bool motor_power_off = true;
bool motor_pulse = false;
int64_t m_positions[4];
int64_t c_positions[4];

const int64_t tick_ = 100;
const int64_t limit_ = 12500;
const int64_t center_ = limit_ / 2;

int64_t smooth_step = 500;
int64_t smooth_period = 10;
int64_t c_smooths[4];
int64_t p_smooths[4];

void setup()
{
  Serial.begin(115200);
  parser.registerCommand("read", "i", &cmd_read);
  parser.registerCommand("write", "ii", &cmd_write);
  parser.registerCommand("power", "i", &cmd_motor_power);
  parser.registerCommand("ready", "i", &cmd_motion_ready);
  parser.registerCommand("position", "iiii", &cmd_position);
  parser.registerCommand("torque", "i", &cmd_torque);
  parser.registerCommand("smooth", "ii", &cmd_set_smooth);

  pinMode(HEART_BEAT, OUTPUT);
  
  pinMode(A0, OUTPUT);
  digitalWrite(A0, motor_power_off);
  
  for (int i = 0; i < 4; i++)
  {
    pinMode(motor_P[i], OUTPUT);
    pinMode(motor_D[i], OUTPUT);
    
    digitalWrite(motor_P[i], false);
    digitalWrite(motor_D[i], false);
    
    m_positions[i] = 0;
    c_positions[i] = 0;
    
    c_smooths[i] = 0;
    p_smooths[i] = smooth_period;
  }
  
  pinMode(MOTOR5_T, OUTPUT);
  
  Timer1.initialize(tick_);
  Timer1.attachInterrupt(motor_pulse_generator);
}

void loop()
{
  if (Serial.available() == false)
    return;
  
  char line[128];
  size_t lineLength = Serial.readBytesUntil('\n', line, 127);
  line[lineLength] = '\0';
  
  char response[MyCommandParser::MAX_RESPONSE_SIZE];
  parser.processCommand(line, response);
  Serial.println(response);
}

void cmd_read(MyCommandParser::Argument *args, char *response)
{
  int index = args[0].asInt64;
  
  int value = digitalRead(index);
  itoa(value, response, 10);
}

void cmd_write(MyCommandParser::Argument *args, char *response)
{
  int index = args[0].asInt64;
  int value = args[1].asInt64;
  
  digitalWrite(index, value);
  itoa(value, response, 10);
}

void cmd_motor_power(MyCommandParser::Argument *args, char *response)
{
  noInterrupts();
  
  bool on = args[0].asInt64 != 0;
  motor_power_off = !on;
  
  for (int i = 0; i < 4; i++)
  {
    m_positions[i] = 0;
    c_positions[i] = 0;
    
    c_smooths[i] = 0;
    p_smooths[i] = smooth_period;
  }
  
  interrupts();
  
  digitalWrite(A0, motor_power_off);
  itoa(on, response, 10);
}

void cmd_motion_ready(MyCommandParser::Argument *args, char *response)
{
  noInterrupts();
  
  bool ready = args[0].asInt64 != 0;
  
  for (int i = 0; i < 4; i++)
  {
    if (ready)
      m_positions[i] = center_;
    else
      m_positions[i] = 0;
  }
  
  interrupts();
  
  itoa(ready, response, 10);
}

void cmd_position(MyCommandParser::Argument *args, char *response)
{
  noInterrupts();
  
  for (int i = 0; i < 4; i++)
  {
    int64_t p = args[i].asInt64 + center_;
    
    if (p < 0) p = 0;
    if (p > limit_) p = limit_;
    
    m_positions[i] = p;
  }
  
  interrupts();
  
  itoa(0, response, 10);
}

void cmd_torque(MyCommandParser::Argument *args, char *response)
{
  int value = args[0].asInt64;
  
  analogWrite(MOTOR5_T, value);
  itoa(value, response, 10);
}

void cmd_set_smooth(MyCommandParser::Argument *args, char *response)
{
  noInterrupts();
  
  smooth_step = args[0].asInt64;
  smooth_period = args[1].asInt64;
  
  for (int i = 0; i < 4; i++)
  {
    c_smooths[i] = 0;
    p_smooths[i] = smooth_period;
  }
  
  interrupts();
}

void motor_pulse_generator()
{
  if (motor_power_off)
  {
    digitalWrite(HEART_BEAT, LOW);
    return;
  }

  heart_value++;
  if (heart_value >= 5000)
  {
    heart_value = 0;
    heart_pulse = !heart_pulse;
  }
  
  digitalWrite(HEART_BEAT, heart_pulse);
  
  
  for (int i = 0; i < 4; i++)
  {
    if (motor_pulse)
    {
      digitalWrite(motor_P[i], false);
      continue;
    }

    int64_t d = m_positions[i] - c_positions[i];
    int64_t absd = abs(d);
    digitalWrite(motor_D[i], d <= 0); // true: CCW, false: CW
    
    if (absd > 0)
    {
      if (absd < smooth_step)
      {
        if (p_smooths[i] != 0)
          c_smooths[i]++;
        
        if (c_smooths[i] > p_smooths[i])
        {
          c_smooths[i] = 0;
          continue;
        }
      }
      
      digitalWrite(motor_P[i], true);
      
      if (d > 0)
        c_positions[i]++;
      else
        c_positions[i]--;
    }
  }

  motor_pulse = !motor_pulse;
}
