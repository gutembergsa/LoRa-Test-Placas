

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/event_groups.h"


#include <WiFi.h>
#include <WiFiUdp.h>

#include <NTPClient.h>


#include <LoRa.h>
#include <PubSubClient.h>

WiFiUDP ntpUDP;

SemaphoreHandle_t xSemaphore = NULL;
TaskHandle_t xLoRa;
TaskHandle_t xConnections;

WiFiClient espClient;
PubSubClient client(espClient);

NTPClient ntp(ntpUDP, "b.ntp.br", -3 * 3600, 60000);

unsigned long counter = 0;

#define ss 5
#define rst 14
#define dio0 2


void checkForLoRaPackets(void * pvParameters){
    while (true){
        if( xSemaphoreTake( xSemaphore,  pdMS_TO_TICKS(200)) == pdTRUE ){
            int packetSize = LoRa.parsePacket();
            ntp.update();         
            if (packetSize) {
                printf("pacote recebido tamanho %i bytes\n", packetSize);
                String hora = ntp.getFormattedTime();
                char msg[packetSize + hora.length()];
                char letter = '0';
                for (size_t i = 0; i < packetSize; i++){
                    if (i == 0){
                        letter = LoRa.read();
                    }
                    else{
                        msg[i - 1] = LoRa.read();
                    }
                }

                if (letter == '1'){
                    counter++;
                    hora += '!';
                    hora += LoRa.packetRssi();

                    for (size_t i = 0; i < (hora.length() + 1); i++){
                        if (i == 0){
                            msg[2 + i] = '.';
                        }
                        else{
                            msg[2 + i] = hora[i - 1];
                        }                    
                    }
                    client.publish("temperatura", msg);
                }          
                else if (letter == '2'){
                    String aux = msg;
                    aux += '|';
                    aux += counter;
                    sprintf(msg, "%s", aux);
                    client.publish("ratings", msg);
                }          
            }            
            xSemaphoreGive( xSemaphore );
            vTaskDelay(400 / portTICK_PERIOD_MS);
        }
    }
}


void startMQTT(){

    ntp.begin();  

    client.setServer("tailor.cloudmqtt.com", 18819);   
    client.connect("Esp32Gutem", "fqqjkyka", "JTc0rI7CORXv");
    if (!client.connected()) {
        printf("Erro ao conectar ao broker\n");
        return;
    }    
}

void startWifi(){
    
    WiFi.begin("D-Link_DIR-610", "mafagafo");

    while (WiFi.status() != WL_CONNECTED) {
        vTaskDelay(500 / portTICK_PERIOD_MS);
    }  
    startMQTT();
}

void checkConnectionTask(void * pvParameters){
    while (true){
        if( xSemaphoreTake( xSemaphore,  pdMS_TO_TICKS(200)) == pdTRUE ){
            if (!client.connected()) {
                startMQTT();
            }
            if (WiFi.status() != WL_CONNECTED) {
                startWifi();
            } 
            xSemaphoreGive( xSemaphore );
            vTaskDelay(400 / portTICK_PERIOD_MS);
        }
    }
}

void startLoRa(){
    LoRa.setPins(ss, rst, dio0);

    while (!LoRa.begin(915E6)) {
        vTaskDelay(500 / portTICK_PERIOD_MS);
    }
    LoRa.setSyncWord(0xC4);    

    printf("\nStartup OK\n");        
    
    xTaskCreatePinnedToCore(checkConnectionTask, "task_1", 5000, NULL, 2, &xConnections, 0);    
    xTaskCreatePinnedToCore(checkForLoRaPackets, "task_2", 5000, NULL, 2, &xLoRa, 1);    
}

void setup(){
    xSemaphore = xSemaphoreCreateBinary();

    if( xSemaphore != NULL )
    {
        xSemaphoreGive( xSemaphore );
        startWifi();
        startLoRa();  
    }   
}

void loop(){}