/*
 * SPDX-FileCopyrightText: 2020-2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <inttypes.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"
#include "esp_timer.h"

/**
 * Test d'un bouton monté en pull-down avec condensateur 10nF pour l'antirebond matériel.
 *
 * Montage :
 *   - Une patte du bouton connectée à VCC (3.3V)
 *   - L'autre patte connectée à GPIO_BUTTON_IO et à une résistance pull-down 10k vers GND
 *   - Condensateur 10nF entre GPIO_BUTTON_IO et GND (antirebond matériel)
 *
 * Logique :
 *   - Repos : GPIO = 0 (pull-down)
 *   - Appui : GPIO = 1 (VCC)
 *   - Interruption sur front montant
 *
 * Antirebond :
 *   - Le 10nF filtre les rebonds hardware (RC avec la résistance pull-down)
 *   - Un antirebond logiciel léger (DEBOUNCE_TIME_MS) est conservé en sécurité
 *
 * GPIO utilisé :
 *   GPIO_BUTTON_IO : entrée bouton (défini via CONFIG_GPIO_INPUT_0 ou modifiable)
 */

/* Configuration */
#define GPIO_BUTTON_IO      CONFIG_GPIO_INPUT_0   /* GPIO connecté au bouton */
#define GPIO_BUTTON_PIN_SEL (1ULL << GPIO_BUTTON_IO)

/*
 * Temps d'antirebond logiciel en millisecondes.
 * Avec 10nF, le filtrage matériel est déjà efficace.
 * On conserve 20ms de sécurité pour les cas limites.
 */
#define DEBOUNCE_TIME_MS    20

#define ESP_INTR_FLAG_DEFAULT 0

static QueueHandle_t gpio_evt_queue = NULL;

/* Timestamp du dernier appui validé (antirebond logiciel) */
static volatile int64_t last_press_time_us = 0;

/* Compteur d'appuis bouton */
static uint32_t press_count = 0;

/**
 * @brief Handler d'interruption GPIO 
 *
 * Envoie le numéro de GPIO dans la queue depuis l'ISR.
 */
static void IRAM_ATTR gpio_isr_handler(void* arg)
{
    uint32_t gpio_num = (uint32_t) arg;
    xQueueSendFromISR(gpio_evt_queue, &gpio_num, NULL);
}

/**
 * @brief Tâche de traitement des événements bouton
 *
 * Reçoit les événements depuis la queue et applique l'antirebond logiciel.
 * Affiche l'état du GPIO et le compteur d'appuis.
 */
static void gpio_button_task(void* arg)
{
    uint32_t io_num;

    for (;;) {
        /* Attente bloquante d'un événement GPIO */
        if (xQueueReceive(gpio_evt_queue, &io_num, portMAX_DELAY)) {

            int64_t now_us = esp_timer_get_time();
            int64_t elapsed_ms = (now_us - last_press_time_us) / 1000;

            /* Antirebond logiciel */
            if (elapsed_ms < DEBOUNCE_TIME_MS) {
                /* Rebond détecté, on ignore */
                continue;
            }

            /* Mise à jour du timestamp */
            last_press_time_us = now_us;

            /* Lecture de l'état réel du GPIO */
            int level = gpio_get_level(io_num);

            if (level == 1) {
                /* Front montant confirmé = appui bouton */
                press_count++;
                printf(">>> Bouton appuyé ! GPIO[%"PRIu32"] = %d | Compteur : %"PRIu32"\n",
                       io_num, level, press_count);
            }
        }
    }
}

void app_main(void)
{
    printf("Test bouton pull-down avec antirebond 10nF\n");
    printf("GPIO utilisé : %d\n", GPIO_BUTTON_IO);
    printf("Antirebond logiciel : %d ms\n", DEBOUNCE_TIME_MS);
    printf("Appuyez sur le bouton...\n\n");

    /* Configuration GPIO entrée bouton */
    gpio_config_t io_conf = {};

    /* Interruption sur front montant uniquement (bouton appuyé) */
    io_conf.intr_type = GPIO_INTR_POSEDGE;

    /* Masque de bit du GPIO bouton */
    io_conf.pin_bit_mask = GPIO_BUTTON_PIN_SEL;

    /* Mode entrée */
    io_conf.mode = GPIO_MODE_INPUT;

    /*
     * Pull-up interne désactivé : on utilise une résistance pull-down externe.
     * Activer le pull-down interne en complément est possible mais optionnel
     * si la résistance externe est déjà présente.
     */
    io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE; /* Résistance pull-down externe */

    gpio_config(&io_conf);
    gpio_evt_queue = xQueueCreate(10, sizeof(uint32_t));

    /* Démarrage de la tâche de traitement */
    xTaskCreate(gpio_button_task,   /* Fonction de la tâche */
                "gpio_button_task", /* Nom de la tâche */
                2048,               /* Taille de la pile en mots */
                NULL,               /* Paramètre passé à la tâche */
                10,                 /* Priorité */
                NULL);              /* Handle (non utilisé) */

    gpio_install_isr_service(ESP_INTR_FLAG_DEFAULT);
    gpio_isr_handler_add(GPIO_BUTTON_IO, gpio_isr_handler, (void*) GPIO_BUTTON_IO);

    printf("Heap libre minimum : %"PRIu32" bytes\n\n", esp_get_minimum_free_heap_size());

    /* Boucle principale */
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}