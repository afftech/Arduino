#include <avr/io.h>
#include <avr/interrupt.h>
#include "button.h"
#include "buttonAn.h"
bool error = false;
bool fire_on = false;
volatile byte seconds;
volatile byte seconds1;
volatile bool Motor1Pa_d = false;
volatile bool Motor2Pa_d = false;

/// отсрочка старта
volatile byte Start_Timer;
volatile byte start = false;
bool checkT = false;

bool value_pa1;
bool value_pa2;


int sos = 0;
int Motor1 = 5;
bool motor1_on = false;
bool motor2_on = false;
int Motor2 = 6;
int Motor1Pa = 2;
int Motor2Pa = 3;
int Pa_after = 4;   // input разрывает цепь если давление завышено
int Pa_before = 7;  // input разрывает цепь если давление занижено
//////////////////////////////////
// input режим работы
/*button btn_mode1(8);
button btn_mode2(9);*/
int btn_mode1 = 8;
int btn_mode2 = 9;

int mode = 0;
int led1_error = 1;
int led1_mode = 10;
int led2_fire = 11;


//  запуск мотора 1 или 2
button btn_start1(12);
button btn_start2(13);
//  остановка мотора
buttonAn btn_stop1(0);
buttonAn btn_stop2(1);

// проверка давления воды подоваемой до и после мотора
bool value_Pa_before = false;
bool value_Pa_after = false;

uint32_t Timer1, Timer2, Timer3;



void setup() {
  pinMode(sos, INPUT_PULLUP);
  pinMode(Motor1Pa, INPUT_PULLUP);
  pinMode(Motor2Pa, INPUT_PULLUP);
  pinMode(Pa_after, INPUT_PULLUP);
  pinMode(Pa_before, INPUT_PULLUP);
  // кнопка режима работы
  pinMode(btn_mode1, INPUT_PULLUP);
  pinMode(btn_mode2, INPUT_PULLUP);

  Serial.begin(9600);
  pinMode(Motor1, OUTPUT);
  pinMode(Motor2, OUTPUT);
  pinMode(led1_mode, OUTPUT);   //лампы режима работы
  pinMode(led2_fire, OUTPUT);   //пожар
  pinMode(led1_error, OUTPUT);  //лампа ошибки
  // инициализация Timer1
  cli();       // отключить глобальные прерывания
  TCCR1A = 0;  // установить регистры в 0
  TCCR1B = 0;

  OCR1A = 15624;  // установка регистра совпадения

  TCCR1B |= (1 << WGM12);  // включить CTC режим
  TCCR1B |= (1 << CS10);   // Установить биты на коэффициент деления 1024
  TCCR1B |= (1 << CS12);

  TIMSK1 |= (1 << OCIE1A);  // включить прерывание по совпадению таймера
  sei();                    // включить глобальные прерывания
}

