#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <wiringPi.h>
#include <wiringSerial.h>
#include <signal.h>
#include <time.h>
#include <stdint.h>
#include <pthread.h>
#include <math.h>
#include <unistd.h>

#define BAUD_RATE 115200
#define UART2_DEV "/dev/ttyAMA1"
#define BUFFER_SIZE 1024
#define MAXTIMINGS    85
#define DHTPIN        4  //wPi pin. physical num 7

#define D4 293.70
#define G4 392.00
#define As5 932.30
#define D5 587.30
#define G5 780.00
#define C5 523.30
#define A5 880.00
#define A6 1760.00
#define F5 698.50
#define Ds5 622.30
#define E5 659.30
#define B5 987.80
#define Cs5 544.40
#define Fs5 740.00
#define As6 1864.70
#define C4 261.60
#define As4 446.20
#define A4 440.00
#define B4 493.90
#define Cs4 277.20
#define Fs4 370.00
#define R 0
#define TOTAL_NOTES (sizeof(notes) / sizeof(notes[0]))
/// 위에 음악 코드
#define SEG_A 19
#define SEG_B 10
#define SEG_C 25 
#define SEG_D 20
#define SEG_E 21
#define SEG_F 6
#define SEG_G 24
#define SEG_DP 16

#define NUMS 12
#define SEG_COUNT 8 

#define DIGIT_1 26
#define DIGIT_2 5
#define DIGIT_3 9
#define DIGIT_4 23


int notes[] = {
    D4, G4, As5, D5, R, D5, C5, As5, A5, As5, R, G4, As5, D5, G5, R, G5, R, G5, A6, F5, Ds5, F5, R,
    As5, D5, F5, A6, G5, F5, E5, F5, G5, R, F5, E5, D5, C5, As5, C5, D5, C5, G4, A5, R,
    As5, A5, C5, As5, A5, R, A4, D4, R, D4, R, A4, D4, R, D4, R, A4, D4, R, D4, R,
    D4, G4, As5, D5, R, D5, C5, As5, A5, As5, R, G4, As5, D5, G5, R, G5, R, G5, A6, F5, Ds5, F5, R,
    As5, D5, F5, A6, G5, F5, E5, F5, G5, R, F5, E5, D5, C5, As5, C5, D5, C5, G4, A5, R};


int durations[TOTAL_NOTES] = {
    500, 500, 500, 700, 400, 500, 500, 500, 500, 900, 400, 500, 500, 500, 700, 400,
    400, 400, 300, 300, 1000, 400, 500, 500, 500, 1000, 500, 500, 300, 300, 300, 300,
    1000, 500, 300, 300, 300, 300, 500, 1000, 400, 500, 500, 500, 500, 700, 400, 500,
    500, 500, 500, 700, 400, 400, 400, 300, 300, 1000, 400, 500, 500, 500, 1000, 500,
    500, 300, 300, 300, 300, 1000, 500, 300, 300, 300, 300, 500, 1000, 400, 500, 300,
    300, 300, 300, 500, 1000, 400, 300, 300, 300, 300, 500, 1000, 400, 300, 300, 500};


int numbers[NUMS][SEG_COUNT] = {
    {1, 1, 1, 1, 1, 1, 0, 0}, // 0
    {0, 1, 1, 0, 0, 0, 0, 0}, // 1
    {1, 1, 0, 1, 1, 0, 1, 0}, // 2
    {1, 1, 1, 1, 0, 0, 1, 0}, // 3
    {0, 1, 1, 0, 0, 1, 1, 0}, // 4
    {1, 0, 1, 1, 0, 1, 1, 0}, // 5
    {1, 0, 1, 1, 1, 1, 1, 0}, // 6
    {1, 1, 1, 0, 0, 0, 0, 0}, // 7
    {1, 1, 1, 1, 1, 1, 1, 0}, // 8
    {1, 1, 1, 1, 0, 1, 1, 0}, //9
    {1, 0, 0, 1, 1, 1, 0, 0},   //C
    {0, 0, 0, 0, 0, 0, 0, 1}   //.


};
//온습도 0,1 습도, 온도 

