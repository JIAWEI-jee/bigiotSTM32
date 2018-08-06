/**
  ******************************************************************************
  * @file    main.c
  * @author  www.bigiot.net
  * @version V0.1
  * @date    2017-6-9
  * @brief   STM32F103C8T6_ESP01_LED���豸��¼Ϊ������ģʽ��
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include <string.h>
#include "main.h"
#include "delay.h"
#include "usart1.h"
#include "usart2.h"
#include "sd.h"
#include "ff.h"
#include "ld3320.h"
#include "mp3.h"
#include "led.h"
#include "asr.h"
#include "cJSON.h"
#include "millis.h"
#include "bigiot.h"

FATFS fs[_VOLUMES];

unsigned long lastCheckStatusTime = 0;
unsigned long lastCheckOutTime = 0;
unsigned long lastSayTime = 0;
const unsigned long postingInterval = 40000;
const unsigned long statusInterval = 60000;
bigiot_state BGT_STA = OFF_LINE;
/*�����������������޸�Ϊ�Լ����豸ID��APIKEY���ڱ��������û����������豸���ɻ��*/
char *DEVICEID = "458";
char *APIKEY = "9cb787949";

/*��CJSON�������յ�����Ϣ*/
int processMessage(char *msg) {
    cJSON *jsonObj = cJSON_Parse(msg);
    cJSON *method;
    char *m;
    //json�ַ�������ʧ�ܣ�ֱ���˳�
    printf("������գ�%s", msg);
    if(!jsonObj)
    {
        //uart1.USART2_printf("json string wrong!");
        return 0;
    }
    method = cJSON_GetObjectItem(jsonObj, "M");
    m = method->valuestring;
    if(strncmp(m, "WELCOME", 7) == 0)
    {
        BGT_STA = CONNECTED;
        //��ֹ�豸����״̬δ�������ȵǳ�
        checkout(DEVICEID, APIKEY);
        lastCheckOutTime = millis();
    }
    if(strncmp(m, "connected", 9) == 0)
    {
        BGT_STA = CONNECTED;
        checkin(DEVICEID, APIKEY);
    }
    //{"M":"checkout","IP":"xx1","T":"xx2"}\n
    if(strncmp(m, "checkout", 8) == 0)
    {
        BGT_STA = FORCED_OFF_LINE;
    }
    //{"M":"checkinok","ID":"xx1","NAME":"xx2","T":"xx3"}\n
    if(strncmp(m, "checkinok", 9) == 0)
    {
        BGT_STA = CHECKED;
    }
    if(strncmp(m, "checked", 7) == 0)
    {
        BGT_STA = CHECKED;
    }
    //���豸���û���¼�����ͻ�ӭ��Ϣ
    if(strncmp(m, "login", 5) == 0)
    {
        char *from_id = cJSON_GetObjectItem(jsonObj, "ID")->valuestring;
        char new_content[] = "Dear friend, welcome to BIGIOT !";
        say(from_id, new_content);
    }
    //�յ�sayָ�ִ����Ӧ��������������Ӧ�ظ�
    if(strncmp(m, "say", 3) == 0 && millis() - lastSayTime > 10)
    {
        char *content = cJSON_GetObjectItem(jsonObj, "C")->valuestring;
        char *from_id = cJSON_GetObjectItem(jsonObj, "ID")->valuestring;
        lastSayTime = millis();
        if(strncmp(content, "play", 4) == 0)
        {
            char new_content[] = "led played";
            //do something here....
            LED_D3 = 0;
            say(from_id, new_content);
        }
        else if(strncmp(content, "stop", 4) == 0)
        {
            char new_content[] = "led stoped";
            //do something here....
            LED_D3 = 1;
            say(from_id, new_content);
        }
    }
    if(jsonObj)cJSON_Delete(jsonObj);
    return 1;
}


void setup(void)
{
    delay_init();//��ʱ��ʼ��
    USART1_init(115200);
    USART2_init(115200);
    LED_Init(); 	                   //LED��ʼ��
    while(SD_Init())                   //SD����ʼ��
    {
        printf("SD����ʼ������\r\n");  //������ʾSD����ʼ������
        delay_ms(2000);                //��ʱ2s
    }
    f_mount(&fs[0], "0:", 1); 	     //����SD��
    LD3320_Init();	                   //��ʼ��LD3320
    MILLIS_Init();
}