void loop() {
  bool Data_btn_mode1 = !digitalRead(btn_mode1);
  bool Data_btn_mode2 = !digitalRead(btn_mode2);
  if (Data_btn_mode1) {  // 10 раз в секунду опрос кнопки
    mode = 1;
  }
  if (Data_btn_mode2) {  // 10 раз в секунду опрос кнопки
    mode = 2;
  }
  if (!Data_btn_mode1 && !Data_btn_mode2) {
    mode = 0;
  }
  digitalWrite(Motor1, motor1_on);
  digitalWrite(Motor2, motor2_on);
  digitalWrite(led2_fire, fire_on);   //лампа пожара 
  digitalWrite(led1_error, error);
  //delay(500);
  if (millis() - Timer1 >= 400) {  //2.2 раза в секунду
    Timer1 = millis();
    if (1 == mode) {
      ServiceMode();
      digitalWrite(led1_error, false);  //сброс ошибки
      digitalWrite(led1_mode, true);  //автоматика отключена
      fire_on = false;
    } else if (2 == mode) {
      digitalWrite(led1_mode, false);  //автоматика включена
      Auto();
    } else {
      digitalWrite(led1_mode, true);  //автоматика отключена
      Motor2Pa_d = false;             //отключаем таймер 30 сек задержки на проверку давления в моторе 2
      motor2_on = false;              //останавливаем моторы если был переход из ручного режима или автоматического
      motor1_on = false;
      fire_on = false;   //обнуляем отложенный пуск
      digitalWrite(led1_error, false);  //сброс ошибки
      //таймер 1
      Motor1Pa_d = false;             //отключаем таймер 30 сек задержки на проверку давления в моторе 1
      seconds = 0;
      //таймер 2
      checkT = false;  //управление задержкой 35сек
      Start_Timer = 0;
      start = false;  //обнуляем отложенный пуск
      //таймер 3
      Motor2Pa_d = false;
      seconds1=0;
    }
  }
}
void ServiceMode() {
  if (btn_start2.click()) {
    motor2_on = !motor2_on;
  }
  if (btn_start1.click()) {
    motor1_on = !motor1_on;
  }
  if (btn_stop1.click()) {
    motor1_on = false;
  }
  if (btn_stop2.click()) {
    motor2_on = false;
  }
  Serial.println("ServiceMode");
}
void Auto() {
  bool DataSos = !digitalRead(sos);
  if (DataSos) {
    fire_on = true;
    if (checkT) {
      Serial.println("sos");
      Motor1Pa_d = true;
      MotorActive(true);
    } else {
      start = true;
    }
  } else {
    Serial.println("No sos");
    MotorActive(false);
    motor2_on = false;  //останавливаем моторы если был переход из ручного режима и если нет сигнала
    motor1_on = false;
    start = false;
    fire_on = false;
    value_pa1 = true;
    value_pa2 = true;

  }
}
int MotorActive(bool active) {

  value_Pa_before = !digitalRead(Pa_before);  /// датчик воды до двигателей
  value_Pa_after = !digitalRead(Pa_after);    /// датчик воды после двигателей
  if ((value_Pa_before == true && value_Pa_after == true)) {
    //обработка ошибки давления воды до моторов
    if(value_Pa_before!=true)//если нет воды в системе ошибка
    {
      error =true;
    }
    //запуск насосов если давлеение на входе и на входе в порядке
    if (active && value_pa1) {
      motor1_on = true;
      Serial.println("Engine 1 start");
    } else {
      motor1_on = false;
      error = true;
      Serial.println("Engine 1 Stop");
    }
    if (active && value_pa2 && !value_pa1) {
      motor2_on = true;
      Serial.println("Engine 2 start");
    }

    if (active && !value_pa1 && !value_pa2) {

      Serial.println("The first and second engine did not start.");
    }
  } else {
    //остановка всех насосов если давление на входе упало или нет воды
    motor1_on = false;
    motor2_on = false;
    Serial.println("Fatal error. No water or low pressure!!!");
  }
  if (!active) {
    Serial.println("Stopping the motors");
  }
}
void check1() {
  bool data = digitalRead(Motor1Pa);
  if (data) {
    Motor1Pa_d = false;  //выключаем счетчик
    value_pa1 = false;
    Motor2Pa_d = true;
  }
  Serial.println("Value PA1");
}

void check2() {
  bool data = digitalRead(Motor2Pa);
  if (data) {
    Motor2Pa_d = false;  //выключаем счетчик
    value_pa2 = false;
  }
  Serial.println("Value PA2");
}
void check3() {
  checkT = true;
}

ISR(TIMER1_COMPA_vect) {

  if (Motor1Pa_d == true) {
    seconds++;
    if (seconds == 30) {
      check1();
      seconds = 0;
    }
  }
  if (start == true) {  /// таймер для старта
    Serial.println(Start_Timer);
    Start_Timer++;
    if (Start_Timer == 30) {
      Start_Timer = 0;
      check3();
      start = false;
    }
  }
  if (Motor2Pa_d == true) {
    seconds1++;
    if (seconds1 == 30) {
      check2();
      seconds1 = 0;
    }
  }
}
