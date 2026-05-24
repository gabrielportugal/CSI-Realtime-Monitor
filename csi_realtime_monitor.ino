/*
=========================================================
  ESP32 CSI REALTIME MONITOR
  Monitor de Channel State Information em Tempo Real
=========================================================

Descrição:
  Captura e visualiza CSI (Channel State Information) do WiFi
  para detectar mudanças ambientais através de variações no
  sinal sem fio. Utiliza análise de subportadoras OFDM.

Hardware:
  - Placa: ESP32-WROOM-32
  - WiFi: 2.4GHz (modo promíscuo)
  - Comunicação: Serial 115200 baud

Detecção de:
  ✓ Movimento de pessoas
  ✓ Presença em ambiente
  ✓ Alterações físicas no espaço
  ✓ Mudanças na propagação RF

Visualização:
  Arduino IDE → Tools → Serial Plotter (115200 baud)

Autor: Gabriel Portugal
Data: Maio 2026

=========================================================
*/

#include "WiFi.h"
#include "esp_wifi.h"

/*
=========================================================
  CONFIGURAÇÃO DE REDE WiFi
=========================================================
  Credenciais da rede WiFi 2.4GHz para conexão.
  IMPORTANTE: Altere com suas credenciais antes do upload.
=========================================================
*/

const char* ssid = "";           // Nome da rede WiFi (SSID)
const char* password = "";   // Senha da rede WiFi

/*
=========================================================
  VARIÁVEIS GLOBAIS
=========================================================
  Controle do processamento de sinal CSI e detecção.
=========================================================
*/

// Energia média calculada do CSI (não utilizada atualmente)
float energiaCSI = 0;

// Contador de amostras processadas (não utilizado atualmente)
int amostras = 0;

// Média suavizada com filtro EMA (α=0.1) - valor principal de análise
float mediaSuavizada = 0;

// Threshold para detecção de movimento (legacy - não usado com delta 0.4)
float thresholdMovimento = 16.0;

// Último valor CSI processado - usado para calcular variação (delta)
float ultimoCSI = 0;

/*
=========================================================
  CALLBACK CSI - FUNÇÃO DE PROCESSAMENTO PRINCIPAL
=========================================================
  Executada automaticamente a cada pacote WiFi recebido
  pelo ESP32 quando o modo CSI está ativo.
  
  Processamento:
  1. Extrai dados CSI do buffer
  2. Calcula energia das subportadoras
  3. Aplica filtro de suavização EMA
  4. Detecta movimento por análise de delta
  5. Envia dados formatados para Serial Plotter
=========================================================
*/

static void wifi_csi_rx_cb(void *ctx, wifi_csi_info_t *data)
{
    if (!data)
        return;

    /*
    =====================================================
    BUFFER CSI - Dados das Subportadoras
    =====================================================
    Ponteiro para array de valores CSI (int8_t)
    Cada valor representa amplitude de uma subportadora OFDM
    =====================================================
    */

    int8_t *csi = data->buf;

    /*
    =====================================================
    TAMANHO DO BUFFER CSI
    =====================================================
    Número de subportadoras capturadas (varia conforme
    configuração e tipo de pacote WiFi recebido)
    =====================================================
    */

    int len = data->len;

    /*
    =====================================================
    CÁLCULO DE ENERGIA CSI
    =====================================================
    Soma dos valores absolutos de todas as subportadoras.
    Representa a energia total do sinal multi-path.
    =====================================================
    */

    float soma = 0;

    for (int i = 0; i < len; i++)
    {
        soma += abs(csi[i]);  // Valor absoluto de cada subportadora
    }

    /*
    =====================================================
    ENERGIA MÉDIA NORMALIZADA
    =====================================================
    Divide pela quantidade de subportadoras para normalizar
    independente do tamanho do buffer.
    =====================================================
    */

    float energia = soma / len;

    /*
    =====================================================
    FILTRO EMA (Exponential Moving Average)
    =====================================================
    Suavização exponencial com α=0.1 (fator de peso)
    
    Fórmula: S(t) = α·x(t) + (1-α)·S(t-1)
    - Reduz ruído de alta frequência
    - Mantém tendências de baixa frequência
    - α baixo = mais suave, menos responsivo
    =====================================================
    */

    mediaSuavizada =
        (0.9 * mediaSuavizada) +  // 90% do valor anterior
        (0.1 * energia);          // 10% do valor atual

    /*
    =====================================================
    RSSI (Received Signal Strength Indicator)
    =====================================================
    Indicador de força do sinal em dBm (valores negativos)
    Típico: -30 (excelente) a -90 (fraco)
    Diferente do CSI: RSSI é medida agregada, CSI é detalhado
    =====================================================
    */

    int rssi = data->rx_ctrl.rssi;

    /*
    =====================================================
    SAÍDA FORMATADA PARA SERIAL PLOTTER
    =====================================================
    Formato: nome:valor,nome:valor,...
    Arduino Plotter cria gráfico automático com este formato.
    =====================================================
    */

    Serial.print("CSI:");              // Energia CSI suavizada
    Serial.print(mediaSuavizada);

    Serial.print(",");

    Serial.print("RSSI:");             // Força do sinal (dBm)
    Serial.print(rssi);

    Serial.print(",");

    /*
    =====================================================
    DETECTOR DE MOVIMENTO POR ANÁLISE DE VARIAÇÃO
    =====================================================
    Calcula diferença absoluta entre leituras consecutivas.
    
    Threshold: 0.4
    - delta > 0.4 → movimento detectado (MOV=100)
    - delta ≤ 0.4 → sem movimento (MOV=0)
    
    Princípio: movimento altera propagação multi-path,
    causando variação nas subportadoras CSI.
    =====================================================
    */

    float delta = abs(mediaSuavizada - ultimoCSI);  // Variação absoluta
    ultimoCSI = mediaSuavizada;                     // Atualiza referência
    
    Serial.print("ultimoCSI:");        // Valor anterior (referência)
    Serial.print(ultimoCSI);
    Serial.print(",");
    Serial.print("delta:");            // Variação calculada
    Serial.print(delta);
    Serial.print(",");

    if (delta > 0.4)                   // Threshold de sensibilidade
    {
        Serial.println("MOV:100");     // Movimento detectado
    }
    else
    {
        Serial.println("MOV:0");       // Ambiente estável
    }
}

