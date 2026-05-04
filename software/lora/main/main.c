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

// Note : Configuration avec idf.py menuconfig
// TTGO T-Beam V1.1 : CS GPIO 18, RST GPIO 23, SCK GPIO 5, MISO GPIO 19, MOSI GPIO 27

#define TRANSCIVER_MODE 0 // 0: Mode RX, 1: Mode TX

uint8_t buf[32]; // Buffer pour les données reçues

void task_tx(void *p)
{
    for (;;)
    {
        vTaskDelay(pdMS_TO_TICKS(5000));
        lora_send_packet((uint8_t *)"Hello", 5);
        printf("packet sent...\n");
    }
}

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

    // Paramètres de configuration du module LoRa
    lora_set_tx_power(10);        // Puissance d'émission (dBm)
    lora_set_frequency(433e6);    // Fréquence (433 MHz)
    lora_set_spreading_factor(7); // Facteur de propagation (SF7)
    lora_set_bandwidth(250e3);    // Bande passante (250 kHz)
    lora_set_coding_rate(5);      // Taux de codage (4/x)
    lora_enable_crc();            // CRC activé (Cyclic Redundancy Check) : le récepteur rejette silencieusement les trames corrompues

    if (TRANSCIVER_MODE == 0)
    { // Mode émetteur
        // Tâche FreeRTOS qui s'exécutera en parallèle du reste du programme
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
