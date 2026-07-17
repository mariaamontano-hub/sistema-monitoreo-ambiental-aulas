from django.contrib import admin
from .models import MedicionAmbiental


@admin.register(MedicionAmbiental)
class MedicionAmbientalAdmin(admin.ModelAdmin):
    list_display = (
        'nodo',
        'aula',
        'temperatura',
        'humedad',
        'co2',
        'iluminacion',
        'ruido',
        'movimiento',
        'fecha_hora',
    )

    list_filter = ('nodo', 'aula', 'movimiento')
    search_fields = ('nodo', 'aula')
    ordering = ('-fecha_hora',)