int dht11_dat[5] = { 0, 0, 0, 0, 0 };

int gpio = 18;

int clockModeSwitch = 17;
int tempModeSwitch = 22;
int alarmOffSwitch = 27;
int saveInt = 24;
int saveDecimal =5;

int isRinging = 0; // 0일때 알람 울리고 있지 않음, 1일때 알람 울리는중
int currentShow = 0; // 0일때 시간, 1일때 온도
int alarmActive = 0; // 0일때 알람 설정 안됨, 1일때 설정됨

pthread_mutex_t lock;
char alarmTime[6];

// GPIO 핀 번호 설정
int segments[] = {SEG_A, SEG_B, SEG_C, SEG_D, SEG_E, SEG_F, SEG_G, SEG_DP};
int digits[] = {DIGIT_1, DIGIT_2, DIGIT_3, DIGIT_4};

void initMyTone(int gpio, int freq);
void myTone(int gpio, int freq);
unsigned char serialRead(const int fd);

void *inputFunc(void*); //블루투스 입력 처리(알람설정,취소,해제) 함수
void *displayTimeFunc(void*); //시간 출력 함수 
void *displayTempFunc(void*); //온도 출력 함수
void *alarmFunc(void*); //알람 함수
void *btnFunc(void*); //스위치 처리 함수
void *readTempFunc(void*); // 온도 읽기 함수


unsigned char serialRead(const int fd)
{
    unsigned char x;
    if (read(fd, &x, 1) != 1)
        return -1;
    return x; // 읽어온 데이터 반환
}

void initMyTone(int gpio, int freq)
{
    pinMode(gpio, PWM_OUTPUT);
    pwmSetMode(PWM_MODE_MS);
    pwmSetRange(100);
    int divisor = 19200000 / (freq * 100);
    pwmSetClock(divisor);
    pwmWrite(gpio, 0);
}

void myTone(int gpio, int freq)
{
    if (freq == R)
    {
        pwmWrite(gpio, 0);
    }
    else
    {
        int divisor = 19200000 / (freq * 100);
        pwmSetClock(divisor);
        pwmWrite(gpio, 50);
    }
    delay(250);
    pwmWrite(gpio, 0);
    delay(100);
}

void *alarmFunc(void *arg)
{
    char currentTime[6];
    time_t t;
    struct tm *tmp;
   
    
    while (1)
    {
        pthread_mutex_lock(&lock);
        t = time(NULL);
        tmp = localtime(&t);
        if (tmp == NULL)
        {
            perror("localtime");
            pthread_mutex_unlock(&lock);
            continue;
        }
        strftime(currentTime, sizeof(currentTime), "%H:%M", tmp);
        pthread_mutex_unlock(&lock);
       
        if (alarmActive && strcmp(currentTime, alarmTime) == 0)
        {
            pthread_mutex_lock(&lock);
            isRinging = 1;
            pthread_mutex_unlock(&lock);

            printf("%s에 설정된 알람이 울립니다!\n알람을 끄고 싶다면 stop이라고 보내주세요!\n", alarmTime);

            while(isRinging)
            {
                for (int i = 0; i < TOTAL_NOTES; i++)
                {
                    myTone(gpio, notes[i]);
                    
                    
                    if (! isRinging)
                    {
                        break; // 알람 비활성화 시, 루프 탈출
                    }
                }
            }
        }
        sleep(1);
    }
    return NULL;
}



void displayNumber(int num) {
    for (int i = 0; i < 8; ++i) {
        if (num==11)
        {
            digitalWrite(segments[7],numbers[num][i]);
        }
        else{

            digitalWrite(segments[i], numbers[num][i]);
        }
    }
}



