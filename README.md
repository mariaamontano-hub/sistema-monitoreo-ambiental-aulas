# Sistema de Monitoreo Ambiental en Aulas – Smart Campus UNL

## Descripción

Este repositorio contiene el desarrollo del **Sistema de Monitoreo Ambiental en Aulas**, implementado como parte del trabajo de titulación de la carrera de Ingeniería en Telecomunicaciones de la Universidad Nacional de Loja (UNL).

El objetivo del proyecto es diseñar e implementar un sistema capaz de supervisar en tiempo real las principales variables ambientales presentes en espacios académicos, proporcionando información que contribuya a la evaluación de las condiciones ambientales y al bienestar de la comunidad universitaria.

La solución desarrollada está conformada por dos nodos de monitoreo instalados en aulas diferentes, los cuales adquieren información de temperatura, humedad relativa, concentración de dióxido de carbono (CO₂), nivel de iluminación, nivel sonoro y detección de movimiento. Los datos son transmitidos mediante tecnología LoRa hacia un gateway, desde donde son enviados al servidor desarrollado con Django para su procesamiento, almacenamiento en una base de datos PostgreSQL y visualización mediante un dashboard web.

Este repositorio reúne el firmware de los nodos de monitoreo, el firmware del gateway utilizado durante la etapa de validación del sistema, el desarrollo de la plataforma web, la base de datos, los diseños electrónicos y mecánicos, así como la documentación técnica elaborada durante la implementación del proyecto.

> **Nota:** Durante el desarrollo del proyecto se implementó un gateway de pruebas para validar la comunicación entre los nodos de monitoreo y el servidor mediante tecnología LoRa y WiFi. El firmware correspondiente se incluye únicamente con fines de reproducción y validación del sistema. El desarrollo principal presentado en este repositorio corresponde a los nodos de monitoreo y a la plataforma de gestión y visualización de datos.

---

## Variables monitoreadas

- Temperatura.
- Humedad relativa.
- Concentración de dióxido de carbono (CO₂).
- Nivel de iluminación.
- Nivel sonoro.
- Detección de movimiento.

---

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

---

## Estructura del repositorio

```text
sistema-monitoreo-ambiental-aulas/
│
├── Nodos/
│   ├── Aula_1/             Firmware del nodo de monitoreo instalado en el Aula 1.
│   └── Aula_2/             Firmware del nodo de monitoreo instalado en el Aula 2.
├── Firmware_Pruebas/       Firmware del gateway utilizado únicamente durante la etapa de pruebas.
├── PCB/
│   ├── Diseño_PCB/         Diseño esquemático de la placa (imagen exportada de EasyEDA).
│   ├── Archivo_Gerber/     Archivos Gerber para fabricación de la PCB.
│   └── Serigrafía/         Serigrafía de identificación de componentes.
├── Diseño_Carcasa/         Modelo 3D de la carcasa protectora del nodo.
├── Django/                 Backend y dashboard web desarrollado con Django + PostgreSQL.
├── Manuales/
│   ├── Manual_Usuario/     Guía de uso del sistema para el usuario final.
│   └── Manual_Programador/ Guía técnica de instalación, firmware y base de datos.
│
├── README.md
├── LICENCIA
└── .gitignore

## Autor

**Maria Andreina Montaño Rojas**

Trabajo de Titulación previo a la obtención del título de **Ingeniera en Telecomunicaciones**.

**Carrera de Ingeniería en Telecomunicaciones**  
**Universidad Nacional de Loja**  
**Loja – Ecuador**
