# Sistema de Controle de Temperatura para Brassagem Cervejeira

AUTOR: WENDEL SOUZA SANTOS
![Badge](https://img.shields.io/badge/Status-Concluído-brightgreen) ![Badge](https://img.shields.io/badge/Plataforma-Raspberry%20Pi%20Pico-blue)

Este projeto simula o controle de temperatura em processos de brassagem cervejeira utilizando um Raspberry Pi Pico. O sistema ajusta automaticamente uma válvula de gás (simulada por um servo motor) com base na temperatura simulada, permitindo a automação de etapas críticas do processo.

---

## VIDEO
https://drive.google.com/file/d/1s2ys6HeXArQWGo5koBrbL72zPZjJaQNd/view?usp=sharing

## 📋 Índice

1. [Visão Geral](#visão-geral)
2. [Funcionalidades](#funcionalidades)
3. [Hardware Necessário](#hardware-necessário)
4. [Configuração do Projeto](#configuração-do-projeto)
5. [Instruções de Uso](#instruções-de-uso)
6. [Estrutura do Código](#estrutura-do-código)
7. [Contribuições](#contribuições)
8. [Licença](#licença)

---

## 🌟 Visão Geral

O objetivo deste projeto é demonstrar como sistemas embarcados podem ser usados para automatizar processos industriais, como o controle de temperatura em brassagem cervejeira. O sistema utiliza um joystick para simular a subida e descida da temperatura, enquanto um servo motor controla uma válvula de gás com base nos limites de temperatura definidos para cada estágio de brassagem.

---

## 🔧 Funcionalidades

- **Simulação de Temperatura:** O joystick ajusta a temperatura exibida no display OLED.
- **Controle de Válvula de Gás:** Um servo motor simula o controle da válvula de gás, abrindo ou fechando conforme a temperatura atinge os limites desejados.
- **Estágios de Brassagem:** O sistema suporta quatro estágios principais:
  - Parada Proteica
  - Beta Amilase
  - Alfa Amilase
  - Mash Out
- **Indicação Visual:** LEDs RGB e um display OLED fornecem feedback visual sobre o estado do sistema.
- **Alarme Sonoro:** Um buzzer indica quando um estágio é concluído.

---

## 💻 Hardware Necessário

Para executar este projeto, você precisará dos seguintes componentes:

- **Raspberry Pi Pico** (com suporte para MicroPython ou C/C++)
- **Display OLED SSD1306** (I2C)
- **Servo Motor** (ex.: SG90)
- **Joystick Analógico**
- **Buzzer Piezoelétrico**
- **LEDs RGB** (opcional)
- **Resistores** (para proteção dos LEDs)
- **Jumpers e Protoboard**

---

## ⚙️ Configuração do Projeto

### 1. Configuração do Ambiente de Desenvolvimento

Certifique-se de ter as seguintes ferramentas instaladas:

- **SDK do Raspberry Pi Pico**: [Guia de Instalação](https://datasheets.raspberrypi.com/pico/getting-started-with-pico.pdf)
- **Compilador GCC para ARM**: Disponível no SDK do Pico.
- **Editor de Código**: Recomendamos o VS Code com extensões para C/C++.

### 2. Conexões de Hardware

As conexões devem ser feitas conforme o diagrama abaixo:

| Componente         | Pino do Pico       |
|--------------------|--------------------|
| Joystick X         | GP27              |
| Joystick Y         | GP26              |
| Botão do Joystick  | GP22              |
| Servo Motor        | GP15              |
| Buzzer             | GP21              |
| LED Vermelho       | GP13              |
| LED Verde          | GP11              |
| LED Azul           | GP12              |
| Display OLED (SDA) | GP14              |
| Display OLED (SCL) | GP15              |

> **Nota:** Certifique-se de que o servo motor esteja alimentado com tensão adequada (5V recomendado).

---

## ▶️ Instruções de Uso

1. **Clone o Repositório:**
   ```bash
   git clone https://github.com/seu-usuario/nome-do-repositorio.git
   cd nome-do-repositorio
   ```

2. **Compile o Código:**
   ```bash
   mkdir build
   cd build
   cmake ..
   make
   ```

3. **Transfira o Código para o Pico:**
   - Conecte o Raspberry Pi Pico ao computador enquanto mantém o botão `BOOTSEL` pressionado.
   - Copie o arquivo `.uf2` gerado para o dispositivo montado como unidade USB.

4. **Execute o Programa:**
   - O sistema iniciará automaticamente após o carregamento do firmware.

---

## 📂 Estrutura do Código

O projeto está organizado da seguinte forma:

```
├── lib/               # Bibliotecas externas (ex.: SSD1306)
├── src/               # Código-fonte principal
│   ├── main.c         # Arquivo principal com lógica do sistema
│   ├── ssd1306.h      # Driver para o display OLED
│   └── U7T_projeto.pio.h # Configuração PIO para LEDs WS2812
├── CMakeLists.txt     # Configuração de compilação
└── README.md          # Este arquivo
```

---

## 🤝 Contribuições

Contribuições são bem-vindas! Se você deseja melhorar este projeto, siga os passos abaixo:

1. Faça um fork do repositório.
2. Crie uma branch para sua feature (`git checkout -b feature/nova-funcionalidade`).
3. Envie suas alterações (`git commit -m 'Adiciona nova funcionalidade'`).
4. Faça push para a branch (`git push origin feature/nova-funcionalidade`).
5. Abra um Pull Request.

---