void displayTemperature(int temperatureInt, int tempratureDe)
{

    // 정수 부분을 표시
    displayNumber(temperatureInt / 10);
    digitalWrite(digits[0], LOW);
    delay(5);
    digitalWrite(digits[0], HIGH);

    displayNumber(temperatureInt % 10);
    digitalWrite(digits[1], LOW);
    delay(5);
    digitalWrite(digits[1], HIGH);

    displayNumber(11);
    digitalWrite(digits[1], LOW);
    delay(5);
    digitalWrite(digits[1], HIGH);



    // 소수 부분을 표시 (소수점 뒤 첫째 자리만 표시)
  
    // printf("%d\n", tempratureDe);
    displayNumber(tempratureDe);
    digitalWrite(digits[2], LOW);
    delay(5);
    digitalWrite(digits[2], HIGH);

    displayNumber(10);
    digitalWrite(digits[3], LOW);
    delay(5);
    digitalWrite(digits[3], HIGH);

}


void *displayTempFunc(void *arg){

    
       
    
        while (1) {

            if(currentShow == 1){
        

                if (dht11_dat[2]!=0 && dht11_dat[3] != 0)
                {
                    displayTemperature(dht11_dat[2],dht11_dat[3]);
                }
                else{
                    displayTemperature(saveInt,saveDecimal);
                }
            }
        }

}

void *readTempFunc(void *arg)
{
    while(1)
    {
    uint8_t laststate    = HIGH;
    uint8_t counter        = 0;
    uint8_t j        = 0, i;
    float    f; 
    pthread_mutex_lock(&lock);
    dht11_dat[0] = dht11_dat[1] = dht11_dat[2] = dht11_dat[3] = dht11_dat[4] = 0;
    pthread_mutex_unlock(&lock);

    pinMode( DHTPIN, OUTPUT );
    digitalWrite( DHTPIN, LOW );
    delay( 18 );
    digitalWrite( DHTPIN, HIGH );
    delayMicroseconds( 40 );
    pinMode( DHTPIN, INPUT );
 
    for ( i = 0; i < MAXTIMINGS; i++ )
    {
        counter = 0;
        while ( digitalRead( DHTPIN ) == laststate )
        {
            counter++;
            delayMicroseconds( 1 );
            if ( counter == 255 )
            {
                break;
            }
        }
        laststate = digitalRead( DHTPIN );
 
        if ( counter == 255 )
            break;
 
        if ( (i >= 4) && (i % 2 == 0) )
        {
            pthread_mutex_lock(&lock);
            dht11_dat[j / 8] <<= 1;
            if ( counter > 16 )
                dht11_dat[j / 8] |= 1;
            j++;
            pthread_mutex_unlock(&lock);
        }
    }
    
    // 2번째 3번째 인덱스 온도에서 정수(2번째 인덱스).소수(3번째 인덱스)

    if(dht11_dat[2] !=0 && dht11_dat[3] !=0) // 0도가 아니라면
    {
        
        saveInt = dht11_dat[2];
        saveDecimal =dht11_dat[3];
        
    }

    //데이터를 읽지를 못하거나 0도를 출력을해버릴때 읽지 못하는건 그럴수있음 0도를 출력하는건 좀 아니다
    

    if ((j >= 40) && (dht11_dat[4] == ((dht11_dat[0] + dht11_dat[1] + dht11_dat[2] + dht11_dat[3]) & 0xFF)))
    {
        
        f = dht11_dat[2] * 9. / 5. + 32;
        printf("Humidity = %d.%d %% Temperature = %d.%d C (%.1f F)\n",
               dht11_dat[0], dht11_dat[1], dht11_dat[2], dht11_dat[3], f);

    }
    else
    {
        printf("Data not good, skip\n");
        
    }
    sleep(5);
    }
    
}


void setup() {
    wiringPiSetupGpio();
    
    // GPIO 핀 초기화
    for (int i = 0; i < 8; ++i) {
        pinMode(segments[i], OUTPUT);
        digitalWrite(segments[i], LOW);
    }

    for (int i = 0; i < 4; ++i) {
        pinMode(digits[i], OUTPUT);
        digitalWrite(digits[i], HIGH);
    }
}