u8 read_print(char *path)
{
    u8 res;
    u32 size;
    char buf[100];
    UINT br;
	FIL file;
	res=f_open (&file,"0:/message.txt", FA_OPEN_ALWAYS|FA_READ|FA_WRITE);			//���ļ�,���������½�.
	res=f_lseek(&file,f_size(&file)); 												//�ƶ���дָ��(�ƶ����ļ��Ľ�β��������)
	f_write (&file, "���Լ���ֲ��FATFS�ļ�ϵͳ,�������������Ķ�д����.\r\n", sizeof("���Լ���ֲ��FATFS�ļ�ϵͳ,�������������Ķ�д����.\r\n")-1, &br);	//д������	
	f_close(&file);		//�ر��ļ�
}

int main(void)
{
    u8 nAsrRes = 0;
    u16 len;
    setup();
    
    printf(" ����1������ϵͳ\r\n ");
    printf(" ����2������Ц��\r\n ");
    printf(" ����3����\r\n ");
    printf(" ����4���ر�\r\n ");
    nAsrStatus = LD_ASR_NONE;		     //��ʼ״̬��û������ASR
    PlayDemoSound_mp3("ϵͳ׼��.mp3");   //�����ļ�
    while(bMp3Play == 0);                //�������ݷ������
    delay_ms(5000);

    while (1)
    {
        if ( BGT_STA == CONNECTED && lastCheckOutTime != 0 && millis() - lastCheckOutTime > 200) {
            checkin(DEVICEID, APIKEY);
            lastCheckOutTime = 0;
        }
        if (millis() - lastCheckStatusTime > statusInterval || (lastCheckStatusTime == 0 && millis() > 5000)) {
            check_status();
            lastCheckStatusTime = millis();
        }

        if(USART2_RX_STA & 0x8000)
        {
            len=USART2_RX_STA&0x3fff;                       //�õ��˴ν��յ������ݳ���
            printf("usart2 �յ����ȣ�%d\r\n",len);
            USART2_RX_BUF[len]='\0';
            //printf("usart2 �յ����ݣ�%s\r\n",USART2_RX_BUF);
            processMessage((char*)USART2_RX_BUF);
            USART2_RX_STA = 0;
        }
        switch(nAsrStatus)
        {
        case LD_ASR_RUNING:
            break;

        case LD_ASR_ERROR:
            break;

        case LD_ASR_NONE:
            nAsrStatus = LD_ASR_RUNING;            //����һ��ASRʶ�����̣�ASR��ʼ����ASR���ӹؼ��������ASR����
            if (RunASR() == 0)
            {
                printf("ASR_ERROR\r\n");
                nAsrStatus = LD_ASR_ERROR;
            }
            break;

        case LD_ASR_FOUNDOK:
            nAsrRes = LD_ReadReg(0xc5);	            //һ��ASRʶ��ɹ�������ȡASRʶ����
            switch(nAsrRes)
            {
            case CODE_CQXT:
                printf("���յ��������ϵͳ\r\n");
                PlayDemoSound_mp3("����.mp3");   //�����ļ�
                while(bMp3Play == 0);            //�������ݷ������
                delay_ms(3000); 				 //��������3s����ʱ4S��ȷ�������������
                NVIC_SystemReset();              //����
                break;
            case CODE_JGXH:
                printf("���յ��������Ц��\r\n");
                PlayDemoSound_mp3("Ц��.mp3");   //�����ļ�
                while(bMp3Play == 0);            //�������ݷ������
                delay_ms(12000); 				 //��������10s����ʱ12S��ȷ�������������
                break;
            case CODE_DK:
                printf("���յ������\r\n");
                PlayDemoSound_mp3("��.mp3");       //�����ļ�
                while(bMp3Play == 0);                //�������ݷ������
                delay_ms(5000);                      //��������4s����ʱ5S��ȷ�������������
                delay_ms(1000);                      //��������ʾ��1s��ʱ�󣬴�LED
                LED_D3 = 0;
                LED_D4 = 0;
                break;
            case CODE_GB:
                printf("���յ�����ر�\r\n");
                PlayDemoSound_mp3("�ر�.mp3");       //�����ļ�
                while(bMp3Play == 0);                //�������ݷ������
                delay_ms(5000);                      //��������4s����ʱ5S��ȷ�������������
                delay_ms(1000);                      //��������ʾ��1s��ʱ�󣬹ر�LED
                LED_D3 = 1;
                LED_D4 = 1;
                break;
            default:
                printf("���ڿ��֮��\r\n");
                break;
            }
            nAsrStatus = LD_ASR_NONE;
            break;

        case LD_ASR_FOUNDZERO:
            printf("δ֪����\r\n");
            nAsrStatus = LD_ASR_NONE;
            break;

        default:
            nAsrStatus = LD_ASR_NONE;
            break;
        }
    }
}
