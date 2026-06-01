/*
=========================================================
ESP32 CSI RAW DATA LOGGER
Versão V1 - Dataset para Machine Learning

Objetivo:
Capturar CSI bruto e enviar para Python

Formato:
RAW,timestamp,rssi,channel,csi1,csi2,csi3,...

Autor: Gabriel Portugal
=========================================================
*/

#include "WiFi.h"
#include "esp_wifi.h"

/*
=========================================================
CONFIGURAÇÃO WIFI
=========================================================
*/

const char* ssid = "SEU_WIFI";
const char* password = "SUA_SENHA";

/*
=========================================================
CONFIGURAÇÕES
=========================================================
*/

// Envia somente 1 pacote a cada X ms
#define SAMPLE_INTERVAL 50

unsigned long lastSample = 0;

/*
=========================================================
CALLBACK CSI
=========================================================
*/

static void wifi_csi_rx_cb(void *ctx, wifi_csi_info_t *data)
{
    if (!data || !data->buf)
        return;

    unsigned long now = millis();

    // Controle da taxa de envio
    if ((now - lastSample) < SAMPLE_INTERVAL)
        return;

    lastSample = now;

    int8_t *csi = data->buf;
    int len = data->len;

    // Validação: garantir que há dados CSI
    if (len <= 0 || len > 384)
        return;

    int rssi = data->rx_ctrl.rssi;
    int channel = data->rx_ctrl.channel;

    /*
    =====================================================
    FORMATO:

    RAW,timestamp,rssi,channel,csi1,csi2,...
    =====================================================
    */

    Serial.print("RAW,");

    // timestamp
    Serial.print(now);
    Serial.print(",");

    // RSSI
    Serial.print(rssi);
    Serial.print(",");

    // Canal
    Serial.print(channel);

    // CSI bruto
    for (int i = 0; i < len; i++)
    {
        Serial.print(",");
        Serial.print(csi[i]);
    }

    Serial.println();
}

/*
=========================================================
SETUP
=========================================================
*/

void setup()
{
    Serial.begin(115200);

    delay(2000);

    Serial.println();
    Serial.println("================================");
    Serial.println("ESP32 CSI RAW LOGGER");
    Serial.println("================================");

    /*
    =====================================================
    WIFI STATION
    =====================================================
    */

    WiFi.mode(WIFI_STA);

    WiFi.begin(ssid, password);

    Serial.print("Conectando");

    // Timeout de 20 segundos para conexão WiFi
    int timeout = 0;
    while (WiFi.status() != WL_CONNECTED && timeout < 40)
    {
        delay(500);
        Serial.print(".");
        timeout++;
    }

    if (WiFi.status() != WL_CONNECTED)
    {
        Serial.println();
        Serial.println("ERRO: Falha na conexão WiFi");
        Serial.println("Verifique SSID e senha");
        while(1); // Para a execução
    }

    Serial.println();
    Serial.println("WiFi conectado");

    Serial.print("IP: ");
    Serial.println(WiFi.localIP());

    /*
    =====================================================
    MODO PROMISCUO
    =====================================================
    */

    esp_wifi_set_promiscuous(true);

    /*
    =====================================================
    CONFIGURAÇÃO CSI
    =====================================================
    */

    wifi_csi_config_t csi_config = {

        .lltf_en = true,

        .htltf_en = true,

        .stbc_htltf2_en = true,

        .ltf_merge_en = true,

        .channel_filter_en = true,

        .manu_scale = false,

        .shift = false
    };

    esp_wifi_set_csi_config(&csi_config);

    /*
    =====================================================
    CALLBACK CSI
    =====================================================
    */

    esp_wifi_set_csi_rx_cb(&wifi_csi_rx_cb, NULL);

    /*
    =====================================================
    ATIVA CSI
    =====================================================
    */

    esp_wifi_set_csi(true);

    Serial.println();
    Serial.println("CSI ATIVADO");
    Serial.println("Transmitindo dados RAW...");
    Serial.println();
}

/*
=========================================================
LOOP
=========================================================
*/

void loop()
{
    delay(50);  // 50ms = ~20 iterações/seg (baixo consumo)
}