void ClearPinMap() {
    printf("\ngood-bye\n");

    for (int b = 0; b < SEG_COUNT; ++b) {
        digitalWrite(segments[b], LOW);
    }

    for (int b = 0; b < 4; ++b) {
        digitalWrite(digits[b], LOW);
    }

    exit(0);
}




void displayTime(struct tm *t) {
    int hour = t->tm_hour;
    int minute = t->tm_min;

    // Display hour (two digits)
    displayNumber(hour / 10);
    digitalWrite(digits[0], LOW);
    delay(5);
    digitalWrite(digits[0], HIGH);

    displayNumber(hour % 10);
    digitalWrite(digits[1], LOW);
    delay(5);
    digitalWrite(digits[1], HIGH);

    // Display minute (two digits)
    displayNumber(minute / 10);
    digitalWrite(digits[2], LOW);
    delay(5);
    digitalWrite(digits[2], HIGH);

    displayNumber(minute % 10);
    digitalWrite(digits[3], LOW);
    delay(5);
    digitalWrite(digits[3], HIGH);
}


void printFormattedTime(const char *timeStr)
{
    int hour, minute;
    sscanf(timeStr, "%2d:%2d", &hour, &minute);
    printf("%d시 %d분에 알람이 설정되었어요!\n", hour, minute);
}

int isValidTimeFormat(const char *str)
{
    if (strlen(str) != 5)
        return 0;

    if (!isdigit(str[0]) || !isdigit(str[1]) || str[2] != ':' || !isdigit(str[3]) || !isdigit(str[4]))
        return 0;

    int hour = (str[0] - '0') * 10 + (str[1] - '0');
    int minute = (str[3] - '0') * 10 + (str[4] - '0');

    if (hour < 0 || hour > 23 || minute < 0 || minute > 59)
        return 0;

    return 1;
}
void *inputFunc(void *arg)
{
    int fd_serial = *(int *)arg;
    char buffer[BUFFER_SIZE] = {0};
    int bufferIndex = 0;
    char currentTime[6];
    time_t t;
    struct tm *tmp;
   
    while (1)
    {
        if (serialDataAvail(fd_serial))
        {
            char c = serialGetchar(fd_serial);

            if (c == '\n' || c == '\r')
            {
                
                // 현재 시간 업데이트
                t = time(NULL);
                tmp = localtime(&t);
                strftime(currentTime, sizeof(currentTime), "%H:%M", tmp);
               
                if (bufferIndex > 0)  // 버퍼에 데이터가 있으면
                {
                    if (strcmp(buffer, "stop") == 0)
                    {
                        pthread_mutex_lock(&lock);
                        alarmActive = 0;  // 알람 비활성화
                        isRinging = 0;
                        pthread_mutex_unlock(&lock);
                        printf("%s 입력 확인 알람이 중지되었습니다.\n",buffer);
                    }
                    else if (isValidTimeFormat(buffer))
                    {  
                        if(strcmp(buffer, currentTime) > 0) //현재 시간보다 더 이후인지
                        {
                            pthread_mutex_lock(&lock);
                            strcpy(alarmTime, buffer);  // 알람 시간 저장
                            alarmActive = 1;  // 알람 활성화
                            pthread_mutex_unlock(&lock);
                            printFormattedTime(buffer);
                            printf("%s 알람을 취소하고 싶으시면 취소\n", alarmTime);
                        }
                        else
                        {
                            printf("%s 보다 더 이후 시간으로 맞춰주세요. \n",currentTime);
                        }
                    }
                    else if (strcmp(buffer, "취소") == 0)
                    {
                        if(alarmActive == 1 && isRinging==0)
                        {
                            pthread_mutex_lock(&lock);
                            alarmActive = 0;
                            pthread_mutex_unlock(&lock);
                            printf("%s 알람이 취소 되었습니다\n",alarmTime);
                        }
                        else if(alarmActive ==1 && isRinging==1 ){
                            printf("울리고 있는 알람은 stop 명령어를 통해 멈춰주세요!\n");
                        }
                        else
                        {
                            printf("알람을 먼저 설정해주세요\n");
                        }
                    }
                    else
                    {
                        printf("%s는 알람 형식에 맞지 않습니다! $$:$$ 형식으로 보내주세요!\n", buffer);  // 기타 문자열 출력
                    }
                    memset(buffer, 0, sizeof(buffer));  // 버퍼 초기화
                    bufferIndex = 0;  // 인덱스 초기화
                }
                    
            }
            else if (bufferIndex < BUFFER_SIZE - 1)
            {
                buffer[bufferIndex++] = c;  // 버퍼에 문자 추가
                buffer[bufferIndex] = '\0';  // 널 종료 문자 추가
            }
        }
        usleep(100000);  // 0.1초 대기
    }

    return NULL;
}


