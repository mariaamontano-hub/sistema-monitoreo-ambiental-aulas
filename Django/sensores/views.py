from django.shortcuts import render
from django.http import HttpResponse
from django.utils import timezone
from django.contrib.auth.decorators import login_required
from django.contrib.auth import logout
from rest_framework import viewsets

from .models import MedicionAmbiental
from .serializer import MedicionAmbientalSerializer

import json
import csv


class MedicionAmbientalViewSet(viewsets.ModelViewSet):
    queryset = MedicionAmbiental.objects.all().order_by('-fecha_hora')
    serializer_class = MedicionAmbientalSerializer


def dashboard(request):
    aula_seleccionada = request.GET.get('aula', 'todas')

    aulas = MedicionAmbiental.objects.values_list(
        'aula',
        flat=True
    ).distinct().order_by('aula')

    if aula_seleccionada == 'todas':
        mediciones = MedicionAmbiental.objects.all().order_by('-fecha_hora')[:20]
    else:
        mediciones = MedicionAmbiental.objects.filter(
            aula=aula_seleccionada
        ).order_by('-fecha_hora')[:20]

    mediciones_grafica = list(reversed(mediciones))

    estado_general = "Sin datos"
    alertas_ambientales = []

    if mediciones:
        ultima = mediciones[0]
        alertas = 0

        if ultima.co2 >= 1000:
            alertas += 1
            alertas_ambientales.append(
                f"CO₂ elevado en {ultima.aula}: se recomienda ventilar el espacio."
            )

        if ultima.ruido > 50:
            alertas += 1
            alertas_ambientales.append(
                f"Ruido elevado en {ultima.aula}: revisar fuentes de ruido o actividad en el aula."
            )

        if ultima.iluminacion < 100:
            alertas += 1
            alertas_ambientales.append(
                f"Iluminación muy baja en {ultima.aula}: revisar luminarias o entrada de luz natural."
            )

        if ultima.humedad < 30 or ultima.humedad > 70:
            alertas += 1
            alertas_ambientales.append(
                f"Humedad fuera de rango en {ultima.aula}: revisar ventilación o condiciones ambientales."
            )

        if ultima.temperatura < 18 or ultima.temperatura > 28:
            alertas += 1
            alertas_ambientales.append(
                f"Temperatura fuera de confort en {ultima.aula}: revisar ventilación o climatización."
            )

        if alertas == 0:
            estado_general = "Adecuado"
        elif alertas <= 2:
            estado_general = "Precaución"
        else:
            estado_general = "Crítico"

    datos_grafica = {
        'fechas': json.dumps([
            timezone.localtime(m.fecha_hora).strftime("%H:%M:%S")
            for m in mediciones_grafica
        ]),
        'temperatura': json.dumps([m.temperatura for m in mediciones_grafica]),
        'humedad': json.dumps([m.humedad for m in mediciones_grafica]),
        'co2': json.dumps([m.co2 for m in mediciones_grafica]),
        'iluminacion': json.dumps([m.iluminacion for m in mediciones_grafica]),
        'ruido': json.dumps([m.ruido for m in mediciones_grafica]),
    }

    return render(request, 'sensores/dashboard.html', {
        'mediciones': mediciones,
        'datos_grafica': datos_grafica,
        'aulas': aulas,
        'aula_seleccionada': aula_seleccionada,
        'estado_general': estado_general,
        'alertas_ambientales': alertas_ambientales,
        'total_mediciones': len(mediciones),
    })


@login_required
def exportar_csv(request):
    if not request.user.is_staff:
        return HttpResponse("No autorizado", status=403)

    response = HttpResponse(content_type='text/csv')
    response['Content-Disposition'] = (
        'attachment; filename="ultimas_20_mediciones_ambientales.csv"'
    )

    writer = csv.writer(response, delimiter=';')

    writer.writerow([
        'Fecha y hora',
        'Nodo',
        'Aula',
        'Temperatura',
        'Humedad',
        'CO2',
        'Iluminacion',
        'Ruido',
        'Movimiento'
    ])

    mediciones = MedicionAmbiental.objects.all().order_by('-fecha_hora')[:20]

    for m in mediciones:
        writer.writerow([
            timezone.localtime(m.fecha_hora).strftime("%Y-%m-%d %H:%M:%S"),
            m.nodo,
            m.aula,
            m.temperatura,
            m.humedad,
            m.co2,
            m.iluminacion,
            m.ruido,
            "Si" if m.movimiento else "No"
        ])

    logout(request)

    return response