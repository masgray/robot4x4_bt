#include <MapleFreeRTOS900.h>

constexpr int PinWheelLeftUp PROGMEM     = PB1;
constexpr int PinWheelLeftDown PROGMEM   = PB0;
constexpr int PinWheelRightUp PROGMEM    = PA7;
constexpr int PinWheelRightDown PROGMEM  = PA6;

auto& BTSerial = Serial2;

enum class Commands
{
  Stop,
  Forward,
  Backward,
  Right,
  Left,
  ForwardRight,
  ForwardLeft,
  BackwardRight,
  BackwardLeft
};

static uint32_t btUpdatedTime = 0;
static Commands command = Commands::Stop;
static int speedK = 5;

int GetPwmSpeed(int);

constexpr int MaxPwm = 65535;
static int GoPwmLeft = GetPwmSpeed(5);
static int GoPwmRight = GetPwmSpeed(5);

enum class WheelsState
{
  RotationRight,
  RotationLeft,
  Stop,
  Go,
  Back
};

static WheelsState lastState = WheelsState::Stop;

constexpr TickType_t xDelay1Ms PROGMEM = 1 / portTICK_PERIOD_MS;

void setup() 
{
  pinMode(PinWheelLeftUp, PWM);
  pinMode(PinWheelLeftDown, PWM);
  pinMode(PinWheelRightUp, PWM);
  pinMode(PinWheelRightDown, PWM);
  WheelsStop();

  Serial.begin(115200);
  BTSerial.begin(38400);
  delay(500);

  xTaskCreate(vWheelsTask,  "T1", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY + 2, NULL);
  xTaskCreate(vCheckBtTask, "T2", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY + 2, NULL);
  vTaskStartScheduler();
}

static void vWheelsTask(void *pvParameters) 
{
  static bool rotateLeft = false;
  for (;;)
  {
    switch (command)
    {
      case Commands::Stop:
        SetNormalSpeed();
        WheelsStop();
        break;
      case Commands::Forward:
        SetNormalSpeed();
        WheelsGo();
        break;
      case Commands::Backward:
        SetNormalSpeed();
        WheelsBack();
        break;
      case Commands::Right:
        Rotate(false, 70);
        command = Commands::Stop;
        break;
      case Commands::Left:
        Rotate(true, 70);
        command = Commands::Stop;
        break;
      case Commands::ForwardRight:
        AddToRight();
        WheelsGo();
        vTaskDelay(xDelay1Ms * 200);
        break;
      case Commands::ForwardLeft:
        AddToLeft();
        WheelsGo();
        vTaskDelay(xDelay1Ms * 200);
        break;
      case Commands::BackwardRight:
        AddToLeft();
        WheelsBack();
        vTaskDelay(xDelay1Ms * 200);
        break;
      case Commands::BackwardLeft:
        AddToRight();
        WheelsBack();
        vTaskDelay(xDelay1Ms * 200);
        break;
    }
    vTaskDelay(xDelay1Ms);
  }
}

static void vCheckBtTask(void *pvParameters) 
{
  for (;;)
  {
    checkBluetoothCommand();
    vTaskDelay(xDelay1Ms);
  }
}

void loop()
{
}

void checkBluetoothCommand() 
{ 
  if (BTSerial.available())
  { 
    WheelsControl(BTSerial.read());
  }
  else if (millis() - btUpdatedTime > 1000)
  {
    command = Commands::Stop;
  }
} 

void WheelsControl(char val)
{
  btUpdatedTime = millis();
  switch (val)
  {
    case 'S':// Stop
    { 
      if (command != Commands::Stop)
      {
        command = Commands::Stop;
        Serial.println("Stop");
      }
      break;
    } 
    case 'F':// Forward 
    { 
      if (command != Commands::Forward)
      {
        command = Commands::Forward;
        Serial.println("Forward");
      }
      break;
    } 
    case 'B':// Backwards
    {
      if (command != Commands::Backward)
      {
        command = Commands::Backward;
        Serial.println("Backward");
      }
      break;
    }
    case 'R':// Right 
    {
      if (command != Commands::Right)
      {
        command = Commands::Right;
        Serial.println("Right");
      }
      break;
    } 
    case 'L':// Left 
    {
      if (command != Commands::Left)
      {
        command = Commands::Left;
        Serial.println("Left");
      }
      break;
    }
    case 'I':// Forward Right
    {
      if (command != Commands::ForwardRight)
      {
        command = Commands::ForwardRight;
        Serial.println("ForwardRight");
      }
      break;
    }
    case 'G':// Forward Left
    {
      if (command != Commands::ForwardLeft)
      {
        command = Commands::ForwardLeft;
        Serial.println("ForwardLeft");
      }
      break;
    }
    case 'H':// Back Left
    {
      if (command != Commands::BackwardLeft)
      {
        command = Commands::BackwardLeft;
        Serial.println("BackwardLeft");
      }
      break;
    }
    case 'J':// Back Right
    {
      if (command != Commands::BackwardRight)
      {
        command = Commands::BackwardRight;
        Serial.println("BackwardRight");
      }
      break;
    }
    case '0'://set speed 
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
    case '8':
    case '9':
    case 'q':
    {
      speedK = val - 0x30;
      if (val == 'q')
        speedK = 10;
      Serial.println("Speed set to: " + speedK);
      GoPwmLeft = GetPwmSpeed(speedK);
      GoPwmRight = GetPwmSpeed(speedK);
      break;
    }
  } 
}