void *displayTimeFunc(void *arg) {

   
    char currentTime[6];
    while (1) {
        if (currentShow == 0)
        {
        char digitChar ;
        

        
        time_t timer = time(NULL);
        struct tm *t = localtime(&timer);
        pthread_mutex_lock(&lock);
        strftime(currentTime, sizeof(currentTime), "%H:%M", t);
        pthread_mutex_unlock(&lock);

        if (strcmp(alarmTime,currentTime) == 0 && alarmActive ==1) {
            
            
            while(isRinging){

            
            for (int i = 0; i < 250; ++i) {
                for (int digit = 0; digit < 4; ++digit) {

                    pthread_mutex_lock(&lock);
                    digitalWrite(digits[digit], LOW);
                    pthread_mutex_unlock(&lock);

                    if(digit>=2)
                    {
                       digitChar = alarmTime[digit+1];
                    }
                    else{

                        digitChar = alarmTime[digit];
                    }
                   
                    int num = digitChar - '0';
                    displayNumber(num);
                    
                    delay(1); 
                    pthread_mutex_lock(&lock);
                    digitalWrite(digits[digit], HIGH);
                    pthread_mutex_unlock(&lock);
                }
                
            }
            sleep(1);
            }
        } else {
            displayTime(t);
        }
        }

        // 기다림 (필요에 따라 조절)
        
    }
}

void *btnFunc (void *arg){
    pinMode (alarmOffSwitch,INPUT);
    pinMode (clockModeSwitch,INPUT);
    pinMode (tempModeSwitch,INPUT);
    int detectionAlarmOff;
    int detectionClockSwitch;
    int detectionTempSwitch;
    
    while(1){
        detectionAlarmOff = digitalRead(alarmOffSwitch);
        detectionClockSwitch = digitalRead(clockModeSwitch);
        detectionTempSwitch = digitalRead(tempModeSwitch);

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
        if (detectionClockSwitch==LOW){
            pthread_mutex_lock(&lock);
            currentShow=0;
            pthread_mutex_unlock(&lock);
        }
        else if(detectionTempSwitch==LOW && isRinging == 0)
        {
            pthread_mutex_lock(&lock);
            currentShow=1;
            pthread_mutex_unlock(&lock);
        }
        else if (detectionTempSwitch ==LOW && isRinging == 1){
            printf("알림이 실행중일 땐 모드를 바꿀 수 없습니다. 알림을 먼저 취소해주세요\n");
        }
    }
        
}

int main() {
    

    setup();
    signal(SIGINT, ClearPinMap);
    int fd_serial;
    pthread_t inputThreadID, displayTimeThreadID, readTempID, displayTempID , alarmThreadID, btnThreadID;

    if (wiringPiSetup() < 0 || wiringPiSetupGpio() == -1)
        return 1;
       
    if ((fd_serial = serialOpen(UART2_DEV, BAUD_RATE)) < 0)
    {
        printf("Unable to open serial device.\n");
        return 1;
    }

    initMyTone(gpio, C4);

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
    return 0;

}