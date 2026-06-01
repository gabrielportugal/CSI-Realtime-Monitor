"""
Coleta de Dados CSI
Responsável por coletar dados CSI do ESP32, extrair features e salvar no dataset.
"""

import serial
import csv
import numpy as np
import os
import sys
from feature_extraction import extract_features

# ===== CONFIGURAÇÕES =====
PORT = "COM5"      # Ajuste para a porta do seu ESP32
BAUD = 115200

LABEL = 1
# 0 = parado
# 1 = movimento

# Criar pasta train se não existir
os.makedirs("train", exist_ok=True)

# Verificar se o dataset já existe
dataset_path = "train/dataset.csv"
is_new_file = not os.path.exists(dataset_path)

# Contador de amostras
sample_count = 0

# ===== BEM-VINDO =====
print("\n" + "="*60)
print(" SISTEMA DE COLETA V1 ESP32 CSI ML")
print("="*60)
print(" Este sistema coleta dados CSI do ESP32 para")
print(" treinamento de modelos de Machine Learning.")
print("\n Arquivos disponiveis:")
print("  - collet.py  : Coleta dados do ESP32")
print("  - train.py   : Treina o modelo (voce esta aqui)")
print("  - predict.py : Faz predicoes em tempo real")
print("="*60 + "\n")

print(f"Iniciando coleta com LABEL = {LABEL}")
print(f"Porta: {PORT}")
if is_new_file:
  print("[INFO] Novo arquivo dataset.csv será criado")
else:
  print("[INFO] Dados serão adicionados ao dataset existente")
print("Pressione Ctrl+C para parar\n")

try:
  ser = serial.Serial(PORT, BAUD, timeout=2)
except serial.SerialException as e:
  print(f"ERRO: Nao foi possivel conectar a porta {PORT}")
  print(f"Detalhes: {e}")
  sys.exit(1)

try:
  with open(dataset_path, "a", newline="") as file:
    writer = csv.writer(file)

    while True:
      try:
        line = ser.readline().decode(errors="ignore").strip()

        if not line.startswith("RAW"):
          continue

        values = line.split(",")

        # Validação: verificar se há dados suficientes
        if len(values) < 5:
          print("Aviso: linha com dados insuficientes")
          continue

        timestamp = int(values[1])
        rssi = int(values[2])
        channel = int(values[3])

        csi = np.array(values[4:], dtype=float)

        # Validação: CSI não pode estar vazio
        if len(csi) == 0:
          continue

        features = extract_features(csi)

        row = [
          timestamp,
          rssi,
          channel,
          *features,
          LABEL
        ]

        writer.writerow(row)
        file.flush()  # Garante que os dados são salvos imediatamente

        sample_count += 1
        print(f"Amostra {sample_count} salva | RSSI: {rssi} | Canal: {channel}")

      except ValueError as e:
        print(f"Erro ao processar valores: {e}")
        continue
      except Exception as e:
        print(f"Erro inesperado: {e}")
        continue

except KeyboardInterrupt:
  print(f"\n\nColeta interrompida pelo usuário")
  print(f"Total de amostras coletadas: {sample_count}")

finally:
  if ser.is_open:
    ser.close()
  print("Conexao serial fechada")