/*
=========================================================
  SETUP - INICIALIZAÇÃO DO SISTEMA
=========================================================
  Configuração única executada ao ligar/resetar o ESP32:
  1. Inicializa comunicação serial
  2. Conecta ao WiFi
  3. Ativa modo promíscuo
  4. Configura e ativa CSI
  5. Registra callback de processamento
=========================================================
*/

void setup()
{
    Serial.begin(115200);              // Inicia serial (115200 baud)

    delay(2000);                       // Aguarda estabilização

    Serial.println("INICIANDO CSI PLOTTER");

    /*
    =====================================================
    MODO WIFI STATION (STA)
    =====================================================
    Configura ESP32 como cliente WiFi (não Access Point)
    Necessário para conectar em rede existente
    =====================================================
    */

    WiFi.mode(WIFI_STA);

    /*
    =====================================================
    CONEXÃO À REDE WIFI
    =====================================================
    Tenta conectar usando credenciais configuradas.
    Loop bloqueante até estabelecer conexão.
    =====================================================
    */

    WiFi.begin(ssid, password);

    Serial.print("Conectando");

    while (WiFi.status() != WL_CONNECTED)  // Aguarda conexão
    {
        delay(500);
        Serial.print(".");                 // Indicador visual
    }

    Serial.println();
    Serial.println("WiFi conectado!");

    /*
    =====================================================
    ATIVA MODO PROMÍSCUO
    =====================================================
    Permite capturar todos os pacotes WiFi no canal,
    não apenas os destinados ao ESP32.
    ATENÇÃO: Verifique legalidade no seu país/região.
    =====================================================
    */

    esp_wifi_set_promiscuous(true);

    /*
    =====================================================
    CONFIGURAÇÃO CSI (Channel State Information)
    =====================================================
    Define quais componentes do sinal capturar:
    
    lltf_en         → Legacy Long Training Field (802.11a/g)
    htltf_en        → HT Long Training Field (802.11n)
    stbc_htltf2_en  → STBC HT-LTF2 (Space-Time Block Coding)
    ltf_merge_en    → Combinar múltiplos LTF
    channel_filter  → Aplicar filtro de canal
    manu_scale      → Escala manual (desabilitado)
    shift           → Deslocamento de bits (desabilitado)
    =====================================================
    */

    wifi_csi_config_t csi_config = {

        .lltf_en = true,           // Captura pacotes Legacy

        .htltf_en = true,          // Captura pacotes HT (802.11n)

        .stbc_htltf2_en = true,    // Habilita STBC

        .ltf_merge_en = true,      // Merge de campos LTF

        .channel_filter_en = true, // Filtragem de canal ativa

        .manu_scale = false,       // Auto-scaling

        .shift = false             // Sem deslocamento de bits
    };

    /*
    =====================================================
    APLICA CONFIGURAÇÃO CSI AO HARDWARE
    =====================================================
    Envia configurações para o chip WiFi Espressif
    =====================================================
    */

    esp_wifi_set_csi_config(&csi_config);

    /*
    =====================================================
    REGISTRA FUNÇÃO CALLBACK
    =====================================================
    Define wifi_csi_rx_cb como handler de eventos CSI.
    Será chamada automaticamente a cada pacote recebido.
    =====================================================
    */

    esp_wifi_set_csi_rx_cb(&wifi_csi_rx_cb, NULL);

    /*
    =====================================================
    ATIVA CAPTURA CSI
    =====================================================
    Inicia a captura de Channel State Information.
    A partir daqui o callback será executado continuamente.
    =====================================================
    */

    esp_wifi_set_csi(true);

    Serial.println("CSI ATIVADO");
    Serial.println("Abra Serial Plotter para visualização!");
}

/*
=========================================================
  LOOP PRINCIPAL
=========================================================
  Loop vazio - todo processamento ocorre no callback CSI.
  Delay evita watchdog reset e reduz consumo de CPU.
=========================================================
*/

void loop()
{
    delay(50);  // 50ms = ~20 iterações/seg (baixo consumo)
}