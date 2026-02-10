
                                Diagrama de pinagem - saídas de sinais
                                   ________________________________
                                  |EN          ________     GPIO 23|
                                  |GPI 36     |        |    GPIO 22|
                                  |GPI 39     | ESP32  |    GPIO  1|
                                  |GPIO 34    |        |    GPIO  3|
                                  |GPIO 35    |        |    GPIO 21|
                                  |GPI 32     |        |    GPIO 19| - PWM RAMPA           
                                  |GPI 33     |________|    GPIO 18| - PWM 25%            
                      ALEATORIO - |GPIO 25                  GPIO  5|
                        SENOIDE - |GPIO 26                  GPIO 17| - UART "OFF"         
                                  |GPIO 27                  GPIO 16|
                                  |GPIO 14                  GPIO  4| - MORSE               
                                  |GPIO 12                  GPIO  2| - TOGGLE
                                  |GPIO 13       ____       GPIO 15|
                                  |GND          |    |          GND| - Ponta de referência
                                  |VIN  ________|____|_______  3.3V| - Medição de tensão

