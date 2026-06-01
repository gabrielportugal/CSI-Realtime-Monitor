"""
Verifica portas COM disponíveis
"""
import serial.tools.list_ports

print("\n=== Portas COM Disponíveis ===\n")
ports = serial.tools.list_ports.comports()

if not ports:
    print("Nenhuma porta COM detectada!")
else:
    for port in ports:
        print(f"Porta: {port.device}")
        print(f"  Descrição: {port.description}")
        print(f"  Hardware ID: {port.hwid}")
        print()
