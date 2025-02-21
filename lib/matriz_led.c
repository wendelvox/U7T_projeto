#include "matriz_led.h"
#include "hardware/pio.h"
#include "ws2812.pio.h"
#include <stdio.h>

// Declaração do buffer de pixels
pixel_t leds[LED_COUNT];
PIO np_pio;
uint sm;

// Carrega promagram PIO
void npInit(uint pin) {
    uint offset = pio_add_program(pio0, &U7T_projeto_program);
    np_pio = pio0;

    sm = pio_claim_unused_sm(np_pio, false);
    if (sm < 0) {
        np_pio = pio1;
        sm = pio_claim_unused_sm(np_pio, true);
    }

    U7T_projeto_program_init(np_pio, sm, offset, pin, 800000.f);

    // Limpa buffer de LEDs
    npClear();
}

void npSetLED(const uint index, const uint8_t r, const uint8_t g, const uint8_t b) {
    leds[index].R = r;
    leds[index].G = g;
    leds[index].B = b;
}

void npClear() {
    for (uint i = 0; i < LED_COUNT; ++i) {
        npSetLED(i, 0, 0, 0);
    }
}

void npWrite() {
    for (uint i = 0; i < LED_COUNT; ++i) {
        pio_sm_put_blocking(np_pio, sm, leds[i].G);
        pio_sm_put_blocking(np_pio, sm, leds[i].R);
        pio_sm_put_blocking(np_pio, sm, leds[i].B);
    }
    sleep_us(100);
}

void npDraw(uint8_t vetorR[5][5], uint8_t vetorG[5][5], uint8_t vetorB[5][5]) {
    int i, j, idx, col;
    for (i = 0; i < 5; i++) {
        idx = (4 - i) * 5;
        for (j = 0; j < 5; j++) {
            col = (i % 2 == 0) ? (4 - j) : j;
            npSetLED(idx + col, vetorR[i][j], vetorG[i][j], vetorB[i][j]);
        }
    }
    npWrite();
}


// Vetor que representa os LEDs vermelhos e verdes da matriz
uint8_t vetorRG[5][5] = {
    { 0 , 0 , 0 , 0 , 0 },
    { 0 , 0 , 0 , 0 , 0 },
    { 0 , 0 , 0 , 0 , 0 },
    { 0 , 0 , 0 , 0 , 0 },
    { 0 , 0 , 0 , 0 , 0 }
};

void num_0() {
  // Vetor que representa os LEDs azuis
  uint8_t vetorB[5][5] = {
    { 0 , 1 , 1 , 1 , 0 },
    { 0 , 1 , 0 , 1 , 0 },
    { 0 , 1 , 0 , 1 , 0 },
    { 0 , 1 , 0 , 1 , 0 },
    { 0 , 1 , 1 , 1 , 0 }
  };
  npDraw(vetorRG,vetorRG,vetorB); // Carrega os buffers.
  npWrite();                      // Escreve na matriz de LEDs.

}

void num_1() {
  uint8_t vetorB[5][5] = {
    { 0 , 0 , 1 , 0 , 0 },
    { 0 , 1 , 1 , 0 , 0 },
    { 0 , 0 , 1 , 0 , 0 },
    { 0 , 0 , 1 , 0 , 0 },
    { 0 , 1 , 1 , 1 , 0 }
  };
  npDraw(vetorRG, vetorRG, vetorB);
  npWrite();

}

void num_2() {
  uint8_t vetorB[5][5] = {
    { 0 , 1 , 1 , 1 , 0 },
    { 0 , 0 , 0 , 1 , 0 },
    { 0 , 1 , 1 , 1 , 0 },
    { 0 , 1 , 0 , 0 , 0 },
    { 0 , 1 , 1 , 1 , 0 }
  };
  npDraw(vetorRG, vetorRG, vetorB);
  npWrite();

}

