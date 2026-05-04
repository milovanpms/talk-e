/*
 *
 */

#include <stdio.h>
#include <string.h>
#include "sdkconfig.h"
#include "cmd_i2ctools.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "lora.h"

#define TRANSCIVER_MODE 0 // 0: Mode RX, 1: Mode TX

void task_tx(void *p)
{
    for (;;)
    {
        vTaskDelay(pdMS_TO_TICKS(5000));
        lora_send_packet((uint8_t *)"Hello", 5);
        printf("packet sent...\n");
    }
}

uint8_t buf[32]; // Buffer pour les données reçues

void task_rx(void *p)
{
    int x;
    for (;;)
    {
        lora_receive();
        while (lora_received())
        {
            x = lora_receive_packet(buf, sizeof(buf));
            buf[x] = 0;
            printf("Received: %s\n", buf);
            lora_receive();
        }
        vTaskDelay(1);
    }
}

void app_main(void)
{
    lora_init();
    lora_set_frequency(433e6); // Fréquence
    lora_enable_crc();         // CRC activé (Cyclic Redundancy Check) : le récepteur rejette silencieusement les trames corrompues
    
    if (TRANSCIVER_MODE == 0)
    { // Mode émetteur
        xTaskCreate(
            &task_tx,  // Pointeur vers la fonction de la tâche à exécuter
            "task_tx", // Nom de la tâche
            2048,      // Taille de la pile allouée à cette tâche (octets)
            NULL,      // Paramètre à passer à la tâche
            5,         // Priorité de la tâche (plus le nombre est élevé, plus la tâche est prioritaire)
            NULL       // Pointeur pour récupérer le handle de la tâche
        );
    }
    else
    { // Mode récepteur
        xTaskCreate(&task_rx, "task_rx", 2048, NULL, 5, NULL);
    }
}
