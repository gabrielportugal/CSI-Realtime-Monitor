"""
Predição em Tempo Real
Usa o modelo treinado para classificar movimento em tempo real.
"""

import serial
import numpy as np
import pandas as pd
import joblib
import os
import sys
from feature_extraction import extract_features

# ===== CONFIGURAÇÕES =====
PORT = "COM5"
BAUD = 115200
MODEL_PATH = "train/model.pkl"
# ===== BEM-VINDO =====
print("\n" + "="*60)
print(" SISTEMA DE PREDICAO V1 ESP32 CSI ML")
print("="*60)
print(" Este sistema faz predicoes em tempo real usando modelos de Machine Learning")
print(" para deteccao de movimento com dados CSI.")
print("\n Arquivos disponiveis:")
print("  - collet.py  : Coleta dados do ESP32")
print("  - train.py   : Treina o modelo (voce esta aqui)")
print("  - predict.py : Faz predicoes em tempo real")
print("="*60 + "\n")

# Validação: verificar se o modelo existe
if not os.path.exists(MODEL_PATH):
  print(f"ERRO: Modelo nao encontrado em '{MODEL_PATH}'")
  print("Execute 'train.py' primeiro para treinar o modelo")
  sys.exit(1)

print("Carregando modelo...")
try:
  model = joblib.load(MODEL_PATH)
  print("Modelo carregado com sucesso!\n")
except Exception as e:
  print(f"ERRO ao carregar modelo: {e}")
  sys.exit(1)

print(f"Conectando a porta {PORT}...")
try:
  ser = serial.Serial(PORT, BAUD, timeout=2)
  print("Conexao estabelecida!")
except serial.SerialException as e:
  print(f"ERRO: Nao foi possivel conectar a porta {PORT}")
  print(f"Detalhes: {e}")
  sys.exit(1)

print("\n" + "="*50)
print("SISTEMA DE DETECCAO DE MOVIMENTO ATIVO")
print("="*50)
print("Pressione Ctrl+C para parar\n")

prediction_count = 0

try:
  while True:
    try:
      line = ser.readline().decode(errors="ignore").strip()

      if not line.startswith("RAW"):
        continue

      values = line.split(",")

      # Validação: verificar dados suficientes
      if len(values) < 5:
        continue

      rssi = int(values[2])
      csi = np.array(values[4:], dtype=float)

      # Validação: CSI não pode estar vazio
      if len(csi) == 0:
        continue

      features = extract_features(csi)

      # Preparar amostra para predição com nomes de colunas (evita warning)
      sample = pd.DataFrame([[
        rssi,
        features[0],  # mean
        features[1],  # var
        features[2],  # std
        features[3],  # energy
        features[4],  # max
        features[5]   # min
      ]], columns=["rssi", "mean", "var", "std", "energy", "max", "min"])

      prediction = model.predict(sample)
      prediction_count += 1

      # Feedback visual melhorado
      if prediction[0] == 0:
        status = "[PARADO]"
      else:
        status = "[MOVIMENTO]"

      print(f"[{prediction_count}] {status} | RSSI: {rssi:3d} dBm")

    except ValueError as e:
      print(f"Erro ao processar valores: {e}")
      continue
    except Exception as e:
      print(f"Erro inesperado: {e}")
      continue

except KeyboardInterrupt:
  print("\n\nSistema interrompido pelo usuario")
  print(f"Total de predicoes: {prediction_count}")

finally:
  if ser.is_open:
    ser.close()
  print("Conexao serial fechada")