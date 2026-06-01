"""
Treinamento do Modelo Random Forest
Treina um classificador para detectar movimento usando dados CSI.
"""

import pandas as pd
import os
import sys
from sklearn.ensemble import RandomForestClassifier
from sklearn.model_selection import train_test_split
from sklearn.metrics import accuracy_score, classification_report, confusion_matrix
import joblib

# ===== BEM-VINDO =====
print("\n" + "="*60)
print(" SISTEMA DE TREINAMENTO V1 ESP32 CSI ML")
print("="*60)
print(" Este sistema treina modelos de Machine Learning para")
print(" deteccao de movimento usando dados CSI.")
print("\n Arquivos disponiveis:")
print("  - collet.py  : Coleta dados do ESP32")
print("  - train.py   : Treina o modelo (voce esta aqui)")
print("  - predict.py : Faz predicoes em tempo real")
print("="*60 + "\n")

# ===== CONFIGURAÇÕES =====
DATASET_PATH = "train/dataset.csv"
MODEL_PATH = "train/model.pkl"

# Validação: verificar se o dataset existe
if not os.path.exists(DATASET_PATH):
  print(f"ERRO: Dataset nao encontrado em '{DATASET_PATH}'")
  print("Execute 'collet.py' primeiro para coletar dados")
  sys.exit(1)

print(f"Carregando dataset de '{DATASET_PATH}'...")

df = pd.read_csv(DATASET_PATH, header=None)

# Definir nomes das colunas
df.columns = [
  "timestamp",
  "rssi",
  "channel",
  "mean",
  "var",
  "std",
  "energy",
  "max",
  "min",
  "label"
]

print(f"Total de amostras: {len(df)}")

# Validação: verificar quantidade mínima de dados
if len(df) < 20:
  print("AVISO: Dataset muito pequeno (< 20 amostras)")
  print("Recomenda-se coletar mais dados para melhor treinamento")

# Verificar distribuição das classes
print("\nDistribuição das classes:")
print(df['label'].value_counts())

# Verificar balanceamento
label_counts = df['label'].value_counts()
if len(label_counts) < 2:
  print("\nERRO: Dataset contém apenas uma classe")
  print("Colete dados com LABEL = 0 e LABEL = 1")
  sys.exit(1)

# Features usadas no treinamento
X = df[[
  "rssi",
  "mean",
  "var",
  "std",
  "energy",
  "max",
  "min"
]]

y = df["label"]

print("\nDividindo dataset (80% treino, 20% teste)...")

X_train, X_test, y_train, y_test = train_test_split(
  X,
  y,
  test_size=0.2,
  random_state=42,
  stratify=y  # Mantém proporção das classes
)

print(f"Treino: {len(X_train)} amostras")
print(f"Teste: {len(X_test)} amostras")

print("\nTreinando Random Forest...")

model = RandomForestClassifier(
  n_estimators=100,
  random_state=42,
  n_jobs=-1  # Usa todos os processadores disponíveis
)

model.fit(X_train, y_train)

print("Treinamento concluído!")

# Avaliação
predictions = model.predict(X_test)
acc = accuracy_score(y_test, predictions)

print("\n" + "="*50)
print("RESULTADOS")
print("="*50)
print(f"\nAcuracia: {acc:.4f} ({acc*100:.2f}%)")

print("\nRelatorio de Classificacao:")
print(classification_report(y_test, predictions, target_names=["Parado", "Movimento"]))

print("Matriz de Confusao:")
print(confusion_matrix(y_test, predictions))

# Salvar modelo
print(f"\nSalvando modelo em '{MODEL_PATH}'...")
joblib.dump(model, MODEL_PATH)
print("Modelo salvo com sucesso!")

print("\nExecute 'predict.py' para usar o modelo em tempo real")