void num_3() {
  uint8_t vetorB[5][5] = {
    { 0 , 1 , 1 , 1 , 0 },
    { 0 , 0 , 0 , 1 , 0 },
    { 0 , 1 , 1 , 1 , 0 },
    { 0 , 0 , 0 , 1 , 0 },
    { 0 , 1 , 1 , 1 , 0 }
  };
  npDraw(vetorRG, vetorRG, vetorB);
  npWrite();

}

void num_4() {
  uint8_t vetorB[5][5] = {
    { 0 , 1 , 0 , 1 , 0 },
    { 0 , 1 , 0 , 1 , 0 },
    { 0 , 1 , 1 , 1 , 0 },
    { 0 , 0 , 0 , 1 , 0 },
    { 0 , 0 , 0 , 1 , 0 }
  };
  npDraw(vetorRG, vetorRG, vetorB);
  npWrite();

}

void num_5() {
  uint8_t vetorB[5][5] = {
    { 0 , 1 , 1 , 1 , 0 },
    { 0 , 1 , 0 , 0 , 0 },
    { 0 , 1 , 1 , 1 , 0 },
    { 0 , 0 , 0 , 1 , 0 },
    { 0 , 1 , 1 , 1 , 0 }
  };
  npDraw(vetorRG, vetorRG, vetorB);
  npWrite();

}

void num_6() {
  uint8_t vetorB[5][5] = {
    { 0 , 1 , 1 , 1 , 0 },
    { 0 , 1 , 0 , 0 , 0 },
    { 0 , 1 , 1 , 1 , 0 },
    { 0 , 1 , 0 , 1 , 0 },
    { 0 , 1 , 1 , 1 , 0 }
  };
  npDraw(vetorRG, vetorRG, vetorB);
  npWrite();

}

void num_7() {
  uint8_t vetorB[5][5] = {
    { 0 , 1 , 1 , 1 , 0 },
    { 0 , 0 , 0 , 1 , 0 },
    { 0 , 0 , 0 , 1 , 0 },
    { 0 , 0 , 0 , 1 , 0 },
    { 0 , 0 , 0 , 1 , 0 }
  };
  npDraw(vetorRG, vetorRG, vetorB);
  npWrite();

}

void num_8() {
  uint8_t vetorB[5][5] = {
    { 0 , 1 , 1 , 1 , 0 },
    { 0 , 1 , 0 , 1 , 0 },
    { 0 , 1 , 1 , 1 , 0 },
    { 0 , 1 , 0 , 1 , 0 },
    { 0 , 1 , 1 , 1 , 0 }
  };
  npDraw(vetorRG, vetorRG, vetorB);
  npWrite();

}

void num_9() {
  uint8_t vetorB[5][5] = {
    { 0 , 1 , 1 , 1 , 0 },
    { 0 , 1 , 0 , 1 , 0 },
    { 0 , 1 , 1 , 1 , 0 },
    { 0 , 0 , 0 , 1 , 0 },
    { 0 , 0 , 0 , 1 , 0 }
  };
  npDraw(vetorRG, vetorRG, vetorB);
  npWrite();

}

int handle_numbers(char num) {
    // Funções para desenhar os números (de 0 a 9) podem ser inseridas aqui
    // Exemplo: caso '0' chama função para desenhar o número 0, etc.
    // Cada número deve configurar os vetores R, G, B para a sua representação visual.
  switch(num) {
    case '0':
      num_0();
      break;
    case '1':
      num_1();
      break;
    case '2':
      num_2();
      break;
    case '3':
      num_3();
      break;
    case '4':
      num_4();
      break;
    case '5':
      num_5();
      break;
    case '6':
      num_6();
      break;
    case '7':
      num_7();
      break;
    case '8':
      num_8();
      break;
    case '9':
      num_9();
      break;
    default:
      npClear();
      npWrite();
      return 1;
  }
  printf("-------- %c Escrito na matriz! --------\n", num); // Printf para visualização no terminal
  return 0;
}
