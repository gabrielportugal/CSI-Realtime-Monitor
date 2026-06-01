"""
Feature Extraction para CSI
Extrai características estatísticas dos dados CSI para alimentar o modelo ML.

Features extraídas:
- Média: intensidade média do sinal
- Variância: dispersão dos valores
- Desvio padrão: raiz quadrada da variância
- Energia: soma das magnitudes absolutas
- Máximo: maior valor observado
- Mínimo: menor valor observado
"""

import numpy as np

def extract_features(csi):
  """
  Extrai features estatísticas de um array CSI.
  
  Args:
    csi: numpy array com valores CSI
    
  Returns:
    Lista com [mean, variance, std, energy, max, min]
    
  Raises:
    ValueError: se csi estiver vazio ou None
  """
  
  if csi is None or len(csi) == 0:
    raise ValueError("CSI não pode estar vazio")
  
  # Conversão para numpy array se necessário
  csi_array = np.asarray(csi, dtype=float)
  
  mean = np.mean(csi_array)
  variance = np.var(csi_array)
  std = np.std(csi_array)
  energy = np.sum(np.abs(csi_array))
  maximum = np.max(csi_array)
  minimum = np.min(csi_array)

  return [
    mean,
    variance,
    std,
    energy,
    maximum,
    minimum
  ]
