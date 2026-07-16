# Sistema de Monitoreo Ambiental en Aulas — Smart Campus UNL

## Descripción

Este repositorio contiene el desarrollo del sistema de monitoreo ambiental para aulas de la Universidad Nacional de Loja (UNL), implementado como parte del trabajo de titulación de la carrera de Ingeniería en Telecomunicaciones.

El proyecto tiene como finalidad diseñar e implementar un sistema de monitoreo capaz de supervisar en tiempo real las principales variables ambientales presentes en espacios académicos, con el propósito de proporcionar información que contribuya a la evaluación de las condiciones ambientales y al bienestar de la comunidad universitaria.

La solución desarrollada está conformada por dos nodos de monitoreo instalados en aulas diferentes, los cuales realizan la adquisición de información correspondiente a temperatura, humedad relativa, concentración de dióxido de carbono (CO₂), nivel de iluminación, nivel sonoro y detección de movimiento. La transmisión de la información se realiza mediante tecnología LoRa, mientras que el procesamiento, almacenamiento y visualización de los datos se implementan mediante una plataforma web desarrollada con Django y PostgreSQL.

Este repositorio reúne el código fuente, la documentación técnica y los archivos de diseño desarrollados durante la implementación del sistema.

## Variables monitoreadas

- Temperatura.
- Humedad relativa.
- Concentración de dióxido de carbono (CO₂).
- Nivel de iluminación.
- Nivel sonoro.
- Detección de movimiento.

## Tecnologías empleadas

### Hardware

- Heltec WiFi LoRa 32 V3.
- Sensor SHT31.
- Sensor MH-Z19C.
- Sensor BH1750.
- Micrófono digital INMP441.
- Sensor PIR HC-SR501.

### Software

- Arduino IDE.
- Python.
- Django.
- PostgreSQL.
- HTML.
- CSS.
- JavaScript.

## Estructura del repositorio

```text
Sistema-Monitoreo-Ambiental-Aulas
│
├── Backend/             Plataforma web desarrollada en Django.
├── Base_de_datos/       Scripts y archivos relacionados con PostgreSQL.
├── Diseño_Carcasa/      Diseño de la carcasa del prototipo.
├── Manuales/            Manual de Usuario y Manual del Programador.
├── Nodos/               Código fuente de los nodos de monitoreo.
├── PCB/                 Diseño electrónico, serigrafía y archivos Gerber.
│
├── README.md
├── LICENSE
└── .gitignore
```

## Autor

**Maria Andreina Montaño Rojas**

Trabajo de titulación presentado para optar al título de Ingeniera en Telecomunicaciones.

Universidad Nacional de Loja.
