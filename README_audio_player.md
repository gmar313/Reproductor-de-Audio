# 🎵 Real-Time 8-bit Audio Player — STM32 + FreeRTOS

Reproductor de audio en tiempo real implementado sobre un microcontrolador STM32, con procesamiento de señal digital, visualización de espectro de frecuencias y arquitectura multitarea con FreeRTOS.

## Demo

> 📹 [Video de funcionamiento]	https://youtu.be/eHPBzsD5Xt4 
---

## Descripción

El sistema reproduce archivos de audio almacenados en una tarjeta SD de forma continua y sin interrupciones, aplicando filtros de audio en tiempo real controlados por el usuario. El espectro de frecuencias se visualiza en vivo en una pantalla OLED.

El diseño se basa en una arquitectura de doble buffer (ping-pong buffer) que permite leer y reproducir datos simultáneamente, garantizando reproducción sin cortes.

---

## Características principales

- Reproducción de audio de 8 bits en tiempo real desde tarjeta SD
- Arquitectura multitarea con **FreeRTOS** (lectura, procesamiento y reproducción en tareas separadas)
- **Doble buffer (ping-pong)** para reproducción continua sin glitches
- Dos **filtros biquad** (pasa-bajos y pasa-altos) con ganancia ajustable mediante potenciómetros
- Visualización del espectro de frecuencias en pantalla **OLED** mediante FFT
- Comunicación con SD card por SPI, pantalla OLED por I2C

---

## Hardware utilizado

| Componente | Descripción |
|---|---|
| STM32F103C8T6 | Microcontrolador principal |
| Tarjeta SD | Almacenamiento de archivos de audio |
| Pantalla OLED | Visualización del espectro (I2C) |
| Potenciómetros (x2) | Control de ganancia de filtros (ADC) |
| DAC / PWM | Salida de audio |

---

## Arquitectura del software

```
┌─────────────────────────────────────────┐
│              FreeRTOS                   │
├────────────┬────────────┬───────────────┤
│  Task: SD  │ Task: DSP  │ Task: Output  │
│  Reader    │ (Biquad +  │ (DAC/PWM +    │
│            │  FFT)      │  OLED)        │
└─────┬──────┴─────┬──────┴───────┬───────┘
      │            │              │
      └────────────┴──────────────┘
              Ping-Pong Buffer
```

**Flujo de datos:**
1. La tarea de lectura obtiene muestras de la SD y llena el buffer inactivo
2. La tarea DSP aplica los filtros biquad y calcula la FFT sobre el buffer activo
3. La tarea de salida envía las muestras procesadas al DAC y actualiza la pantalla OLED

---

## Decisiones de diseño

**¿Por qué doble buffer?** Leer desde SD tiene latencia variable. Con un solo buffer, cualquier demora en la lectura produce un corte en el audio. El doble buffer desacopla completamente la lectura de la reproducción.

**¿Por qué FreeRTOS?** Las tres operaciones (leer SD, procesar señal, reproducir) tienen requisitos de timing distintos. Separarlo en tareas con prioridades permite que el sistema cumpla los deadlines de audio sin bloqueos.

**Filtros biquad:** Se implementaron en forma directa II transpuesta para minimizar el uso de memoria y errores de punto fijo. Los coeficientes se recalculan cuando el usuario ajusta la ganancia con los potenciómetros.

---

## Cómo compilar y ejecutar

```bash
# Clonar el repositorio
git clone https://github.com/TU_USUARIO/audio-player-stm32

# Abrir el proyecto en STM32CubeIDE
# Compilar y flashear con ST-Link
```

**Requisitos:**
- STM32CubeIDE
- FreeRTOS (incluido en CubeMX)
- Librería FATFS para SD card

---

## Estructura del repositorio

```
TP-especial_final/
├── Core/
│   ├── Src/
│   │   ├── main.c
│   │   ├── fft.c     		# Implementación FFT
│   │   └── biquad.c       	# Implementación filtros biquad 
│   └── Inc/
├── Middlewares/
│   └── Third_Party/
│ 	├── FreeRTOS/
│	└── FatFs/		#implementacion de el lector SD
├── Drivers/
└── README.md
```

---

## Aprendizajes clave

- Implementación de DSP en microcontroladores con recursos limitados (sin FPU dedicada)
- Gestión de memoria y timing en sistemas de tiempo real con RTOS
- Diseño de pipelines de procesamiento de señal para audio embebido
