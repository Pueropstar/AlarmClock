# AlarmClock

**현재시간과 온도를 보여주고, 알람 기능을 탑재한 시계**


# 실행 방법

> sudo를 사용하지 않을 시 제대로 실행이 되지 않을 수 있으니 주의.


- 컴파일

```
gcc -o thread thread.c -lwiringPi -lpthread -lm
```
- 실행
```
sudo ./thread
```

# 개발 시 문제점 및 해결 방안

1. 본래 스위치 하나만 사용해서 토글 형식으로 모드 변경을 하려 했으나, 스위치 입력이 한번에 많이 감지되어 실패<br/>→ **스위치 두개를 사용 하여 시계 모드, 온도 모드 따로 구현**
2. 온습도 센서인 dht11에서 가끔씩 온도를 제대로 감지하지 못하는 문제<br/> →  **예외 처리를 하여 정상 온도일때 온도를 저장, 제대로 감지하지 못했을 때 저장한 값을 출력**
```c
 if(dht11_dat[2] !=0 && dht11_dat[3] !=0) // 0도가 아니라면
{
        
        saveInt = dht11_dat[2];
        saveDecimal =dht11_dat[3];
        
}
```

```c
 if (dht11_dat[2]!=0 && dht11_dat[3] != 0)
{
  displayTemperature(dht11_dat[2],dht11_dat[3]);
}
else{
  displayTemperature(saveInt,saveDecimal);
}
```
# 역할 분담

|학번|이름|역할|
|:---|:---|:---|
|20190400|류현식|블루투스 통신, 알람 기능 예외처리 및 멀티 쓰레드 기능 |
|20190402|마재성|온습도 센서 데이터 처리, 스위치 기능, 멀티 쓰레드 기능 |
|20190531|박현빈|블루투스 통신, 알람 기능 |
|20190568|백재민|4-digit 을 통한 시간 및 온도 표시, 멀티 쓰레드 기능 |

# 아이디어 소개

- 설정한 시간에 맞춰 알람이 울리는 알람 시계를 제작 하였다.
- 현재 시간을 보여줄 수 있고, 스위치를 사용하여 온도 또한 보여줄 수 있다.
- 스위치를 눌러 알람을 종료할 수 있고, 핸드폰을 사용하여 명령어를 전송하여 종료하는 것 또한 가능하다.

# 전체 시스템 구조
![image](https://github.com/Pueropstar/AlarmClock/assets/109838831/30893ac0-7383-4c39-9429-552852ef9625)

**1. BlueTooth를 사용한 핸드폰과 라즈베리 파이 사이의 통신**
- 블루투스를 사용하여 핸드폰과 라즈베리파이를 연결하고 UART 통신을 사용하여 특정 명령어를 통해 알람이 울릴 시간, 알람 취소, 알람 종료 등이 가능하다.

**2. 온도 표시**
- 온습도 센서인 dht11을 사용하였고, thread를 사용하여 온도를 계속 측정하여 4-digit 7-segment에 표시한다.

**3. 시간 표시**
- 시간 또한 thread를 사용하여 계속 측정하여 4-digit 7-segment에 표시한다.

**4. 알람 재생**
- 핸드폰에서 설정한 시간이 되면, 코드 내부에 있는 주파수에 맞게 수동 부저에서 음이 재성되며 알람이 울리게 된다.

**5. 스위치**
- 스위치는 총 3개로 각각 알람 종료, 시계 모드, 온도 모드 역할을 하며, 스위치를 누를때 마다, 플래그를 제어하여 동작하게 된다.

<br/><br/><br/>

![image](https://github.com/Pueropstar/AlarmClock/assets/109838831/e5af40f8-d816-465d-bf13-048a164c9cca)

실제 회로도는 다음과 같다.

# 개발 일정 (12.4 ~ 12.15)

![image](https://github.com/Pueropstar/AlarmClock/assets/109838831/f30f13df-16e4-4a39-8328-5dfedb5148e1)


# 제한 사한 구현 내용 (멀티프로세스/쓰레드, IPC/뮤텍스)

### 멀티 쓰레드 관련 구현 내용

```C
void *inputFunc(void*); //블루투스 입력 처리(알람설정,취소,해제) 함수
void *displayTimeFunc(void*); //시간 출력 함수 
void *displayTempFunc(void*); //온도 출력 함수
void *alarmFunc(void*); //알람 함수
void *btnFunc(void*); //스위치 처리 함수
void *readTempFunc(void*); // 온도 읽기 함수
```
쓰레드에서 사용할 함수들을 정의

```C
//  ...
    pthread_mutex_init(&lock, NULL);

    pthread_create(&inputThreadID, NULL, inputFunc, &fd_serial);
    pthread_create(&displayTimeThreadID, NULL, displayTimeFunc, NULL);
    pthread_create (&displayTempID, NULL, displayTempFunc, NULL);
    pthread_create(&alarmThreadID, NULL, alarmFunc, NULL);
    pthread_create(&btnThreadID, NULL, btnFunc, NULL);
    pthread_create(&readTempID,NULL, readTempFunc,NULL);

    pthread_join(inputThreadID, NULL);
    pthread_join(displayTimeThreadID, NULL);
    pthread_join(alarmThreadID, NULL);
    pthread_join(btnThreadID, NULL);
    pthread_join(readTempID, NULL);
    pthread_join(displayTempID, NULL);

    pthread_mutex_destroy(&lock);
//  ...
```

### 뮤텍스 관련 구현 내용