int GetPwmSpeed(int speedK)
{
  return MaxPwm - MaxPwm * speedK / 10;
}

void SetNormalSpeed()
{
  GoPwmRight = GetPwmSpeed(speedK);
  GoPwmLeft = GetPwmSpeed(speedK);
}

void AddToLeft()
{
  GoPwmRight = GetPwmSpeed(speedK + 3);
  GoPwmLeft = GetPwmSpeed(speedK - 3);
  if (GoPwmRight > MaxPwm)
    GoPwmRight = MaxPwm;
  if (GoPwmLeft < 0)
    GoPwmLeft = 0;
}

void AddToRight()
{
  GoPwmRight = GetPwmSpeed(speedK - 3);
  GoPwmLeft = GetPwmSpeed(speedK + 3);
  if (GoPwmLeft > MaxPwm)
    GoPwmLeft = MaxPwm;
  if (GoPwmRight < 0)
    GoPwmRight = 0;
}

void WheelsStop()
{
  if (lastState != WheelsState::Stop)
  {
    lastState = WheelsState::Stop;
    Serial.println("WheelsStop");
  }
  WheelLeftUpOff();
  WheelLeftDownOff();
  WheelRightUpOff();
  WheelRightDownOff();
}

void Rotate(bool rotateLeft, TickType_t ms)
{
  WheelsStop();
  if (rotateLeft)
  {
    WheelsRotationLeft();
  }
  else
  {
    WheelsRotationRight();
  } 
  vTaskDelay(xDelay1Ms * ms);
  WheelsStop();
}

void WheelsRotationLeft()
{
  if (lastState != WheelsState::RotationLeft)
  {
    lastState = WheelsState::RotationLeft;
    Serial.println("WheelsRotationLeft");
  }
  WheelLeftUpOff();
  WheelLeftDownOn();
  WheelRightUpOn();
  WheelRightDownOff();
}

void WheelsRotationRight()
{
  if (lastState != WheelsState::RotationRight)
  {
    lastState = WheelsState::RotationRight;
    Serial.println("WheelsRotationRight");
  }
  WheelLeftUpOn();
  WheelLeftDownOff();
  WheelRightUpOff();
  WheelRightDownOn();
}

void WheelsGo()
{
  if (lastState != WheelsState::Go)
  {
    lastState = WheelsState::Go;
    Serial.println("WheelsGo");
  }
  WheelLeftUpOn();
  WheelLeftDownOff();
  WheelRightUpOn();
  WheelRightDownOff();
}

void WheelsBack()
{
  if (lastState != WheelsState::Back)
  {
    lastState = WheelsState::Back;
    Serial.println("WheelsBack");
  }
  WheelLeftUpOff();
  WheelLeftDownOn();
  WheelRightUpOff();
  WheelRightDownOn();
}

void WheelLeftUpOn()
{
  pwmWrite(PinWheelLeftUp, GoPwmLeft);
}

void WheelLeftUpOff()
{
  pwmWrite(PinWheelLeftUp, MaxPwm);
}

void WheelLeftDownOn()
{
  pwmWrite(PinWheelLeftDown, GoPwmLeft);
}

void WheelLeftDownOff()
{
  pwmWrite(PinWheelLeftDown, MaxPwm);
}

void WheelRightUpOn()
{
  pwmWrite(PinWheelRightUp, GoPwmRight);
}

void WheelRightUpOff()
{
  pwmWrite(PinWheelRightUp, MaxPwm);
}

void WheelRightDownOn()
{
  pwmWrite(PinWheelRightDown, GoPwmRight);
}

void WheelRightDownOff()
{
  pwmWrite(PinWheelRightDown, MaxPwm);
}

