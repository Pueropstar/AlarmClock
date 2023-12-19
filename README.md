# AlarmClock

**현재시간과 온도를 보여주고, 알람 기능을 탑재한 시계**
+ 임베디드 시스템 기말 프로젝트 - 2분반 5팀

# 아이디어 소개

- 사용자가 설정한 시간에 맞춰 알람이 울리는 알람 시계를 제작 하였다.
- 현재 시간을 보여줄 수 있고, 실내 온도 또한 보여줄 수 있다.
- 핸드폰을 사용하여 알람 설정, 알람 해제, 알람 종료가 가능하다.
- 스위치를 사용하여 알람을 종료할 수 있으며, 시계 모드와 온도 모드 전환도 가능하다.

# 기능 상세
**1. BlueTooth를 사용한 핸드폰과 라즈베리 파이 사이의 통신**
- 블루투스를 사용하여 핸드폰과 라즈베리파이를 연결하고 UART 통신을 사용하여 특정 명령어를 통해 알람이 울릴 시간, 알람 취소, 알람 종료 등이 가능하다.

**2. 온도 표시**
- 온습도 센서인 dht11을 사용하였고, 온도를 지속적으로 측정하여 4-digit 7-segment에 표시한다.

**3. 시간 표시**
- 현재 시간을 24시간 형식으로 4-digit 7-segment에 표시한다.

**4. 알람 재생**
- 핸드폰에서 설정한 시간이 되면, 코드 내부에 있는 주파수에 맞게 수동 부저에서 음이 재성되며 알람이 울리게 된다.

**5. 스위치**
- 스위치는 총 3개로 각각 알람 종료, 시계 모드, 온도 모드 역할을 하며, 스위치를 누를때 마다, 플래그를 제어하여 동작하게 된다.

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

1. 본래 스위치 하나만 사용해서 토글 형식으로 모드 변경을 하려 했으나, 스위치 입력이 한번에 많이 감지되어 실패<br/>→ **스위치 두 개를 사용하여 시계 모드, 온도 모드 따로 구현**
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
|20190400|류현식|블루투스 통신, 알람 기능 예외처리 및 멀티 쓰레드 기능 개발 |
|20190402|마재성|온습도 센서 데이터 처리, 스위치 기능, 멀티 쓰레드 기능 개발 |
|20190531|박현빈|블루투스 통신, 알람 기능 개발 |
|20190568|백재민|4-digit 을 통한 시간 및 온도 표시, 멀티 쓰레드 기능 개발 |

# 전체 시스템 구조
![image](https://github.com/Pueropstar/AlarmClock/assets/109838831/30893ac0-7383-4c39-9429-552852ef9625)

<br/><br/><br/><br/>

![image](https://github.com/Pueropstar/AlarmClock/assets/109838831/e5af40f8-d816-465d-bf13-048a164c9cca)



# 개발 일정 (12.4 ~ 12.15)

![image](https://github.com/Pueropstar/AlarmClock/assets/109838831/f30f13df-16e4-4a39-8328-5dfedb5148e1)


# 제한 사항 구현 내용 (멀티프로세스/쓰레드, IPC/뮤텍스)

### 멀티 쓰레드 관련 구현 내용

- 다양한 기능을 병렬로 처리하기 위해 여러 쓰레드를 사용하였다. 

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

- 공유 자원에 대한 동시 접근을 관리하기 위해 뮤텍스를 사용하였다.

```c
// ...
int isRinging = 0; // 0일때 알람 울리고 있지 않음, 1일때 알람 울리는중
int currentShow = 0; // 0일때 시간, 1일때 온도
int alarmActive = 0; // 0일때 알람 설정 안됨, 1일때 설정됨
// ...
```

프로그램에서 사용하는 공유 변수들이다

```c
// ...
    if(strcmp(buffer, currentTime) > 0) //현재 시간보다 더 이후인지
    {
      pthread_mutex_lock(&lock);
      strcpy(alarmTime, buffer);  // 알람 시간 저장
      alarmActive = 1;  // 알람 활성화
      pthread_mutex_unlock(&lock);
      printFormattedTime(buffer);
      printf("%s 알람을 취소하고 싶으시면 취소\n", alarmTime);
    }
// ...
```
블루투스의 입력 처리를 하는 inputFunc의 일부

```c
        if (alarmActive && strcmp(currentTime, alarmTime) == 0)
        {
            pthread_mutex_lock(&lock);
            isRinging = 1;
            pthread_mutex_unlock(&lock);
            // ...
        }
```
알람을 울리는 함수인 alarmFunc의 일부


```c
        if(detectionAlarmOff==LOW){
            
            if(isRinging){
                pthread_mutex_lock(&lock);
                isRinging = 0;
                alarmActive =0;
                pthread_mutex_unlock(&lock);
            }
            else{
                printf("울리고 있는 알림이 없습니다! \n");
            }
        }
```

스위치 처리를 하는 btnFunc의 일부

**위의 alarmActive와 isRinging은 다른 쓰레드에서도 사용하기 때문에 충돌을 방지하기 위하여 뮤텍스 lock을 걸고 처리한다.**

# 데모 영상

+ 알람 시연
  [![image](https://github.com/Pueropstar/AlarmClock/assets/109838831/5f82877f-d958-492c-af86-5e9db7b73426)](https://youtu.be/-FSf1KY0SdY)
+ 모드 시연
  [![image](https://github.com/Pueropstar/AlarmClock/assets/109838831/19baac2c-b0a6-432a-ae2c-f92b9c11f580)](https://youtu.be/bM6fxsMUYrk)






